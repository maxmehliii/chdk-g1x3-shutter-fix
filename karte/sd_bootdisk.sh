#!/bin/bash
# sd_bootdisk.sh - SD-Karte (FAT32) fuer den CHDK-Kartenboot der g1x3 markieren (macOS)
#
#   ./sd_bootdisk.sh status <diskN>   nur lesen: zeigt Dateisystem und ob "BOOTDISK" gesetzt ist
#   ./sd_bootdisk.sh set    <diskN>   schreibt "BOOTDISK" an Offset 0x1E0 des FAT32-Bootsektors (sudo)
#   ./sd_bootdisk.sh clear  <diskN>   loescht die Markierung wieder (8 Nullbytes, sudo)
#
# Beleg (g1x3-Dump GM1.00A, NOTES.md Prompt 5):
#   FUN_e03a039e prueft bei FAT32 strncmp(bootsektor+0x1E0,"BOOTDISK",8), bei FAT12/16 +0x40.
#   exFAT wird nie als bootfaehig erkannt. -> Karte muss FAT32 sein (SDHC bis 32 GB, in der Kamera formatiert).
#   Zusaetzlich startet Diskboot nur, wenn der Schreibschutzschieber der Karte auf LOCK steht (PhySwBootSD.c).
#
# Sicherheit: Das Skript schreibt NUR 8 Byte in Sektor 0 der Partition s1 und nur nach Pruefung,
# dass es eine externe, wechselbare Karte <= 64 GB mit FAT32 ist und du die Disk-ID nochmal eintippst.
set -euo pipefail

mode="${1:-}"; disk="${2:-}"
if [[ -z "$mode" || -z "$disk" ]]; then
    echo "Aufruf: $0 status|set|clear diskN   (diskN aus 'diskutil list external')"; exit 1
fi
disk="${disk#/dev/}"; disk="${disk#r}"
[[ "$disk" =~ ^disk[0-9]+$ ]] || { echo "Ungueltige Disk-ID: $disk (erwartet z.B. disk4)"; exit 1; }
part="${disk}s1"

info="$(diskutil info "$disk")" || { echo "Disk $disk nicht gefunden"; exit 1; }
pinfo="$(diskutil info "$part")" || { echo "Partition $part nicht gefunden"; exit 1; }

internal=$(echo "$info" | awk -F: '/Device Location:/ {gsub(/ /,"",$2); print $2}')
removable=$(echo "$info" | awk -F: '/Removable Media:/ {gsub(/ /,"",$2); print $2}')
protocol=$(echo "$info" | awk -F: '/Protocol:/ {sub(/^ +/,"",$2); print $2; exit}')
bytes=$(echo "$info" | sed -n 's/.*Disk Size:.*(\([0-9]*\) Bytes).*/\1/p')
fstype=$(echo "$pinfo" | awk -F: '/File System Personality:/ {sub(/^ +/,"",$2); print $2}')
mountp=$(echo "$pinfo" | awk -F: '/Mount Point:/ {sub(/^ +/,"",$2); print $2}')

echo "Disk:        /dev/$disk  ($(echo "$info" | awk -F: '/Media Name:/ {sub(/^ +/,"",$2); print $2}'))"
echo "Ort:         $internal   Wechselmedium: $removable   Protokoll: $protocol"
echo "Groesse:     $bytes Bytes"
echo "Partition:   /dev/$part  Dateisystem: $fstype  Mount: ${mountp:-–}"

[[ "$internal" == "External" || "$protocol" == "Secure Digital" ]] || { echo "ABBRUCH: weder externe Disk noch SD-Kartenleser"; exit 1; }
[[ "$removable" == "Removable" ]] || { echo "ABBRUCH: kein Wechselmedium"; exit 1; }
[[ -n "$bytes" && "$bytes" -le 68719476736 ]] || { echo "ABBRUCH: groesser als 64 GB"; exit 1; }
[[ "$fstype" == "MS-DOS FAT32" ]] || { echo "ABBRUCH: nicht FAT32 (exFAT bootet nie). In der Kamera formatieren (SDHC <= 32 GB)."; exit 1; }

tmp="$(mktemp -t vbr)"; trap 'rm -f "$tmp" "$tmp.new"' EXIT
readvbr() { sudo dd if="/dev/r$part" of="$tmp" bs=512 count=1 2>/dev/null; }

if [[ "$mode" == "status" ]]; then
    if ! dd if="/dev/r$part" of="$tmp" bs=512 count=1 2>/dev/null; then
        echo "(Lesen braucht Admin-Rechte)"; readvbr
    fi
else
    diskutil unmountDisk "/dev/$disk" >/dev/null
    readvbr
fi

sig=$(dd if="$tmp" bs=1 skip=$((0x1FE)) count=2 2>/dev/null | xxd -p)
fat=$(dd if="$tmp" bs=1 skip=$((0x52)) count=8 2>/dev/null)
cur=$(dd if="$tmp" bs=1 skip=$((0x1E0)) count=8 2>/dev/null | tr -c '[:print:]' '.')
[[ "$sig" == "55aa" && "$fat" == "FAT32   " ]] || { echo "ABBRUCH: Sektor 0 ist kein FAT32-Bootsektor (sig=$sig typ='$fat')"; exit 1; }
echo "Offset 0x1E0: '$cur'  -> $( [[ "$cur" == "BOOTDISK" ]] && echo 'bootfaehig markiert' || echo 'nicht markiert')"
[[ "$mode" == "status" ]] && exit 0

case "$mode" in
    set|clear) ;;
    *) echo "Unbekannter Modus: $mode"; exit 1 ;;
esac
echo
read -r -p "Zum Schreiben auf /dev/$part die Disk-ID nochmal eintippen ($disk): " confirm
[[ "$confirm" == "$disk" ]] || { echo "Nicht bestaetigt, nichts geschrieben."; exit 1; }

bak="$(cd "$(dirname "$0")" && pwd)/vbr_backup_${part}_$(date +%Y%m%d_%H%M%S).bin"
cp "$tmp" "$bak"; echo "Bootsektor gesichert: $bak"
cp "$tmp" "$tmp.new"
if [[ "$mode" == "set" ]]; then printf 'BOOTDISK'; else head -c 8 /dev/zero; fi | \
    dd of="$tmp.new" bs=1 seek=$((0x1E0)) count=8 conv=notrunc 2>/dev/null
# cmp -l: 1-basierter Byte-Offset dezimal; erlaubt nur 0x1E0..0x1E7 = 480..487 (0-basiert)
diffs="$(cmp -l "$tmp" "$tmp.new" || true)"   # cmp liefert Code 1 bei Unterschieden (pipefail!)
[[ -n "$diffs" ]] || { echo "Nichts zu aendern."; exit 0; }
echo "$diffs" | awk '{ if ($1-1 < 480 || $1-1 > 487) bad=1 } END { exit bad+0 }' \
    || { echo "ABBRUCH: Aenderung ausserhalb 0x1E0..0x1E7"; exit 1; }
sudo dd if="$tmp.new" of="/dev/r$part" bs=512 count=1 conv=notrunc 2>/dev/null
sync
readvbr
cur=$(dd if="$tmp" bs=1 skip=$((0x1E0)) count=8 2>/dev/null | tr -c '[:print:]' '.')
echo "Geschrieben. Offset 0x1E0 jetzt: '$cur'"
diskutil mountDisk "/dev/$disk" >/dev/null || true
