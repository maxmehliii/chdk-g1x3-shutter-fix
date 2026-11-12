#!/bin/bash
# sd_bootdisk.sh - mark an SD card (FAT32) for CHDK card boot on the g1x3 (macOS)
#
#   ./sd_bootdisk.sh status <diskN>   read only: shows the file system and whether "BOOTDISK" is set
#   ./sd_bootdisk.sh set    <diskN>   writes "BOOTDISK" at offset 0x1E0 of the FAT32 boot sector (sudo)
#   ./sd_bootdisk.sh clear  <diskN>   removes the mark again (8 zero bytes, sudo)
#
# Evidence (g1x3 dump GM1.00A, docs/NOTES_de.md Prompt 5):
#   FUN_e03a039e checks strncmp(bootsector+0x1E0,"BOOTDISK",8) on FAT32, +0x40 on FAT12/16.
#   exFAT is never recognized as bootable. -> the card must be FAT32 (SDHC up to 32 GB, formatted in the camera).
#   In addition, diskboot only starts when the card's write-protect slider is on LOCK (PhySwBootSD.c).
#
# Safety: the script writes ONLY 8 bytes in sector 0 of partition s1, and only after checking
# that it is an external, removable card <= 64 GB with FAT32 and you have typed the disk ID again.
set -euo pipefail

mode="${1:-}"; disk="${2:-}"
if [[ -z "$mode" || -z "$disk" ]]; then
    echo "Usage: $0 status|set|clear diskN   (diskN from 'diskutil list external')"; exit 1
fi
disk="${disk#/dev/}"; disk="${disk#r}"
[[ "$disk" =~ ^disk[0-9]+$ ]] || { echo "Invalid disk ID: $disk (expected e.g. disk4)"; exit 1; }
part="${disk}s1"

info="$(diskutil info "$disk")" || { echo "Disk $disk not found"; exit 1; }
pinfo="$(diskutil info "$part")" || { echo "Partition $part not found"; exit 1; }

internal=$(echo "$info" | awk -F: '/Device Location:/ {gsub(/ /,"",$2); print $2}')
removable=$(echo "$info" | awk -F: '/Removable Media:/ {gsub(/ /,"",$2); print $2}')
protocol=$(echo "$info" | awk -F: '/Protocol:/ {sub(/^ +/,"",$2); print $2; exit}')
bytes=$(echo "$info" | sed -n 's/.*Disk Size:.*(\([0-9]*\) Bytes).*/\1/p')
fstype=$(echo "$pinfo" | awk -F: '/File System Personality:/ {sub(/^ +/,"",$2); print $2}')
mountp=$(echo "$pinfo" | awk -F: '/Mount Point:/ {sub(/^ +/,"",$2); print $2}')

echo "Disk:        /dev/$disk  ($(echo "$info" | awk -F: '/Media Name:/ {sub(/^ +/,"",$2); print $2}'))"
echo "Location:    $internal   Removable: $removable   Protocol: $protocol"
echo "Size:        $bytes bytes"
echo "Partition:   /dev/$part  File system: $fstype  Mount: ${mountp:-–}"

[[ "$internal" == "External" || "$protocol" == "Secure Digital" ]] || { echo "ABORT: neither an external disk nor an SD card reader"; exit 1; }
[[ "$removable" == "Removable" ]] || { echo "ABORT: not removable media"; exit 1; }
[[ -n "$bytes" && "$bytes" -le 68719476736 ]] || { echo "ABORT: larger than 64 GB"; exit 1; }
[[ "$fstype" == "MS-DOS FAT32" ]] || { echo "ABORT: not FAT32 (exFAT never boots). Format in the camera (SDHC <= 32 GB)."; exit 1; }

tmp="$(mktemp -t vbr)"; trap 'rm -f "$tmp" "$tmp.new"' EXIT
readvbr() { sudo dd if="/dev/r$part" of="$tmp" bs=512 count=1 2>/dev/null; }

if [[ "$mode" == "status" ]]; then
    if ! dd if="/dev/r$part" of="$tmp" bs=512 count=1 2>/dev/null; then
        echo "(reading needs admin rights)"; readvbr
    fi
else
    diskutil unmountDisk "/dev/$disk" >/dev/null
    readvbr
fi

sig=$(dd if="$tmp" bs=1 skip=$((0x1FE)) count=2 2>/dev/null | xxd -p)
fat=$(dd if="$tmp" bs=1 skip=$((0x52)) count=8 2>/dev/null)
cur=$(dd if="$tmp" bs=1 skip=$((0x1E0)) count=8 2>/dev/null | tr -c '[:print:]' '.')
[[ "$sig" == "55aa" && "$fat" == "FAT32   " ]] || { echo "ABORT: sector 0 is not a FAT32 boot sector (sig=$sig type='$fat')"; exit 1; }
echo "Offset 0x1E0: '$cur'  -> $( [[ "$cur" == "BOOTDISK" ]] && echo 'marked bootable' || echo 'not marked')"
[[ "$mode" == "status" ]] && exit 0

case "$mode" in
    set|clear) ;;
    *) echo "Unknown mode: $mode"; exit 1 ;;
esac
echo
read -r -p "To write to /dev/$part, type the disk ID again ($disk): " confirm
[[ "$confirm" == "$disk" ]] || { echo "Not confirmed, nothing written."; exit 1; }

bak="$(cd "$(dirname "$0")" && pwd)/vbr_backup_${part}_$(date +%Y%m%d_%H%M%S).bin"
cp "$tmp" "$bak"; echo "Boot sector backed up: $bak"
cp "$tmp" "$tmp.new"
if [[ "$mode" == "set" ]]; then printf 'BOOTDISK'; else head -c 8 /dev/zero; fi | \
    dd of="$tmp.new" bs=1 seek=$((0x1E0)) count=8 conv=notrunc 2>/dev/null
# cmp -l: 1-based decimal byte offset; only 0x1E0..0x1E7 = 480..487 (0-based) is allowed
diffs="$(cmp -l "$tmp" "$tmp.new" || true)"   # cmp returns 1 on differences (pipefail!)
[[ -n "$diffs" ]] || { echo "Nothing to change."; exit 0; }
echo "$diffs" | awk '{ if ($1-1 < 480 || $1-1 > 487) bad=1 } END { exit bad+0 }' \
    || { echo "ABORT: change outside 0x1E0..0x1E7"; exit 1; }
sudo dd if="$tmp.new" of="/dev/r$part" bs=512 count=1 conv=notrunc 2>/dev/null
sync
readvbr
cur=$(dd if="$tmp" bs=1 skip=$((0x1E0)) count=8 2>/dev/null | tr -c '[:print:]' '.')
echo "Written. Offset 0x1E0 now: '$cur'"
diskutil mountDisk "/dev/$disk" >/dev/null || true
