# Canon PowerShot G1 X Mark III: shutter-button fix via CHDK (S2 on Q/SET)

## What this is

The second contact of my G1 X Mark III's shutter button broke: the full press (S2).
The half press (S1, focus and exposure) still worked, but the camera could no longer take pictures.

This project replaces the broken contact with another button:

**Hold the shutter button half-way, then press Q/SET → the camera takes the picture.**

Everything else stays exactly like Canon's firmware:
- Q/SET on its own opens the Q menu as always.
- Burst shooting and video work like with a real shutter button.
- There is no visible CHDK: no menu, no ALT mode, no on-screen overlays.

Technically it is a **minimal port of [CHDK](https://chdk.fandom.com/wiki/CHDK)**.
- The camera loads it from the SD card into RAM every time it is switched on.
- Nothing is written to flash, not even settings or calibration data.
- Without the prepared card the camera starts normally with Canon's firmware.

As an extra, a **custom start-up image** can replace the Canon logo at power-on (also RAM only).

## Controls

| Situation | What the camera sees |
|---|---|
| Q/SET on its own | Q/SET as usual (Q menu) |
| Shutter half-pressed (at least ~50 ms), then Q/SET | half **and full** press → photo. Q/SET is hidden from Canon |
| Q/SET held, shutter still half-pressed | full press stays (burst, video like with a real shutter button) |
| Q/SET released | full press released, half press as physically pressed |
| Shutter released first | both released. Q/SET stays hidden until it is released (no Q menu after the photo) |
| Q/SET first, then half-press | **no shot**. Q/SET already went to Canon (Q menu) |
| all other buttons | unchanged |

**Power-on:**
- Booting from the card, the camera starts in **playback mode**. A half press switches to shooting mode as usual.
- To start directly in **shooting mode** (lens extends), **hold the power button until the start-up image appears**.

## Quick start without building

Ready-made files are on the **[Releases](https://github.com/MaxTechSupport/chdk-g1x3-shutter-fix/releases/latest)** page:

| File | Contents |
|---|---|
| `g1x3_GM1.00A_final.zip` | shutter fix only, Canon's start-up image stays. **Tested on the camera.** |
| `g1x3_GM1.00A_splash_example.zip` | plus an example start-up image (an aperture). The code is the same as in the tested start-up-image build, only the picture differs |

Each ZIP contains `DISKBOOT.BIN` and short instructions (`README.txt`). Checksums are in `SHA256SUMS.txt`.

1. **Check the firmware:** G1 X Mark III with firmware GM1.00A only (camera menu shows version 1.0.0).
2. **Format the card:** format an SD card (SDHC up to 32 GB) in the camera. This erases everything. It must be FAT32, exFAT does not work.
3. **Mark the card bootable:** write "BOOTDISK" into the boot sector, see [Prepare the SD card](#3-prepare-the-sd-card).
4. **Copy the file:** `DISKBOOT.BIN` from the ZIP into the root folder of the card.
5. **Eject the card and set its write-protect slider to LOCK.**
   The camera only runs code from the card when it is locked. Photos and videos are still saved normally.
6. **Switch on and shoot:** insert the card, switch on, then half-press + Q/SET = photo. See [Controls](#controls).
   - Start-up image version: set the camera menu **Start-up Image** to **2**.

## Requirements and safety

- **PowerShot G1 X Mark III with firmware GM1.00A only.**
  - The loader checks the platform ID and the firmware string at start.
  - If either does not match, the lamp next to the screen blinks forever and CHDK does not start. Then remove the battery.
  - Do not update the camera firmware, otherwise the addresses no longer match.
- **RAM only.** The port calls no function that writes flash, persistent parameters or calibration data.
  The list of these functions is in [docs/NOTES_de.md](docs/NOTES_de.md) (Prompt 4).
- **Way back:** remove the battery, insert a normal card (or set the write-protect slider to "unlocked"). Canon's firmware then starts as always.
- Use at your own risk. This project has nothing to do with Canon. It has been tested on **one** camera.

## Building it yourself, step by step

### 1. Tools

- macOS or Linux, `git`, `svn` (Subversion), Docker
  - macOS: `brew install subversion colima docker`, then `colima start`
- For custom start-up images also Python 3 with `pip install pillow numpy opencv-python`

### 2. Get CHDK and build

```bash
git clone https://github.com/MaxTechSupport/chdk-g1x3-shutter-fix.git && cd chdk-g1x3-shutter-fix
./setup.sh            # checks out CHDK trunk r6358 into ./chdk and builds the Docker image "chdkbuild"
./build.sh final      # → out/DISKBOOT.BIN (shutter fix only, Canon's start-up image stays)
```

You do **not** need a firmware dump to build. All addresses are already in `port/`.

The build with CHDK r6358 is reproducible. My `final` build, tested on the camera, has the SHA-256
`35bf072cbedc9b6f74b221f0deed93d7eac9b5cca554f72acb004c974b7306e6`. Your `out/DISKBOOT.BIN` should have the same checksum.

### 3. Prepare the SD card

The camera only runs code from the card under three conditions:

1. **FAT32.** exFAT is never recognized as bootable.
   - SDHC cards up to 32 GB can be formatted in the camera.
   - Format larger cards on a computer as FAT32 with MBR.
2. **"BOOTDISK" at offset 0x1E0 of the FAT32 boot sector.** On macOS, `sdcard/sd_bootdisk.sh` does this.
   It first checks that it really is an external FAT32 SD card, and backs up the boot sector:
   ```bash
   diskutil list external                # find the card's disk ID, e.g. disk4
   ./sdcard/sd_bootdisk.sh status disk4
   ./sdcard/sd_bootdisk.sh set disk4     # asks for sudo and for the disk ID again
   ```
   On Linux, write the 8 bytes `BOOTDISK` at offset 0x1E0 of the partition (not the whole device),
   e.g. with `printf BOOTDISK | sudo dd of=/dev/sdX1 bs=1 seek=480 conv=notrunc`. Double-check the device first!
   On Windows there are tools that make SD cards bootable for CHDK (e.g. EOScard). I have not tested them.
3. **Copy `out/DISKBOOT.BIN` into the root folder of the card**, eject it and set the **write-protect slider to LOCK**.

The port clears the write-protect bit in the button scan, so Canon saves photos and videos normally despite LOCK.
**Do not format this card in the camera afterwards.** Otherwise "BOOTDISK" is gone, and cards over 32 GB become exFAT.

### 4. Test

1. Insert the card, switch on. The camera starts normally; booting from the card it starts in playback mode.
2. Half-press → shooting mode. Keep it half-pressed and press Q/SET → photo.
3. Q/SET on its own → Q menu.

If anything hangs: remove the battery, insert a normal card.

## Custom start-up image (optional)

Canon's start-up image is stored in ROM. At power-on the firmware copies it into a RAM buffer and shows it from there.
The `splash` build replaces only this RAM buffer, and only if it really contains Canon's image.
On the next start without the card everything is original again.

```bash
python3 splash/make_splash.py my_image.png                      # black background, fitted automatically
python3 splash/make_splash.py my_image.png --background ffffff --width 400
./build.sh splash
```

**Important:** set the camera menu **Start-up Image** to **2**.
- Only for setting 2 does Canon tell the JPEG decoder the full buffer size of 12179 bytes. For setting 1 it is just 5182 bytes.
- If the setting is 1, the Canon logo simply appears. That is harmless.

**Tips:**
- The JPEG can be at most ~12 KB. The script automatically picks the best quality that still fits.
- Flat colors and clean lines look much better than photos.
- A black background with a bright subject needs the fewest bytes.
- `splash/example.png` (an aperture) is the default image.

**Canon lettering below the image:**
- It is not in the repository. The script can take it from your **own** firmware dump:
  `--logo-from-dump PRIMARY.BIN`.
- The dump is made with the Canon Basic script from the CHDK wiki
  ([Canon Basic/Scripts/Dumper](https://chdk.fandom.com/wiki/Canon_Basic/Scripts/Dumper)).

## How it works

This is a summary. All details, with evidence from the dump, are in the (German) development log [docs/NOTES_de.md](docs/NOTES_de.md).

- **Card boot:** the firmware starts `DISKBOOT.BIN` from a FAT32 card with "BOOTDISK" and the write-protect slider on LOCK.
  - The file is encoded with dancingbits version 17.
  - The loader (`port/loader/g1x3`) checks camera and firmware and copies a small core into RAM.
  - This core restarts Canon's firmware with a few hooked-in changes. The big CHDK core is not used here.
- **S1/S2 on the G1 X Mark III:** the two contacts do not go through the normal button scan `physw_status`.
  They go through interrupts and a dedicated task `PhySw_ReleaseSw` with its own state machine.
  - The port replaces this task with a copy that understands one extra message, "press virtual S2".
  - While the virtual S2 is active, the copy keeps the S2 level at "pressed".
- **Button logic** (`kbd_d.c`): runs in a copy of the button task `PhySw`, right after the firmware has read the buttons.
  - When it sees a Q/SET press while S1 is held stable, it hides Q/SET from Canon and switches on the virtual S2.
  - Releasing undoes everything.
- **Write protection:** card boot requires LOCK, so the port clears the bit through which Canon sees the slider.
- **Start-up image:** see above. In addition the port records in the firmware's RAM state that image set 2 is already in the buffer.
  Otherwise Canon would refill the buffer from ROM before showing it.
- **Developed in stages**, each tested on the camera before the next began (`port/g1x3_stage_*`):
  - a/b: load and hand over cleanly to Canon
  - c/c2: only read the buttons and log them to the card (example logs in `docs/logs/`)
  - d: change the buttons
  - The log texts of stages c to d were translated to English after testing. Those builds differ from the tested ones only in these strings (stage a/b is bit-identical).
  - The starting point was the CHDK port for the G7 X Mark II. Every address was re-verified in my own G1 X III dump.

## Folders

| Folder | Contents |
|---|---|
| `port/` | the g1x3 port. `build.sh` copies it into the CHDK tree `./chdk` |
| `port/g1x3_final/` | final version: shutter fix only |
| `port/g1x3_splash/` | final version + custom start-up image |
| `port/g1x3_stage_*/` | test stages from development |
| `port/loader/g1x3/`, `port/platform/g1x3/` | loader and platform files (stubs, function lists) |
| `splash/` | start-up image script and example image |
| `sdcard/` | script that marks the SD card bootable (macOS) |
| `docs/` | development log (German) with all address lists, button logs |

## License and thanks

- **License:** GPL-2.0, like CHDK (see `LICENSE`).
- **Thanks** to the CHDK project and to the authors of the G7 X Mark II port, which served as the template.
- **Not included:** Canon firmware or parts of it as files (dump, images). "Canon" and "PowerShot" are trademarks of Canon Inc.
