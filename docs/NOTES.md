# Entwicklungsprotokoll – CHDK-Minimalport G1 X Mark III (g1x3)

> Originalprotokoll der Entwicklung (deutsch, chronologisch, mit allen Belegen und Adresslisten).
> „Prompt N“ = Arbeitsschritt. Erwähnte private Startbilder (z. B. Snoopy) sind nicht Teil dieses Repositorys.

## Prompt 1 – Umgebung und Dump prüfen (2026-11-12)

### Befund: Projektordner war leer
Der Projektordner enthielt beim Start keine Dateien:
kein `CLAUDE.md`, kein `dumps/`, kein CHDK-Source. Eine Spotlight-Suche über die ganze
Platte nach `PRIMARY.BIN`, `CBDUMPER.LOG` und `finsig_thumb2.c` hat nichts gefunden.
Außer `Macintosh HD` ist kein Volume eingehängt.

### Toolchain (lokal gemessen)
| Tool | Gefunden | Version | Bewertung |
|---|---|---|---|
| arm-none-eabi-gcc | /opt/homebrew/bin | 16.1.0 (Homebrew) | **passt nicht**: CHDK erlaubt nur gcc 4, 5, 8–11 (arm_rules.inc:22) |
| make | /usr/bin/make | GNU Make 3.81 (macOS) | alt, reicht für CHDK vermutlich; **nicht verifiziert** |
| capstone (C-Lib) | Homebrew | 5.0.3 | CHDK-Docker nutzt 4.0.2 → **passt nicht** |
| capstone (Python) | Anaconda | 5.0.7 | für CHDK nicht relevant (finsig_thumb2 linkt die C-Lib) |
| docker | /opt/homebrew/bin | Daemon läuft nicht | Docker-Weg wäre möglich, sobald Docker Desktop gestartet ist |
| Ghidra (CLI) | nicht im PATH | – | – |

### Ghidra-MCP
Kein Ghidra-MCP-Server in dieser Session registriert (Tool-Suche nach "ghidra": kein Treffer).
→ **nicht erreichbar**.

### Setup-Nachtrag (2026-11-12)
- **JDK 21** installiert: Homebrew `openjdk@21` 21.0.13.
- **Subversion** 1.15.0, **uv** 0.13.7 installiert (Homebrew).
- **Ghidra 12.1.4** in `tools/ghidra_12.1.4_PUBLIC`.
  SHA-256 `ddac49f9…d2d4db` = Release-Seite ✔.
  Nicht 12.2: GhidraMCP 2.0 ist fest auf 12.1.4 gebaut (`extension.properties: version=12.1.4`).
  - Native Decompiler für mac_arm_64 selbst gebaut (`support/gradle/gradlew buildNatives`).
    Binaries liegen in `Ghidra/Features/Decompiler/build/os/mac_arm_64/`.
  - `support/launch.properties`: `JAVA_HOME_OVERRIDE` zeigt auf openjdk@21.
  - Start: `tools/ghidra_12.1.4_PUBLIC/ghidraRun`.
- **GhidraMCP 2.0** (LaurieWired), SHA-256 Release-Zip `ebf904fd…5f2e72`.
  - Ghidra-Extension entpackt nach `~/Library/ghidra/ghidra_12.1.4_PUBLIC/Extensions/GhidraMCP`.
    Plugin muss im CodeBrowser unter File → Configure → Developer aktiviert werden.
    HTTP-Backend: 127.0.0.1:8080.
  - Python-Server: Wheel liegt in `tools/GhidraMCP-release-2-0/`, ist bei Claude Code **noch nicht registriert**.
    Der Testlauf wurde vom Auto-Modus blockiert, weil er Fremdcode ausführt.
- **CHDK-Source: noch nicht vorhanden.** Anonymes SVN bei Assembla ist seit 2022 abgeschaltet
  (`svn info` → E215004, Anmeldung nötig).
  Das CHDK-Wiki empfiehlt einen beliebigen Login (z. B. guest/guest). Den gibt der Nutzer selbst ein.
  Der einzige Git-Mirror (Anda2012/CHDK) ist inoffiziell (1 Stern, Upload von Hand) und wird bewusst nicht genutzt.
- Speicherplatz: nur noch ca. 9 GB frei.

### Dump g1x3 (2026-11-12, selbst erstellt)
Erstellt mit dem „Improved universal dumper“ (Canon Basic, CHDK-Wiki), Karte als FAT32-Script-Disk.
Dateien: `dumps/g1x3/PRIMARY.BIN`, `dumps/g1x3/CBDUMPER.LOG`.
Die Kopie wurde per `cmp` mit der Datei auf der Karte verglichen und ist identisch.

| Prüfung | Ergebnis | Status |
|---|---|---|
| CBDUMPER.LOG | `Started` / `E0000000 dry` / `done` | verifiziert |
| Größe | 33554432 = 0x2000000 Bytes | verifiziert |
| SHA-256 | `558e8448bec69624cfcde5fa25b17b1cc2f4b1cce5ba62248479f143c936bd6c` | verifiziert |
| "gaonisoy" @ 0x20004 | ja (davor bei 0x20000 der Thumb-2-Sprung `00f0 04b8`) | verifiziert |
| ROM-Basis | 0xE0000000 (aus dem Log; das Layout entspricht DIGIC 7) | verifiziert |
| Firmware | `Firmware Ver GM1.00A` @ 0x5546B0, `Firmware Version 1.00` @ 0x554694 → CHDK-Unterverzeichnis vermutlich `100a` | verifiziert (String) |
| Build-Datum | `Sep  7 2017` `09:48:01` @ 0x554678, steht direkt neben dem Versionsstring | verifiziert (String) |
| Modell | `Canon PowerShot G1 X Mark III` @ 0x1F20416, Kurzname `G1Xm3` | verifiziert (String) |
| DryOS | `DRYOS version 2.3, release #0060+p1` @ 0x1A0E8 und 0x637660 | verifiziert (String) |
| weitere DryOS-Strings | `#0059+p4` @ 0xE78598, `#0059+p3` @ 0xF5257F, gehören wohl zu Nebenprozessor-/WLAN-Blobs | vermutet |
| RomStarter | `#RomStarter Ver1.06 for EC418` @ 0x5BF8 | verifiziert (String) |
| FIRMINFO.TXT | wird von diesem Dumper nicht erzeugt, ist für CHDK nicht nötig | – |

Auffällig: Ab etwa 0x1F20000 und 0x1F60000 (= 0xE1F2xxxx/0xE1F6xxxx) stehen viele Kopien
von Parameterblöcken mit Modellname, `*DataLost*` und `UITimer`.
Das ist sehr wahrscheinlich der beschreibbare Einstellungsbereich im Flash.
→ Für Prompt 4 (Flash-Schreiber meiden) vormerken.

Hinweis: Auf der Kamera läuft die Firmware 1.00A, also die Auslieferungsversion.
Die Firmware jetzt **nicht** aktualisieren, sonst passt der Port nicht mehr zur Kamera.

### Dump g7x2 1.10B (Referenz, CHDK-Community)
Quelle: CHDK-Dump-Sammlung auf Google Drive (P&S FW Dumps → G Series → `g7x2_110b.7z`),
Dump von philmoz. Verlinkt auf der CHDK-Wiki-Seite „For Developers“.
Nach `dumps/g7x2/` entpackt wurden nur `PRIMARY.BIN` und `Info.txt`; `Strings.txt` wurde weggelassen.

| Prüfung | g1x3 (eigener Dump) | g7x2 (Referenz) |
|---|---|---|
| Größe | 0x2000000 | 0x2000000 |
| SHA-256 | `558e8448…c936bd6c` | `397c6889…42e96bef` |
| ROM-Basis | 0xE0000000 | 0xE0000000 (laut Info.txt) |
| "gaonisoy" @ 0x20004 | ja | ja |
| Reset-Vektor @ 0x0 | `3400 9fe5 10ff 2fe1` | identisch |
| Firmware | GM1.00A | GM1.10B |
| Build-Datum | Sep 7 2017 | Aug 17 2020 |
| DryOS (Hauptstring) | 2.3 release **#0060+p1** | 2.3 release **#0058+p10** |
| RomStarter | Ver1.06 for EC418 | Ver2.14 for EC397 |
| Modellstring | @ 0x1F20416 | @ 0x1F20410 |
| Belegte MB-Blöcke | ohne 20 und 28 | ohne 17 und 26–28 |

Grobe Einschätzung:
- **Layout:** sehr ähnlich. Beide haben dieselbe ROM-Basis, dieselbe Größe und denselben Reset-Vektor.
  Der Thumb-2-Einstieg bei 0x20000 und die Kopierschleifen ab etwa 0x20040 haben dieselbe Struktur.
  Der Parameterbereich bei 0x1F2xxxx liegt in beiden Dumps fast an derselben Stelle.
  → Beide sind DIGIC-7-Firmwares. Der Ansatz „g7x2 als Vorlage“ ist plausibel. (vermutet, grob verglichen)
- **DryOS: nicht dieselbe Version.** g1x3 hat r60, g7x2 hat r58. Funktionen und Strukturoffsets
  (Task-Struktur, Event-Procs usw.) können sich zwischen r58 und r60 verschoben haben.
  Für jeden Stub ist also ein eigener Beleg nötig. (verifiziert: Strings)
- **Startcode:** g1x3 hat bei 0x2002A–0x2003F einen zusätzlichen Block, den g7x2 nicht hat.
  Der Block liest ein Wort über eine per movw/movt geladene Adresse und setzt es auf 1, wenn es 0 ist.
  Wozu, ist offen. Klärung in Ghidra (Prompt 4).
- **RomStarter:** Version und Chip-Kennung unterscheiden sich (EC418 bzw. EC397). Möglicherweise ist es eine andere Bootloader- oder SoC-Variante.
  Für CHDK zählt vor allem, ob der Bootpfad über DISKBOOT/FI2 gleich funktioniert.
  → Muss in Prompt 2/3 geprüft werden.
- Ein byteweiser Vergleich ist wegen verschobenem Code nicht aussagekräftig (69 % abweichende Bytes).

### Verifiziert
- Toolchain-Versionen oben (gcc 16.1.0, make 3.81, capstone 5.0.3).
- g1x3-Dump: siehe Tabelle oben.
- g7x2-Dump: gültig (Größe, Signatur, Basis); Vergleich siehe oben.

### CHDK-Source (2026-11-12)
Trunk **r6358** per svn (guest-Login) nach `chdk/` ausgecheckt, 207 MB.
`platform/g7x2` mit den Unterordnern 101a, 101b, **110b** und 120a ist vorhanden, dazu `loader/g7x2`.
Ein `platform/g1x3` gibt es nicht, und in `camera_list.csv` taucht g1x3 nicht auf.

**Toolchain-Anforderungen laut Source:**
- `arm_rules.inc:21-24`: Der Build bricht ab, wenn die gcc-Hauptversion nicht 4, 5, 8, 9, 10 oder 11 ist.
  Er lässt sich nur mit `DISABLE_GCC_VERSION_CHECK=1` umgehen („if you know what you are doing“).
  `arm_rules.inc:27-31`: Ab gcc 10 wird `-fno-tree-loop-distribute-patterns` gesetzt, weil Builds sonst
  auf manchen Kameras nicht booten (Forum topic=14281).
  → **gcc 16.1.0 (Homebrew) passt nicht.** Die Prüfung abzuschalten, wäre bei einem neuen Port unnötig riskant.
- capstone: `tools/docker/chdkbuild/Dockerfile` baut **capstone 4.0.2** (nur ARM, statisch).
  Gesteuert wird das über `OPT_CAPSTONE_TOOLS=1`, `CAPSTONE_TOOLS_INC` und `CAPSTONE_TOOLS_LINK` (siehe `tools/Makefile:10`).
  → **capstone 5.0.3 (Homebrew) passt nicht.**
- Offizieller Docker-Weg (`tools/docker/README.TXT`): Debian stretch, **arm-none-eabi-gcc 5.4.1**,
  host-gcc 6.3, capstone 4.0.2. Das Image ist etwa 330 MB groß.
  Der Source wird nach `/srv/src` gemountet, die Dumps optional per `PRIMARY_ROOT`.

**Empfehlung:** den Docker-Weg nehmen. Er ist vom CHDK-Maintainer gepflegt und deckt gcc und capstone mit einer Lösung ab.
Voraussetzung ist, dass Docker Desktop läuft. Das tut es aktuell nicht.

**g7x2-Port (makefile.inc, Kurzfassung für Prompt 2):** PLATFORMID=12988 (0x32BC), DryOS 58, DIGIC 70,
ROMBASEADDR=0xE0000000, NEED_ENCODED_DISKBOOT=16, KEYSYS=d4i, FI2FLAGS=W,
PLATFORMID-Speicherort 0xE1F20270.

**Plattform-ID g1x3:** An derselben Stelle (Dump-Offset 0x1F20270) steht `d7 32` → **0x32D7 = 13015**.
Gegenprobe: In g7x2 steht dort `bc 32` = 12988, das stimmt mit dem makefile überein.
Der Kontext davor (`0000 00e0 0000 0002` = ROM-Basis und ROM-Größe) ist in beiden Dumps identisch.
Status: **vermutet**, bis der Sigfinder es in Prompt 3 bestätigt.

### Build-Umgebung (2026-11-12, eingerichtet)
- Docker-Engine: **Colima** 0.10.3 (Homebrew), VM vz/aarch64, 2 CPU, 2 GB RAM, 15 GB Disk.
  Docker-Kontext `colima`. Nach einem Neustart: `colima start`.
- Image `chdkbuild:latest` (406 MB) aus `chdk/tools/docker/chdkbuild/Dockerfile`.
  Die SHA-256-Prüfung des capstone-Tarballs im Dockerfile ist bestanden.
  - arm-none-eabi-gcc **5.4.1** 20160919, host-gcc 6.3.0, GNU Make 4.1
  - capstone **4** (CS_API_MAJOR 4), statisch, `OPT_CAPSTONE_TOOLS=1`
- Wrapper: `./chdkmake.sh <befehl>` mountet `chdk/` → `/srv/src` und `dumps/` → `/srv/dumps` (nur lesend).
  Achtung: Das Dump-Layout passt noch nicht zu `PRIMARY_ROOT`. CHDK erwartet
  `$PRIMARY_ROOT/<platform>/sub/<fw>/PRIMARY.BIN`. Wird in Prompt 3 angelegt.
- Rauchtest: `make PLATFORM=g7x2 PLATFORMSUB=110b fir` → „Firmware creation completed successfully“,
  dazu `make -C tools finsig_thumb2 capdis` → beide gebaut. **Die Toolchain funktioniert.**
  Die g7x2-Ausgabe (`bin/DISKBOOT.BIN`) wurde danach gelöscht, damit sie nicht versehentlich auf die g1x3-Karte kommt.
  Die svn-Arbeitskopie ist unverändert (`svn status` leer).

### Fehlt / offen
- ~~CLAUDE.md~~ → angelegt und vom Nutzer bestätigt (2026-11-12)
- ~~Build-Umgebung~~ → erledigt (Colima + chdkbuild)
- Ghidra läuft noch nicht. Die MCP-Tools sind geladen, das Backend auf :8080 ist aber nicht erreichbar.
  Wird erst für Prompt 4 gebraucht.

---

## Prompt 2 – Analyse des g7x2-Ports (2026-11-12)
Quelle: CHDK-Trunk r6358, `platform/g7x2`, `platform/g7x2/sub/110b`, `loader/g7x2`, `core/kbd_common.c`.
Alle Adressen in diesem Abschnitt sind **g7x2-1.10B-Adressen**. Sie gelten nicht für g1x3 (Regel 3).

### 1. Bootweg
- **Diskboot (DISKBOOT.BIN)** ist der Standardweg. `makefile.inc`: `NEED_ENCODED_DISKBOOT=16`.
  `Makefile:114-119` kodiert `main.bin` mit `tools/dancingbits` (Version 16) zu `DISKBOOT.BIN`.
  Laut Kommentar in `makefile.inc` wurde die Version 16 „Found @e06c2db4“ in der Firmware gefunden.
- **FI2 (PS.FI2, Menü „Firm Update“)** ist nur möglich mit `OPT_FI2` + `FI2KEY`/`FI2IV`.
  Die Schlüssel stehen **nicht** im Source (`Makefile:75-79`, siehe `platform/fi2.inc.txt`).
  `KEYSYS=d4i` und `FI2FLAGS=W` betreffen nur FI2. → **Für den Minimalport nicht nötig.**
- Der Kartenboot setzt die Bootmarkierung auf der Karte voraus (CHDK-Kartenvorbereitung, Details in Prompt 5).
- Ablauf: Der Loader wird auf `MEMBASEADDR` gelinkt: 0x8000 bei THUMB_FW (`makefile_cam.inc:19-24`, THUMB_FW über `makefile_base.inc:59-61`). Die Ladeadresse des Canon-Bootloaders ist nicht separat geprüft. `loader/g7x2/entry.S` setzt `SP=0x2000` und springt nach `my_restart()`.
  1. `get_core_id()` (`mrc p15,0,r0,c0,c0,5 & 0xf`). **Nur Core 0** macht `check_compat()`,
     kopiert den CHDK-Core nach `MEMISOSTART` (110b: 0x2C460C) und schaltet die grüne LED ein (`*0xD20801E4 = 0x024D0002`).
  2. Firmwarespezifisch: Der Loader prüft das Befehlsmuster `0xB5102101` an einer festen ROM-Adresse,
     um die Version zu erkennen. Dann **„unblock core1“** über `sub_e051e078` (1.10B), aber nur auf Core 0.
  3. Auf **beiden** Cores: `dcache_clean_range` (`0xE042EB75`) und `icache_flush_range` (`0xE042EC4D`)
     über den kopierten Bereich, dann Sprung nach `MEMISOSTART+1` (Thumb).
- `boot()` in `sub/110b/boot.c` ist eine Kopie der Firmware-Bootroutine @0xE0020011:
  - Core 1 → `sub_e0539e64` („park core1“), danach weiter bei loc_boot.
  - Kopiert Daten/BSS sowie den Kernel-Code nach **0xDFFC4900–0xDFFD0908** (RAM-Kopie des DryOS-Kerns, Quelle ROM 0xE1019164).
  - `patch_mzrm_sendmsg` (UI-Overlay-Hook, nur für CHDK-OSD).
  - **Installiert den CreateTask-Hook**: Er schreibt 8 Byte (`ldr.w pc,[…]` + Adresse `CreateTask_my`)
    nach `hook_CreateTask` = 0xDFFC93BA (in der RAM-Kopie) und macht danach dcache clean + icache flush.
  - `sub_e00200f8_my`: Setzt den Start des Canon-Heaps auf `new_sa` (= Ende von CHDK).
    **Dadurch reserviert CHDK seinen Speicher.** Ausserdem wird der Start-Modus (Play/Rec) aus GPIO `0xD2082168` Bit 0x10000 nach `0x9914+8` geschrieben.
  - Kette weiter: `sub_e0020398_my` → `sub_e0020750_my` (unblock core1 zweimal, nur Core 0) → `sub_e005b418_my`
    (Einschaltprüfung, Abbruchzweig auskommentiert) → `CreateTask("Startup", task_Startup_my)`.
  - `task_Startup_my`: Original-Startup ohne `sub_e046e3dc` („diskboot“, sonst Bootschleife),
    mit SD-UHS-Fix. Dort ersetzt **`sub_e005b33e_my` den PhySw-Task durch `mykbd_task`** (Stack 0x800 → 0x2000),
    dazu kommen `CreateTask_spytask` und `init_required_fw_features`.

### 2. Gehookte Tasks und warum
| Hook | Weg | Zweck | Minimal nötig? |
|---|---|---|---|
| PhySw → `mykbd_task` | Startkette (`sub_e005b33e_my`) | Tastenabfrage über CHDK | **ja** |
| TricInitTask → `task_TricInitTask_my` | CreateTask-Hook | **Dual-Core-Fix**: Läuft Core 1 schon (`*0xD2050074 != 0`), wird das SmacIdleCmp-Eventflag (0x80) selbst gesetzt, sonst wartet die Firmware ewig | **ja** (vermutet, zu prüfen) |
| InitFileModules → `init_file_modules_task` | CreateTask-Hook | setzt `core_spytask_can_start` (Karte gemountet) | ja, falls Spytask oder Datei-I/O genutzt wird |
| CaptSeq → `capt_seq_task` | CreateTask-Hook | RAW, Override von Belichtungsparametern | nein |
| ExpDrv → `exp_drv_task` | CreateTask-Hook | Belichtungs-Override | nein |
| FileWrite → `filewritetask` | CreateTask-Hook | Remote-Capture | nein |
| FsIoNotifyTask → `_my` | CreateTask-Hook | Crash-Fix für Nicht-Canon-Dateien in A/DCIM | nein (Logger schreibt nicht in DCIM) |
| mzrm_sendmsg-Logging | Funktionszeiger | CHDK-UI-Overlay | nein |
| Spytask (neu) | `CreateTask_spytask` | CHDK-Core (Konfiguration, GUI, Module) | Für Stufe a/b vielleicht nicht nötig. Für den Logger (Stufe c) ist ein Task mit Datei-I/O ausserhalb von PhySw sinnvoll |

**Absolutes Minimum** (CHDK lädt, Canon läuft normal weiter, kbd-Hook aktiv):
Loader (Core-Erkennung, Kopie, unblock core1, Cache-Operationen) → `boot()` mit Kernel-RAM-Kopie
und CreateTask-Hook (nur noch für TricInit) → Heap verschieben → Startkette → `task_Startup_my` mit
PhySw-Ersatz. Im Ersatz-Task: `kbd_p1_f`-Einstieg, `kbd_p1_f_cont`, `sub_e005b13c` (Override)
und `kbd_p2_f` (die Firmware-Version reicht, die CHDK-Kopie dient nur dem Jogdial-Blocking).
Achtung: Der CHDK-Build bindet immer den ganzen Core ein (Spytask, GUI, Konfiguration). Ihn ganz wegzulassen, wäre
ein Eigenbau ohne CHDK-Infrastruktur. **Entscheidung für Prompt 3/5:** CHDK normal mit allen OPT_* aus
(bewährter Weg) oder ein abgespeckter Eigenbau.

### 3. Tasten lesen und simulieren
- `mykbd_task` (`kbd.c:57-75`): Schleife solange `physw_run`, mit `_SleepTask(physw_sleep_delay)`.
  Ruft `wrap_kbd_p1_f()` auf und bei Rückgabe 1 `kbd_p2_f_my()`.
- `wrap_kbd_p1_f` (`kbd.c:42-52`): ersetzt den Anfang von Firmware-`kbd_p1_f`: `push {r1-r7,lr}; movs r4,#0`,
  dann **`my_kbd_read_keys()`**, dann weiter in `kbd_p1_f_cont_my`.
- `my_kbd_read_keys` (`kbd.c:126-133`):
  ```c
  if (kbd_update_key_state()) physw0_override = physw_status[0];
  else                        physw0_override = -1;
  kbd_update_physw_bits();
  ```
- `kbd_fetch_data` (`kbd.c:135-139`): `_GetKbdState(dst)` + `_kbd_read_keys_r2(dst)` liest die Hardware nach `kbd_new_state[3]`.
- `kbd_update_key_state` (`core/kbd_common.c:177-237`): liest per `kbd_fetch_data` und ruft `kbd_process()` auf (CHDK-Logik).
  - Status 0 (kein ALT-Modus): `physw_status[i] = kbd_new_state[i]`, die Tasten gehen unverändert durch.
  - Status ≠ 0: `physw_status[i] = (new & ~KEYS_MASKi) | (kbd_mod_state[i] & KEYS_MASKi)`.
- **Simulierte Tasten**: `kbd_key_press(key)` löscht das Bit in `kbd_mod_state`, denn **0 = gedrückt**
  (`kbd_is_key_pressed` prüft `(state & key) == 0`). `kbd_key_release` setzt das Bit wieder.
  Das Ergebnis landet nur in `physw_status`, wenn `kbd_process()` ≠ 0 zurückgibt **und** das Bit in `KEYS_MASKn` steht.
- **Wichtig für S2:** Die Firmware liest Wort 0 (mit den Auslöserbits) in `sub_e005b13c` **ein zweites Mal**.
  Das Original übergibt `-1`, CHDK übergibt `physw0_override` (`boot.c:1099-1108`).
  Ohne Override (-1) sieht die Firmware vermutlich den echten Hardwarezustand. Eine S2-Injektion würde dann
  an dieser Stelle verloren gehen. → **Bei der Injektion muss `physw0_override` = modifiziertes `physw_status[0]` sein.**
- Danach `kbd_p1_f_cont_my`: Die Firmware berechnet `(physw_status & mask) ^ invert` und erzeugt daraus die Events.
- g7x2-Tastenbits (110b, aus `physw_bits.txt` / physw_event_table @0xE05DF0E4):
  S1 = idx0 `0x00100000` (Event 0x00), S2 = idx0 `0x00080000` (Event 0x01), `KEY_SHOOT_FULL` = `0x00180000`.
  **Die G7 X II hat keine Sterntaste (AE-Lock).** Ein g7x2-Vergleich für das Stern-Bit ist also nicht möglich,
  das muss aus der g1x3-physw_event_table kommen. In CHDK gibt es dafür `KEY_AE_LOCK` (`include/keyboard.h:43`),
  andere Ports (g1x, g15, g16) mappen es.
- **Ableitung von S2 ohne Drücken:** Die physw_event_table der Firmware ordnet Bit → Event-ID zu
  (0x00 = PressSW1, 0x01 = PressSW2). Für g1x3 lässt sich dieselbe Tabelle finden (Prompt 4/6).

### 4. Dual-Core-spezifische Stellen
1. Loader: `get_core_id`. Nur Core 0 kopiert, prüft und schaltet die LED. Core 0 ruft „unblock core1“ auf. Beide Cores machen die Cache-Operationen und springen.
2. `boot()`: Core 1 wird über `sub_e0539e64` geparkt.
3. `boot()`: Der Core-1-Zweig läuft über `sub_e00200f8_my`. Darin entscheidet `cbz r6` (Core-ID) über `sub_dffc570c`.
4. `sub_e0020750_my`: unblock core1 **zweimal**, nur auf Core 0.
5. `task_TricInitTask_my`: `0xD2050074` sagt, ob Core 1 schon läuft. Dann wird das SmacIdleCmp-Eventflag gesetzt.
6. Die RAM-Kopie des Kernels (0xDFFC…) und der CreateTask-Hook müssen per dcache clean und icache flush sichtbar werden.
→ Diese Stellen sind bei g1x3 vermutlich ähnlich, siehe den zusätzlichen Startcode-Block @0x2002A.

### 5. Kameraspezifische Adressen und Stubs für g1x3
**Braucht der Minimalport (alle neu im g1x3-Dump suchen und belegen):**
| Was | g7x2 1.10B | Quelle |
|---|---|---|
| PLATFORMID + Ort | 12988 @0xE1F20270 | makefile.inc (g1x3: 0x32D7, vermutet) |
| ROMBASEADDR, MAXRAMADDR | 0xE0000000, 0x3FFFFFFF | makefile.inc |
| NEED_ENCODED_DISKBOOT | 16 | makefile.inc |
| MEMISOSTART | 0x2C460C | sub/makefile.inc |
| LED-GPIO + Methode | 0xD20801E4, LEDCNTRL_NEW4 | loader, makefile.inc |
| unblock core1 | sub_e051e078 (+ Muster-Check) | loader, boot.c |
| dcache_clean_range / icache_flush_range | 0xE042EB75 / 0xE042EC4D | loader, boot.c |
| Firmware-boot() samt Kopiertabellen | 0xE0020011 ff., Ziel 0xDFFC4900–0xDFFD0908 | boot.c |
| park core1 | sub_e0539e64 | boot.c |
| hook_CreateTask + Fortsetzung | 0xDFFC93BA / 0xDFFC93C3 | stubs_entry.S, boot.c |
| Heap-Start-Routine | sub_e00200f8 (Heap-Ende 0x6CD400) | boot.c |
| Startkette | sub_e0020398, sub_e0020750, sub_e005b418, task_Startup @0xE00206D5 | boot.c |
| diskboot-Aufruf, der in Startup übersprungen wird | sub_e046e3dc | boot.c |
| PhySw-Task-Erzeugung | sub_e005b33e, task_PhySw 0xE005B319, CreateTask_alt sub_dffc95d8 | boot.c |
| task_TricInitTask + Smac-Register | 0xE025D527, 0xD2050074 | boot.c |
| task_InitFileModules (wenn Spytask oder I/O) | 0xE04200B1 | boot.c |
| physw_status / physw_run / physw_sleep_delay | 0x4E440 / 0x8370 / 0x8378 | stubs_entry.S |
| kbd_p1_f / kbd_p1_f_cont / kbd_p2_f | 0xE005B62B / 0xE005B633 / 0xE005B079 | stubs_entry.S |
| sub_e005b13c (2. Lesen Wort 0) | 0xE005B13D, Fortsetzung 0xE005B149 | boot.c |
| GetKbdState / kbd_read_keys_r2 | 0xE004E73B / 0xE004E617 | stubs_entry.S |
| physw_event_table | 0xE05DF0E4 | stubs_entry.S |
| Tastenbits S1, S2, Stern | idx0 0x100000 / 0x80000 / – | kbd.c, physw_bits.txt |
| SleepTask / CreateTask / ExitTask | 0xDFFC9293 / 0xDFFC93BB / 0xDFFC95AF | stubs_entry.S |
| SetEventFlag (TricInit) | 0xDFFC9939 | stubs_entry_2/entry |
| für Stufe c: Open/Write/Close oder Fopen/Fwrite/Fclose_Fut, GetSystemTime | div. | stubs_entry.S |

**Braucht der Minimalport nicht** (nur NULL_SUB/FAKEDEF, damit der Core linkt):
CaptSeq/ExpDrv/FileWrite/MovieRecord-Hooks und alle zugehörigen Stubs, RAW (raw_buffer_*),
Jogdial (dial_positions, get_dial_hw_position), mzrm/UI-Overlay, PTP (get_ptp_*), FsIoNotify-Fix,
ND-Filter, Fokus/Zoom (`init_focus_eventflag`, MF-Funktionen), Property-Case-Wrapper, ARAM/EXMEM,
FI2 (KEYSYS, FI2FLAGS), Wifi/HDMI/USB-Override, playback_image_number, recreview_hold.

### 6. Offene Punkte aus CLAUDE.md, Stand nach Prompt 2
- Bootweg: **DISKBOOT.BIN, dancingbits-kodiert.** Die Version für g1x3 ist **offen**. Im Dump suchen (g7x2: „Found @e06c2db4“).
  FI2 entfällt (kein Schlüssel nötig, kein Bedarf).
- Startcode-Block @0x2002A bei g1x3: **offen**. Er liegt vor den Kopierschleifen, eventuell ein früher Core-1- oder Hardware-Check.
- Minimum an Tasks und Hooks: siehe Tabelle oben. **Dual-Core:** 6 Stellen, siehe oben.
- physw_status: **3 Wörter** (bei g7x2). Die g1x3-Bits sind **offen**. Ableitung über die physw_event_table, dann Messung.
- Sterntaste: Die g7x2 hat keine. Bit und eventuelle Mehrfachbelegung bei g1x3 sind **offen**.
- **Neu:** Polarität. In CHDK gilt 0 = gedrückt (`kbd_common.c:382`). Die Firmware invertiert pro Bit per Maske
  (`kbd_p1_f_cont`: `(physw & mask) ^ invert`). Die Spalte „non-inverted“ in physw_bits.txt gibt das je Taste an. Wird gemessen.
- **Neu:** Die S2-Injektion braucht `physw0_override` (siehe Abschnitt 3). Sonst liest die Firmware Wort 0 roh.

### Entscheidung (Nutzer, 2026-11-12)
**Weg: normaler CHDK-Build mit allen optionalen Features aus** (OPT_* in `localbuildconf.inc`).
Der abgespeckte Eigenbau ist verworfen. Folge: Der CHDK-Core (Spytask, Konfiguration, GUI) läuft mit, daher
muss auch der Spytask-Pfad (InitFileModules-Hook, `core_spytask_can_start`) für g1x3 belegt werden.
Der Stubs-Umfang zum Linken ist voll, wird aber über NULL_SUB/FAKEDEF abgedeckt (Prompt 3).

### Entscheidung Tastenverhalten (Nutzer, 2026-11-12)
- **Auslösetaste ist Q/SET, nicht die Sterntaste.** Q/SET allein bleibt normal (Q-Menü).
- Kein sichtbares CHDK (kein Menü, kein ALT, kein OSD), die Funktion ist permanent aktiv.
- Die drei Detailentscheidungen (SET verstecken, nur S1 zuerst löst aus, S2 solange SET gehalten)
  stehen als Tabelle in CLAUDE.md → „Tastenverhalten“.
- Folge für Prompt 4 und 6: Statt des Stern-Bits brauchen wir das **Q/SET-Bit**. g7x2 bietet einen Vergleich
  (`KEY_SET` idx2 0x2000, physw_event_table-Eintrag 0x0101024d, Event 0x0a). Das Testprotokoll in Prompt 6
  wird auf Q/SET umgestellt (Halbdruck, Q/SET, Halbdruck+Q/SET, Video als Kontrolle).

## Prompt 3 – Skelett und rebuild-stubs (2026-11-12)

### Skelett (frisch geschrieben, nichts aus g7x2 kopiert)
- `chdk/platform/g1x3/makefile.inc`: PLATFORMOS=dryos, PLATFORMOSVER=60, DIGIC=70, THUMB_FW=1, ROMBASEADDR=0xE0000000
- `chdk/platform/g1x3/sub/100a/`: `Makefile` (include makefile_sub.inc), leeres `makefile.inc`,
  `stubs_entry_2.S` und `stubs_min.S` nur mit `#include "stubs_asm.h"` → **keine g7x2-Adresse übernommen**
- Dumps per Hardlink: `dumps/g1x3/sub/100a/PRIMARY.BIN`, `dumps/g7x2/sub/110b/PRIMARY.BIN`
- Aufruf: `./chdkmake.sh make PLATFORM=g1x3 PLATFORMSUB=100a PRIMARY_ROOT=/srv/dumps rebuild-stubs`
  → Abbruch wie erwartet wegen 12 nicht gefundener Namen. Kein Kamera-Build, `bin/` leer.

### Ergebnis finsig_thumb2 (alles hier ist Sigfinder-Ausgabe = „vermutet“, bis in Ghidra geprüft)
- Funktionsliste `funcs_by_address.csv`: **3007 Einträge** (2882 verschiedene Adressen)
- `stubs_entry.S.err`: **149 NHSTUB**, 29 DEF/DEF_CONST, **12 ERROR**
- Fehlende Namen: EngDrvRead, ExpCtrlTool_StopContiAE, RenameFile_Fut, Restart, SetFileAttributes,
  SetFileTimeStamp, TurnOffBackLight, TurnOnBackLight, _pow, exmem_ualloc, exmem_ufree, reboot_fw_update.
  **Keiner davon gehört zum Minimalport.** Zum Linken mit „Optionen aus“ werden sie in stubs_entry_2.S
  als NULL_SUB gesetzt (gleiches Vorgehen wie bei g7x2). `reboot_fw_update` bleibt NULL_SUB (Regel 2).
- Header:
  - Main firmware start 0xe0020000, DRYOS R60 @0xe0637660, Firmware GM1.00A @0xe05546b0
  - VMSA erkannt (DIGIC ≥ 7), MEMISOSTART 0x339448, MEMBASEADDR 0x8000
  - ARAM_HEAP_START/SIZE nicht gefunden
  - **Keine Platform-ID-Ausgabe** vom Sigfinder → PID 0x32D7 bleibt „vermutet“ (nur Dump-Offset 0x1F20270)
- Code-Regionen:
  | Region | Ziel | Quelle im ROM | Größe |
  |---|---|---|---|
  | ROM | 0xe0000000–0xe2000000 | – | 32 MB |
  | RAM data | 0x8000–0x6b1f8 | 0xe10ab568 | 406008 B |
  | RAM code | 0x01900000–0x01901504 | 0xe110e760 | 5380 B |
  | TCM code | 0xdffc4900–0xdffd68b0 | 0xe110fc64 | 73648 B |
  | zico_0 (Xtensa) | 0xbff20000 | 0xe0f48db8 | – |
  | zico_1 (Xtensa) | 0xbff00000 | 0xe0f4daf8 | – |
  | zico_2 (Xtensa) | 0x80a00000 | 0xe0f51d60 | – |
- DryOS-Strings: r60+p1 (0xe001a0e8, Haupt 0xe0637660), r59+p4 (0xe0e78598, 0xe121f100),
  r59+p3 (0xe0f52600), r54 (0xe1bc0360). Nur r60 ist das Haupt-OS, die anderen gehören zu Nebenprozessoren/Blobs.
- Vorschläge platform_camera.h: CAM_DRYOS, R39, R47, R59, CAM_HAS_WIFI, CAM_HAS_ND_FILTER
- Warnungen: mehrere „failed to get CreateTask args“, doppelte Namen (task_LensController, task_Thermometer,
  daf_dumpdafraw_FW, task_Raw2Ss2YuvTask), fehlend GetCurrentDriveBaseSvValue_FW,
  WaitForAnyEventFlagStrictly-Referenz _imageSensorTask nicht gefunden, sig_aram_start ohne DebugAssert-Treffer.
  Nichts davon betrifft den Minimalport.

### Minimalport-Liste: gefunden (Sigfinder) – Status „vermutet“ bis Ghidra-Beleg
| Name | g1x3 | g7x2 (Vergleich) |
|---|---|---|
| physw_status | 0x0006c510 (@0xe0040a84) | 0x0004e440 |
| physw_run | 0x000082b4 | 0x00008370 |
| physw_sleep_delay | 0x000082bc | 0x00008378 |
| task_PhySw | 0xe0040725 | 0xe005b319 |
| kbd_p1_f | 0xe0040a7d | 0xe005b62b |
| kbd_p1_f_cont | 0xe0040a85 | 0xe005b633 |
| kbd_p2_f | 0xe0040479 | 0xe005b079 |
| kbd_read_keys | 0xe0040a47 | 0xe005b5f5 |
| kbd_read_keys_r2 | 0xe003ac59 | 0xe004e617 |
| GetKbdState | 0xe003add3 | 0xe004e73b |
| hook_CreateTask | 0xdffc5ade | 0xdffc93ba |
| CreateTask | 0xdffc5adf | 0xdffc93bb |
| ExitTask | 0xdffc5cd7 | 0xdffc95af |
| SleepTask | 0xdffc598f | 0xdffc9293 |
| SetEventFlag | 0xdffcbc81 | 0xdffc9939 |
| DebugAssert | 0xdffc5e25 | 0xdffc96f5 |
| task_TricInitTask | 0xe03b13fb | 0xe025d527 |
| task_InitFileModules | 0xe0456a0d | 0xe04200b1 |
| err_init_task | 0xe00203a1 | 0xe0020419 |
| Open / Write / Close | 0xe039dc4d / 0xe039e11d / 0xe039dc8b | 0xe0373e7b / … |
| Fopen/Fwrite/Fclose_Fut | 0xe03985eb / 0xe03986bf / 0xe0398613 | – |
| GetSystemTime | 0xe057bb2b | 0xe0569c53 |
| levent_table | 0xe06d8008 | 0xe066c2a8 |
| PostLogicalEventToUI | 0xe039d90d | 0xe037a86d |

physw_status 0x6c510 liegt hinter dem RAM-data-Bereich (bis 0x6b1f8), also im BSS. Das ist plausibel.

### Minimalport: NICHT vom Sigfinder (müssen in Prompt 4 per Ghidra/capdis gefunden werden)
Bei g7x2 sind das harte Adressen in `boot.c`/`kbd.c`, keine Stubs:
- Einsprung `boot()`, Startcode bis Heap-Start (g7x2 sub_e00200f8), Core-1-Unblock (g7x2 sub_e051e078)
- task_Startup und der Diskboot-Aufruf darin (g7x2 sub_e046e3dc)
- PhySw-Erzeugung (g7x2 sub_e005b33e) und der zweite Wort-0-Lesezugriff (g7x2 sub_e005b13c → physw0_override)
- TricInitTask-Prüfadresse (g7x2 0xD2050074) und Event-Flag
- LED-Adresse (g7x2 0xD20801E4), nur optional
- Cache-Routinen kommen aus dem CHDK-Loader, nicht aus der Firmware

### physw_bits.txt / Tastentabelle
- physw_event_table @0xe063ee34 (Found @0xe003ac5e)
- **Q/SET: idx2 0x2000, levent 0x0a, Eintrag @0xe063ef14, info 0x0101024d** – gleiche Werte wie g7x2
- **SHOOT_HALF/SHOOT_FULL nicht gefunden:** In der Tabelle gibt es keine Einträge mit Event 0x00 (PressSW1)
  oder 0x01 (PressSW2). Die S1/S2-Bits werden anders behandelt und müssen in Prompt 4 über den Code
  ermittelt werden. Kandidaten aus dem Tabellendump: idx0-Einträge mit Event -1 (info 0x0400810x bei
  0x200–0x2000, 0x020080xx bei 0x8000–0x40000). Bei g7x2 liegen S1/S2 auf idx0 0x100000 / 0x80000.
- Weitere Vorschläge: SD_READONLY idx0 0x20, USB idx2 0x01000000, BATTCOVER idx2 0x00800000, HOTSHOE idx2 0x00010000
- **Vergleich der Tabellen (Beobachtung, keine Schlussfolgerung):** Bei g7x2 stehen S2/S1 als info 0x08010213
  (Event 0x01, idx0 0x80000) und 0x07010214 (Event 0x00, idx0 0x100000) mitten in einer Reihe 0x20f–0x216.
  Diese Reihe gibt es bei g1x3 als 0x22f–0x236 auf **idx1** mit gleichen Events (0x101, 2, 3, 4 …). An den Plätzen
  von S2/S1 stehen dort aber Event 0x15 (MENU, idx1 0x80000) und 0x0c (idx1 0x100000). Die info-Obergruppen
  0x07/0x08 (bei g7x2 nur S1/S2) kommen bei g1x3 gar nicht vor. → S1/S2 laufen bei g1x3 vermutlich über einen
  anderen Weg als die physw_event_table. Klärung in Prompt 4 (Ghidra: Verwender von PressSW1/PressSW2 bzw.
  levent 0x00/0x01 und die Lesepfade von physw_status).

## Ghidra-Setup für Prompt 4 (2026-11-12)
- Projekte: `ghidra/Canon` (g1x3, Ordner `/g1x3/PRIMARY.BIN`) und `ghidra/Canon_g7x2` (g7x2, `/g7x2/PRIMARY.BIN`).
  Getrennt, damit beide parallel analysiert werden konnten und die GUI g1x3 früh öffnen kann.
- Import headless: `analyzeHeadless … -loader BinaryLoader -loader-baseAddr 0xE0000000 -processor ARM:LE:32:v7`.
- Jython-Extension (aus `Extensions/Ghidra/…_Jython.zip`) nach `~/Library/ghidra/ghidra_12.1.4_PUBLIC/Extensions/` entpackt,
  weil die CHDK-Skripte `@runtime Jython` brauchen.
- Speicherkarte: `InitCHDKMemMap.py` mit `stubs_entry.S.err` (Bereiche wie im Abschnitt Prompt 3, dazu MMIO, ATCM, BTCM, RAM-Lücken).
  Zico-Blobs absichtlich nicht gemappt (Default des Skripts).
- Stubs: `ImportCHDKStubs.py`, Modus „Entry points only“.
  - g1x3: nur `stubs_entry.S.err` + `funcs_by_address.csv`.
  - g7x2: `funcs_by_address.csv`, `stubs_entry.S`, `stubs_entry_2.S`, `stubs_min.S`.
  - Headless-Hinweis: askChoices erwartet das Argument in echten Anführungszeichen, `'"a;b"'`.
- Auto-Analyse: g1x3 317 s, g7x2 384 s, beide ohne Fehler. Warnungen nur zu Lesezugriffen auf uninitialisiertes RAM.
- g1x3 in Ghidra: 49 543 Funktionen, SHA-256 558e8448…6bd6c (= eigener Dump).
- GhidraMCPPlugin im CodeBrowser aktiviert, HTTP auf 127.0.0.1:8080, MCP-Abfrage funktioniert
  (z. B. kbd_p1_f @ 0xe0040a7c).

## Prompt 4 – g1x3-Adressen per Ghidra und capdis (2026-11-12)
Werkzeuge:
- Ghidra-MCP auf g1x3.
- `capdis` (CHDK-Tool, im Docker gebaut) für den Vergleich g7x2 ↔ g1x3, Befehl für Befehl.

„verifiziert“ heißt: Der g1x3-Code ist strukturell gleich wie das g7x2-Original (dieselbe Befehlsfolge, nur andere Adressen
und Konstanten), oder ein Firmware-String bzw. eine Tabelle belegt die Funktion direkt.

Die Ghidra-Speicherkarte stimmt. Stichprobe: TCM-Bytes @0xdffc5ade entsprechen dem Rohdump @0xe110fc64+0x11de.
Hinweis: `read_memory` des MCP meldet byte-gemappte Blöcke als „uninitialized“. Disassembly und Decompiler lesen sie korrekt.

### Adressliste Boot/Loader (Gegenstücke zu g7x2 110b)
| Was | g7x2 110b | **g1x3 100a** | Beleg | Status |
|---|---|---|---|---|
| boot() | 0xe0020011 | **0xe0020011** | gleicher Aufbau (VBAR, SP=0x2000, Core-ID) | verifiziert |
| Zusatzblock 0x2002A (nur Core 0) | – | `if (*0xD2101504==0) *0xD2101504=1` | Disassembly e002002e–e002003e | verifiziert (Inhalt). Zweck unbekannt, wird 1:1 mitkopiert |
| RAM-data-Kopie | e0fd37b8→0x8000..0x4c610, BSS bis 0x2c460c | **e10ab568→0x8000..0x6b1f8, BSS bis 0x339448** | boot() Literale | verifiziert |
| MEMISOSTART | 0x2C460C | **0x339448** | BSS-Ende in boot() + Heap-Start 0x33944f | verifiziert |
| RAMCODE-Kopie | e1017dc8→0x1900000, 0x139c | **e110e760→0x1900000, 0x1504** | boot() | verifiziert |
| TCM/Kernel-Kopie | e1019164→0xdffc4900..0xdffd0908 | **e110fc64→0xdffc4900..0xdffd68b0 (0x11fb0)** | boot() | verifiziert |
| dcache_clean_range | 0xe042eb75 | **0xe03a9971** | Aufruf an gleicher Stelle in boot(), Schleife `mcr c7,c10,1` | verifiziert |
| icache_flush_range | 0xe042ec4d | **0xe03a9a45** | wie oben, `mcr c7,c5,1` + `c7,c1,6` | verifiziert |
| park core1 | sub_e0539e64 | **sub_e05c01f8** | Bytemuster `movs r4,r0; movw r0,#0xa; …` + 2× Warteschleife sub_e05649a8 | verifiziert |
| Heap-Start-Routine | sub_e00200f8 | **sub_e0020108** (Thumb 0xe0020109) | gleicher Code, Heap-Ende 0x6cd400 | verifiziert |
| Startkette 2 | sub_e0020398 | **sub_e0020320** | Strings „USER_MEM size checking“, „dmSetup“ … | verifiziert |
| Startkette 3 (Startup anlegen) | sub_e0020750 | **sub_e0020530** | CreateTask(„Startup“, task_Startup) | verifiziert. **Extra:** `sub_e012e258(2,0x1f0)` am Anfang, Zweck offen (wird mitkopiert) |
| unblock core1 | sub_e051e078 | **sub_e05812b0** (Thumb 0xe05812b1, Muster 0xB5102101) | gleicher Code, schreibt 0xC1001F00 über sub_e0564990 | verifiziert |
| Einschaltprüfung | sub_e005b418 | **sub_e0040842** | liest GPIO 0x168/0x150/0x16c/0x2c0/0x194/0x1e4 | verifiziert |
| Start-Modus setzen (in Einschaltprüfung, CHDK entfernt den Aufruf) | sub_e004e4d6, Var 0x9914+8 | **sub_e003aafe, Var 0x9e1c+8** | Decompile | verifiziert |
| CHDK-Startmodus-Hack | GPIO 0xd2082168 Bit 0x10000 → 0x100000 (play) / 0x80000 (rec) | **0xd2082168 Bit 0x10000 → 0x400000 (play) / 0x200000 (rec)** | sub_e0039cc6 liest Basis 0xd2082000. Das Wort wird in kbd_read_keys_r2 in physw idx2 eingeblendet: 0x400000 = Event 0x80000500, 0x200000 = 0x80000501 (bei g7x2 0x100000 = 0x80000500, 0x80000 = 0x80000501) | verifiziert über Event-IDs |
| task_Startup | 0xe00206d5 | **0xe00204b5** | gleiche Aufrufliste | verifiziert |
| Diskboot-Aufruf (überspringen) | sub_e046e3dc | **sub_e058131c** | String „StartDiskboot“ | verifiziert |
| PhySw-Erzeugung | sub_e005b33e | **sub_e004074a** | „PhySw“, CreateTaskStrictly_alt, Prio 0x13, Stack 0x800 | verifiziert. Ruft danach **loc_e0039f7c** auf (legt PhySw_ReleaseSw an, siehe unten) |
| task_PhySw | 0xe005b319 | **0xe0040725** | Sigfinder + Code | verifiziert |
| hook_CreateTask / Fortsetzung | 0xdffc93ba / 0xdffc93c3 | **0xdffc5ade / 0xdffc5ae7** | gleiche 8 Byte (`push {r1-r7,lr}; mov r4,r3; mov.w r3,#0x1000`) | verifiziert |
| CreateTask / CreateTask_alt / CreateTaskStrictly_alt | 0xdffc93bb / 0xdffc95d8 / – | **0xdffc5adf / 0xdffc5d00 / 0xdffcc3d7** | capdis | verifiziert |
| task_TricInitTask | 0xe025d527 | **0xe03b13fb** | gleicher Code, Event-Flags 0x1fad4 statt 0x256ec | verifiziert |
| Smac-Register für TricInit-Fix | 0xd2050074 | **0xd2050074** | Init schreibt 0x087f0248/0x007f0249, Abschalten 0 (FUN_e03bf794) – identisch zu g7x2 | verifiziert |
| task_InitFileModules + Patchpunkt | 0xe04200b1, sub_e0496864 | **0xe0456a0d, sub_e04e46fc** | gleicher Code | verifiziert |
| dancingbits (NEED_ENCODED_DISKBOOT) | 16 | **17** (Tabelle {4,7,5,6,1,3,2,0} @0xe001a10c im Bootloader, einzige im Dump) | g7x2-Tabelle 16 liegt analog @0xe0018d30 | verifiziert |
| LED | 0xD20801E4 | – | nicht gesucht, optional | offen (wird nicht gebraucht) |

Nicht gebraucht (g7x2-Patches, die wir weglassen): kbd_p2_f_my/sub_e004e5ee_my (Jogdial),
MBR-/Partitions-Patch in der InitFileModules-Kette (sub_e03748a8_my), FsIoNotify-Fix, mzrm.

### Tasten: Ergebnis mit Folgen für das Design
**Q/SET** = physw idx2 Bit 0x2000. Tabelle @0xe063ef14, info 0x0101024d, Event 0x0a.
- GetKbdState: Wort 2 = (reg 0xd208f220 & 0x3e00) | …
- Typ-Byte 0x02 = normale Taste, Flag-Byte gleich wie g7x2.
- Status: verifiziert (Bit). Polarität „0 = gedrückt“ per Analogie zu g7x2, Bestätigung im Logger (Prompt 6).

**S1/S2 laufen bei g1x3 NICHT über physw_status.**
- Die physw_event_table (42 Einträge, Ende bei 0x25b) enthält kein Event 0x00/0x01.
- An den Bitpositionen, an denen g7x2 S1/S2 hat (Register 0xd208f210), liegen hier MENU (0x15) und Event 0x0c.

Stattdessen gibt es einen **Interrupt-Weg „PhySwIsr.c“**:
- Init FUN_e0039d18:
  - S1 = GPIO **0xd20821d8**, S2 = GPIO **0xd20821d4**, jeweils Bit 0x10000 = 0 → gedrückt.
  - Interrupts **0xea (S1)** und **0xfa (S2)**, Pegelregister **0xd5013038 (S1)** / **0xd501303c (S2)**, Bit 0 = 0 → gedrückt.
- ISRs 0xe0039cf6 (S1) / 0xe0039cd4 (S2) schicken Nachricht 1 bzw. 2 an die Queue `*(0x9e3c+8)`.
- **task_PhySw_ReleaseSw** 0xe0039eef (Prio 0x13, Stack 0x400, angelegt in loc_e0039f7c direkt nach PhySw) verarbeitet sie:
  - Nachricht 1/2 → FUN_e0039e98: Ist der Pegel 0, folgt Press über die Zustandsmaschine FUN_e0039e0c.
  - **Nachricht 3 kommt alle 10 ms** (Timer 0x2710 µs, Callback 0xe0039e03).
    Meldet das Pegelregister „los“ und der Zustand „gedrückt“, wird zwangsweise Unpress gesendet.
- Zustandsmaschine:
  - Tabelle @0xe063f074 = `81 c2 00 00 | 01 43 10 01 | 03 02 02 30 | 03 03 02 21`.
  - Zustand `*(0x9e3c+0xc)`, Bit0 = S1, Bit1 = S2.
  - Eingänge: S1 drücken, S2 drücken, S1 los, S2 los.
  - Ausgabe: High-Nibble 0x80 PressSW1, 0x40 PressSW2, 0x20 UnpressSW2, 0x10 UnpressSW1.
  - Im Zustand S1+S2 erzeugt „S1 los“ kein Event. Erst „S2 los“ schickt UnpressSW2 + UnpressSW1.
- Codes an die UI: 0x80000000 PressSW1, 0x80000001 PressSW2, 0 UnpressSW1, 1 UnpressSW2.
  Ausgabe über FUN_e003a00a → FUN_e004077e → FUN_e039d202(code,0), dieselbe Senke wie physw-Tasten („PhySw to UI“).
- Die Register 0xd5013038/3c kommen im ROM nur in dieser Tabelle vor, 0xd20821d4/d8 nirgends als Literal.
  Andere Firmwareteile lesen die S1/S2-Hardware also vermutlich nicht direkt.

**Folgen:**
1. `physw0_override` ist bei g1x3 **nicht nötig**: sub_e004053c (Gegenstück zu g7x2 sub_e005b13c) hat keinen zweiten
   Hardware-Lesezugriff und keinen −1-Parameter. Der CHDK-Patch von kbd_p1_f_cont entfällt.
2. S2 lässt sich **nicht über physw_status** einspeisen. Die Optionen (Entscheidung in Prompt 7, durch den Nutzer):
   - A) task_PhySw_ReleaseSw durch eine CHDK-Kopie ersetzen (Patch in sub_e004074a_my bzw. Kopie von loc_e0039f7c), die an den
     beiden S2-Pegelabfragen ein „virtuelles S2“ dazu-ODERt. Der Kbd-Task setzt das Flag und schickt Nachricht 2.
     Die Canon-Zustandsmaschine bleibt intakt, auch Halten (Serie/Video) und die Reihenfolge der Events. Mehr kopierter Code.
   - B) FUN_e0039e0c(0x80000001) direkt aufrufen. Der 10-ms-Poll löst S2 sofort wieder (Hardware „los“), Halten geht nicht.
     Dazu kommt eine Race-Gefahr mit dem ReleaseSw-Task.
   - C) UI-Codes direkt senden (FUN_e039d202). Halten geht, aber die Zustandsmaschine kennt S2 nicht.
     Wird S1 zuerst losgelassen, kommt UnpressSW1 vor UnpressSW2. Diese Reihenfolge erzeugt echte Hardware nie.
3. S1 zum Erkennen ist am saubersten über den Zustand `*(0x9e48)` Bit 0 zu lesen (entprellt, nur lesen) oder über das Pegelregister 0xd5013038.

### Funktionen, die Flash/ROM/persistente Daten schreiben – NIE aufrufen (Regel 1)
Namen aus den Eventproc-Registrierungen der Firmware (Sigfinder, `funcs_by_name.csv`):
- **ROM/FROM direkt:** WriteToRom_FW 0xe04958cd, EraseRom_FW 0xe0495947, WRITEADJTABLETOFROM_FW 0xe00578b1,
  WRITEADJUSTMENTDATATOFROM_FW 0xe00578d1, WRITEUSERSETTINGDATATOFROM_FW 0xe00578ed, OpLog.WriteToROM_FW 0xe003c653,
  WriteMechaLogToRom_FW 0xe05792f5, EraseLogSector_FW 0xe0041b01, EraseSignature_FW 0xe05709a9,
  WriteWLANMACAddressToFROM_FW 0xe0516161
- **Justage/Kalibrierung:** EraseAdjustmentArea_FW 0xe0056f51, SaveAdjustmentTable_FW 0xe0057293, SaveAdjustmentValue_FW 0xe00572e9,
  SaveDefectAdjTable_FW 0xe00572d7, SaveBatSenseOffsetAdjustmentTable_FW 0xe059f35b, CreateAdjustmentTableMirror_FW 0xe00577f9,
  WriteGyroAdjustment_FW 0xe05a2a65, WriteHallDeviceAdjustment_FW 0xe05a2a5b, WritePWMAdjustment_FW 0xe05a2ab1,
  DispCon_Save*Parameter_FW (0xe04c6877 … 0xe04c6b45), LcdCon_Save*Parameter_FW (0xe0187f6b, 0xe019235f, 0xe0192235, 0xe019367f, 0xe02d3fdd, 0xe02d40ad),
  ClearDefectTurnTable_FW 0xe060cc81, ExecuteDefectRecAndSaveImage*_FW 0xe060cabd/0xe060cbad
- **Werks-/Seriennummer/IDs:** SaveFactoryAdjData_FW 0xe04fce6b, SaveFactoryIDData_FW 0xe04fce9b, SaveFactoryID_FW 0xe059f7cf,
  SaveFactoryReserveData_FW 0xe059fa67, SetFactorySerialNumber_FW 0xe059f8ff, Set/ClearFactoryMasterFlag_FW 0xe002eff1/0xe002efeb,
  Set/ClearFactoryMasterROMProtection_FW 0xe002f00d/0xe002eff7, Set/ClearFactoryMode_FW 0xe002eed5/0xe002eeab,
  UTSU_Set/ClearFactoryMode_FW, Set/ClearWLANFactoryMode_FW, ExecuteInitFactorySetting_FW 0xe04dc9b3, FactoryResetWithShutdownTimer_FW 0xe059fea5
- **Firmware-Update:** FactoryFirmup_FW 0xe059fdc7, TP_FirmwareUpdate_FW 0xe0487be7, WriteBLEFirm_FW 0xe0516ca3, `reboot_fw_update` (nicht gefunden → NULL_SUB)
- **NFC/BLE-Speicher:** WriteNFCSystemArea_FW 0xe05166ab, WriteNFCUserArea_FW 0xe059422b, WriteNFCOptUnitAssemblyAreaData_FW 0xe0516863,
  WriteNFCSystemAreaForOptUnitAssembly_FW 0xe0516879, Clear/ResetNFCSystemArea_FW 0xe0593bc9/0xe0593d01
- **Persistente Kameraeinstellungen** (werden beim Ausschalten ins Parameter-Flash ~0xE1F20000 gesichert):
  SetParameterData = PTM_BackupUIProperty_FW 0xe0290e1d, PTM_AllReset_FW 0xe0290139, PTM_RestoreUIProperty_FW 0xe039b8ff,
  SetPropertyCase 0xe03a5261 (einzelne Propcases werden gesichert).
  → Der Minimalport ruft keine davon auf. Der CHDK-Core mit allen OPT_* aus muss in Prompt 5 daraufhin geprüft werden.

## Prompt 5 – Stufe a/b: Testbuild „laden + sauber an Canon übergeben“ (2026-11-12)

### Neue Belege (alle im g1x3-Dump)
- **P-ID:** Halbwort @0xE1F20270 = 0x32D7 = **13015**. Bei g7x2 liegt an derselben Stelle 0x32BC = 12988 = deren PLATFORMID. Status: verifiziert.
  Wird nur gelesen (Loader-Prüfung).
- **Versionsstring:** „GM1.00A“ @0xE05546BD (Sigfinder „Firmware Ver“, Bytes geprüft). Status: verifiziert.
- **Diskboot-Ladeadresse:** sub_e05812c4 → loc_e058136e:
  - kopiert die Datei nach `*0xe058143c` = **0x40008000** (ungecachter Spiegel von 0x8000) und vergleicht die Kopie,
  - sendet über loc_e05c0214 SGI 10 an den anderen Kern,
  - springt mit `bx (0x8000|1)`, also im Thumb-Modus.
  → MEMBASEADDR 0x8000 und Loader-Einstieg in Thumb: verifiziert.
- **Diskboot-Bedingung** (sub_e058131c, nur wenn der Code aus dem ROM läuft):
  - `FUN_e05c1a9c(0)` = GPIO 0x270 muss 0 sein, `FUN_e05c1a9c(1)` = GPIO 0x274 muss 1 sein. Beides kommt aus „PhySwBootSD.c“.
    Das passt zur CHDK-Regel „Karte auf LOCK“. Welche GPIO den Schieber liest, ist nicht einzeln belegt.
  - FUN_e03a0de2(0): Der Bootsektor muss „BOOTDISK“ enthalten (FUN_e03a039e → FUN_e03a11f6). Offset **0x1E0 bei FAT32**,
    **0x40 bei FAT12/16**. **exFAT wird nie als bootfähig erkannt.**
  - Danach sucht die Firmware „A/DISKBOOT.BIN“, sonst „A/Upgrader.bin“. Ist die Datei nicht ausführbar, folgt „not executable“ (FUN_e05a54b0).
- **SD1stInit:** Bei g1x3 legt die Diskboot-Funktion sub_e058131c über FUN_e0543bd8 auch den Task „SD1stInit“ an.
  Der Task schaltet die Karte über sub_e0543b3e stromlos. Fehlt der Diskboot-Aufruf (wie im g7x2-Port), läuft der Fallback in FUN_e0543c16.
  Dieser ruft sub_e0543b3e ebenfalls auf. Der g7x2-Port ruft dazu als „UHS-Fix“ das Gegenstück sub_e04d9986 im task_Startup auf.
  - g1x3 sub_e0543b3e entspricht g7x2 sub_e04d9986 Befehl für Befehl: GPIO 0x124…0x114 → 0x024c0003, Sleep 20, sub_e04dc4ec(0), Assert „SdPower.c“.
    Zusätzlich setzt g1x3 am Ende GPIO 0x74 → 0x024c0003.
  - sub_e04dc4ec = Kommando 0x2F/0x33 an den Power-Baustein (SD-Versorgung). Flüchtig, **kein Flash**.
- **SmacIdleCmp:** FUN_e03bf7a4 (Smac-Interrupt, Basis 0xd2050000) setzt `SetEventFlag(*0x1fad4, 0x80)` mit dem String „SmacIdleCmp“.
  0x1fad4 ist dasselbe Eventflag, auf das task_TricInitTask wartet. Damit ist der TricInit-Fix für g1x3 belegt.
- **unblock core1** sub_e05812b0(n) = `FUN_e0564990(10, 1<<n)` = GIC-SGI-Register 0xC1001F00 (SGI 10 an Core n).
  Die Park-Routine sub_e05c01f8 wartet **zweimal** auf SGI 10 (2× sub_e05649a8, `wfi` + 0xC100010C). Deshalb ruft CHDK unblock zweimal auf.
- **LED:** Die Canon-LED-Init bei ~0xe002df86 legt LED0 auf GPIO-Offset **0x1dc** (→ 0xD20801DC) und LED1 auf 0x18c.
  - Typ 0 schaltet direkt: an = `*(0xd2080000+off) = 0x024D0002` (sub_e0472134), aus = 0x024C0003.
    Das ist dieselbe Kodierung wie g7x2 (dort 0x1E4).
  - LED0 kann auch über PWM (Kanal 0x10) betrieben werden.
  - **Achtung:** g1x3-GPIO 0x1E4 ist ein **Eingang** (wird in der Einschaltprüfung gelesen). Die g7x2-LED-Adresse wäre hier falsch.
  - Welche Lampe LED0 physisch ist (Kontrollleuchte hinten oder AF-Hilfslicht), ist offen. Das zeigt der Test.

### Bauumfang Stufe a/b (bewusst ohne CHDK-Core)
Ein Build mit CHDK-Core würde den Spytask starten und dadurch ~150 Sigfinder-Stubs aufrufen, die noch nicht einzeln belegt sind (Regel 5).
Für Stufe a/b gibt es deshalb einen **Mini-Kern** (`chdk/g1x3_stufe_ab/`, 1836 Byte). Er enthält nur den Neustart der Canon-Firmware.
Der Loader (`chdk/loader/g1x3/`) ist der endgültige. Für den richtigen Port (Stufe c/d) kommt der CHDK-Core dazu.
- `loader/g1x3/main.c`:
  - Eigene Kompatibilitätsprüfung (P-ID + „GM1.00A“, nur lesen). Passt etwas nicht, blinkt LED0 endlos und es gibt keinen Neustart.
  - Danach: Kern nach 0x339448 kopieren, LED0 an, Muster 0xB5102101 an 0xe05812b0 prüfen, unblock core1 (nur Core 0), dcache/icache, Sprung 0x339449.
- `g1x3_stufe_ab/boot.c`:
  - boot() mit CreateTask-Hook (8 Byte @0xdffc5ade, nur für TricInitTask)
  - sub_e0020108_my: Heap-Start hinter den Kern, Startmodus-Hack 0x9e24
  - sub_e0020320_my, sub_e0020530_my (2× unblock), sub_e0040842_my (Abbruchzweig und sub_e003aafe raus)
  - task_Startup_my: ohne StartDiskboot, mit SD-Stromzyklus
  - task_TricInitTask_my
- **Nicht enthalten:** Spytask, kbd/PhySw-Ersatz (PhySw und PhySw_ReleaseSw bleiben Canon), Capt/Exp/FileWrite/FsIo/InitFileModules-Hooks, mzrm.
- Build: `./chdkmake.sh make -C g1x3_stufe_ab`. Ergebnis: `chdk/g1x3_stufe_ab/DISKBOOT.BIN` (2225 Byte). **Nicht in bin/.**
- Canon-Heap startet dadurch 0x738 Byte später (0x339b80 statt 0x339448). Die Prüfung in sub_e0020108 (≥ 0xee64c frei) bleibt erfüllt.

### Prüfungen am Build (automatisch, 2026-11-12)
1. **Kopierte Funktionen = Dump:** Ein Skript vergleicht jede kopierte Funktion in boot.c Zeile für Zeile mit der capdis-Ausgabe
   des g1x3-Dumps: boot, sub_e0020108, sub_e0020320, sub_e0020530, sub_e0040842, task_Startup, task_TricInitTask.
   Abweichungen gibt es nur an den mit +/−/Patched markierten Stellen.
2. **Alle 63 Stubs** in stubs_auto.S: Kodierung `ldr.w pc,[pc]` + `.long addr|1`, wortausgerichtet, Ziel = Name. 0 Fehler.
3. **Hook:** patch_CreateTask @0x33955e und Ziel 0xdffc5ade liegen beide auf 2 mod 4. Damit ist der PC-relative Literal identisch.
   Der Literal zeigt auf CreateTask_my+1 (0x33944d). Fortsetzung 0xdffc5ae7. Die ersetzten Byte `fe b5 1c 46 4f f4 80 53` stimmen mit dem Dump überein.
4. **DISKBOOT.BIN:** Die Python-Rückdekodierung (Tabelle 17) ergibt genau `loader/g1x3/main.bin`. Tabelle 17 in CHDK = Tabelle im Dump @0xe001a10c.
5. **Keine aufgerufene Adresse** steht auf der Flash-Sperrliste (Abgleich gegen alle 56 Adressen dort: leer).

### Adressliste nach Regel 5 (Stufe a/b) – Status aller Einträge: verifiziert
**Loader (`loader/g1x3/main.c`):**
| Adresse | Art | Beleg | Status |
|---|---|---|---|
| 0x8000 (Thumb) | Einstieg | sub_e05812c4/loc_e05c0214: Kopie nach 0x40008000, `bx 0x8001` | verifiziert |
| 0xE1F20270 | lesen (P-ID) | 0x32D7, gleiche Stelle wie g7x2 | verifiziert |
| 0xE05546BD | lesen („GM1.00A“) | Bytes im Dump | verifiziert |
| 0x339448 | schreiben (Kern-Kopie) | = MEMISOSTART, BSS-Ende in boot() | verifiziert |
| 0xD20801DC | schreiben 0x024D0002/0x024C0003 | Canon-LED-Init LED0, sub_e0472134 | verifiziert (Lage am Gehäuse offen) |
| 0xe05812b0 | lesen (Muster) + aufrufen (r0=1) | unblock core1, SGI 10 | verifiziert |
| 0xe03a9971 | aufrufen | dcache_clean_range (Prompt 4) | verifiziert |
| 0xe03a9a45 | aufrufen | icache_flush_range (Prompt 4) | verifiziert |

**boot.c – gepatcht oder neu hinzugefügt:**
| Adresse | Art | Beleg | Status |
|---|---|---|---|
| 0xdffc5ade (8 Byte) | **gepatcht** (RAM-Kopie des Kernels, nicht ROM) | hook_CreateTask, Bytes = Dump | verifiziert |
| 0xdffc5ae7 | Rücksprung | Ende der 8 ersetzten Byte | verifiziert |
| 0xe03b13fb | Vergleich (task_TricInitTask) | Prompt 4 | verifiziert |
| 0xe05c01f8 | aufrufen (park core1) | Prompt 4 + Wartelogik oben | verifiziert |
| 0xe05812b0 | aufrufen 2× (unblock core1) | wie oben | verifiziert |
| 0x9e24 (=0x9e1c+8) | schreiben 0x400000/0x200000 | Startmodus-Variable aus sub_e003aafe, Event-IDs (Prompt 4) | verifiziert |
| 0xd2082168 | lesen (GPIO Ein/Aus) | sub_e0039cc6 Basis 0xd2082000, Einschaltprüfung liest 0x168 | verifiziert |
| 0xe0543b3e | aufrufen (SD-Stromzyklus) | = g7x2 sub_e04d9986, „SdPower.c“, Canon ruft es in SD1stInit selbst | verifiziert |
| 0xd2050074 | lesen (Smac läuft) | Prompt 4 | verifiziert |
| 0xdffcbc80 | aufrufen SetEventFlag(*0x1fad4,0x80) | FUN_e03bf7a4 „SmacIdleCmp“ | verifiziert |
| 0xe03b1453 / 0xe03b1454 / 0xe03b1526 | Sprung zurück in task_TricInitTask | Originalbefehle an diesen Stellen | verifiziert |
| 0xe02443aa | Sprung (Ende task_Startup) | Original `b.w loc_e02443aa` | verifiziert |
| weggelassen: 0xe058131c (StartDiskboot), 0xe003aafe/0xe003aafc (Startmodus), `beq` Abbruch in 0xe0040842 | – | wie g7x2-Port | – |

**boot.c – unverändert übernommene Canon-Aufrufe** (Beleg: jeweils der Originalbefehl an derselben Stelle der kopierten Funktion, Vergleich 1 oben):
- boot: e03a9970, e03a9a44
- sub_e0020108: e00202dc, dffc5780, dffd1cec (bzero), dffc4a54
- sub_e0020320: e0021246, e00203a0 (init_task_error), e04f2a7c, e00221c8, e002226c, e00205d0, e0020714, e0028458, e00203b6
- sub_e0020530: e012e258, e00207e0, e002079c, e002f0b8 (IsNormalCameraMode), dffc59d4, e03a95a8, dffc5ade (CreateTask, über den Hook), e00207d0
- sub_e0040842: e003aafa, e0039cc6, e003aaf6
- task_Startup: e02443ae, e0469248, dffc5e24 (DebugAssert), e00207b4, e05812c0, e0558d8c, e0040266, e04569c4, e0020878, e0020812,
  e0558dc2, e003d870, e04569ca, e004074a (PhySw anlegen, Canon), e02b09ee, e04569e0, e0554054, e04d1dd4, e0041f40, e00d91bc,
  e04d1d90, e002087c, e03a6482, e04d1d62
- task_TricInitTask: e035a43e, dffcba84, e035a49e, dffcbcde, dffcbcae, e03b18ee

**Ergebnis: kein Eintrag „vermutet“. Ein Kamera-Build ist nach Regel 5 zulässig.**

### Kartenvorbereitung (Hilfsskript, noch nicht ausgeführt)
- `karte/sd_bootdisk.sh status|set|clear diskN` (macOS). Es schreibt nur 8 Byte an Offset 0x1E0 von Sektor 0 der Partition s1.
- Vorher prüft es: extern oder SD-Leser, Wechselmedium, ≤ 64 GB, FAT32, Signatur 55AA, Typ „FAT32“. Zum Schreiben muss man die Disk-ID nochmal eintippen.
- Mit der Dump-Karte ausgeführt (2026-11-12): SanDisk SDSR64G, 64 GB, eingebauter SD-Leser, disk4, FAT32 mit MBR, Name G1X3.
  - Zwei Skriptfehler behoben: awk auf macOS liest keine Hex-Zahlen, und `cmp` liefert unter pipefail Code 1.
    Beide Läufe haben vor dem Schreiben abgebrochen. Die drei Bootsektor-Sicherungen `karte/vbr_backup_disk4s1_*.bin` sind bitgleich.
  - Jetzt steht „BOOTDISK“ @0x1E0. „SCRIPT“ @0x1F0 vom CBasic-Dump bleibt stehen.
  - Die Dump-Skripte extend.m und script.req liegen jetzt in `karte/backup_dumpkarte/` und sind von der Karte entfernt.
  - DISKBOOT.BIN liegt auf der Karte (gleich dem Build). Die Karte ist ausgeworfen.
  - **Diese Karte nie in der Kamera formatieren** (sie würde zu exFAT).

### Kameratest Stufe a/b – erste Rückmeldung (Nutzer, 2026-11-12)
- Karte gesperrt, eingeschaltet: Das Canon-Bild kommt sofort. Die **Lampe neben dem Display blinkt beim Starten**, danach Start im **Wiedergabemodus**.
- Deutung (vorläufig):
  - Der Wiedergabemodus passt zum Startmodus-Hack (Ein/Aus losgelassen → 0x400000 = Wiedergabe). Das spricht dafür, dass der CHDK-Neustart gelaufen ist.
  - Die Lampe neben dem Display ist damit vermutlich LED0 (0xD20801DC).
  - Endlos-Blinken (Kompatibilitätsprüfung fehlgeschlagen) ist ausgeschlossen, weil Canon danach normal startet.
- Noch offen: Blinkt die Lampe einmal oder mehrmals? Gegenprobe mit offenem Schieber. Wechsel in den Aufnahmemodus, Menüs, zweiter Start.
- Rückmeldung 2 (Nutzer, 2026-11-12):
  1. Lampe neben dem Display: Mit Karte auf LOCK blinkt sie beim Start ca. 7×, mit offener Karte (reiner Canon-Start) ca. 5×.
     Die zwei zusätzlichen Blinker passen zu Diskboot-Laden und Loader-LED. Danach hört sie auf, also kein Endlos-Blinken (Kompatibilitätsprüfung bestanden).
  2. Halbdruck → Aufnahmemodus, Objektiv fährt aus, Livebild normal.
  3. MENU und Q/SET-Menü normal.
  4. Zweiter Start gleich.
  5. Gegenprobe mit offenem Schieber: normaler Canon-Start im Aufnahmemodus.
- **Ergebnis: Stufe a/b auf der Kamera bestanden** (laden, sauber an Canon übergeben).
  LED0 = 0xD20801DC ist die Lampe neben dem Display (vermutlich, aus dem Blinkunterschied geschlossen).

## Prompt 6 – Stufe c: Lese-Logger (2026-11-12)

Hinweis: Im ursprünglichen Prompt-Plan steht „Sterntaste“. Der Nutzer hat am 2026-11-12 **Q/SET** festgelegt, deshalb ersetzt Q/SET hier überall die Sterntaste.
Außerdem sind S1/S2 bei g1x3 **nicht** in physw_status (Prompt 4). Der Logger liest deshalb zusätzlich den Canon-Zustand von PhySw_ReleaseSw und die beiden GPIO-Eingänge.

### Neue Belege
- **physw_status 0x6c510:** kbd_p1_f/kbd_p1_f_cont 0xe0040a7d/0xe0040a85 ist Befehl für Befehl gleich wie g7x2 0xe005b62b/0xe005b633.
  Dort steht an derselben Stelle physw_status 0x4e440. → verifiziert (vorher nur Sigfinder).
- **ReleaseSw-Zustand:** In FUN_e0039e0c steht `ldr r1,=0x9e3c; ldr r2,[r1,#0xc]` und `str r0,[r1,#0xc]` → Wort **0x9e48**, Werte 0..3. → verifiziert.
- **Open 0xe039dc4d / Close 0xe039dc8b:** Semaphor-Wrapper „FsIO.c“ um Open_low/Close_low.
  - Flags aus Canons fopen_low → FUN_e0573f76: „w“ = **0x301** (O_WRONLY 1 | O_CREAT 0x100 | O_TRUNC 0x200), Modus **0x1b6**.
    Das entspricht CHDK `include/std/stdlib.h` für DryOS. → verifiziert.
- **Write 0xe039e11d:** Eventproc „Write_FW“ (+ j_Write_FW 0xe039de21). Aufbau wie g7x2 Write 0xe0373e3d: Start-Hook(len), Low-Write, Fehlerpfad bei Kurzschreiben, End-Hook. → verifiziert.
- **SleepTask 0xdffc598f:** Assert-Prüfungen, dann ms → Ticks (dffcbec4) + Warten (dffc6cc0). Canon ruft damit Sleep(20) in SdPower auf. → verifiziert.
- **GetSystemTime 0xe057bb2b:** Wrapper `_GetSystemTime(&t); return t;`. Passt zu CHDK get_tick_count (DryOS: Rückgabewert). → verifiziert (Einheit ms laut CHDK, Gegenprobe im Log).
- **CreateTask 0xdffc5adf:** Prompt 4. task_InitFileModules wird in FUN_e0456a5c über CreateTask (0xdffc5ade) angelegt, läuft also durch unseren Hook. → verifiziert.
- **task_InitFileModules 0xe0456a0d** ist identisch mit g7x2 0xe04200b1, ohne den MBR-Patch, den g1x3 nicht braucht.
- **Schreibschutz:** In der Firmware gibt es nur GetSDProtect_FW (physw-Bit → UI, Strings „SD Protect“/„SD Not Protect“).
  Eine Prüfung im Dateisystem habe ich nicht gefunden. CHDK schreibt auf gesperrte Karten üblicherweise problemlos. Restrisiko: Fehlt KEYLOG.TXT, wäre die Sperre eine mögliche Ursache.

### Aufbau (`chdk/g1x3_stufe_c/`, auf Basis des getesteten Stufe-a/b-Kerns)
- boot.c = Stufe a/b plus:
  - CreateTask-Hook ersetzt zusätzlich task_InitFileModules → init_file_modules_task_my (Kopie + `logger_fs_ready()` nach sub_e04e46fc)
  - task_Startup_my: `logger_start()` nach sub_e004074a (PhySw)
- logger.c, Task „KeyLog“ (Prio 0x19, Stack 0x1000):
  - wartet, bis die Karte eingehängt ist, dann 1 s
  - alle 10 ms lesen: physw_status[0..2], Wort 0x9e48, GPIO 0xd20821d8/0xd20821d4 Bit 16
  - bei Änderung eine Zeile in einen 64-KB-RAM-Puffer
  - A/KEYLOG.TXT komplett neu schreiben (Open 0x301), wenn 1 s Ruhe war oder 5 s seit dem letzten Schreiben vergangen sind
  - Puffer voll → „LOG VOLL“, danach keine Zeilen mehr. **Datei ≤ 64 KB.**
  - **Kein Schreibzugriff auf Tastenzustand, Canon-Variablen oder Register.**
- Canon-Heap beginnt jetzt bei 0x34a130 statt 0x339448 (65 KB weniger). Die Prüfung ≥ 0xee64c bleibt erfüllt.
- Build `./chdkmake.sh make -C g1x3_stufe_c`, Kern 3280 Byte + 65560 BSS, DISKBOOT.BIN 3665 Byte. libgcc wird nur für die Division gelinkt (__aeabi_uidivmod).

### Prüfungen am Build
1. Kopierte Funktionen = Dump: alle 8 (inkl. init_file_modules_task_my), Abweichungen nur an den markierten Stellen.
2. 67 sub_-Stubs und 6 NHSTUBs: Kodierung, Ausrichtung und Ziel korrekt.
3. Hook: patch_CreateTask @0x339572 liegt auf 2 mod 4. Die Literale enthalten task_TricInitTask/task_InitFileModules und ihre Ersatzfunktionen mit Thumb-Bit.
4. Logger-Code: Open(„A/KEYLOG.TXT“, 0x301, 0x1b6), Lesen von 0x6c510, 0x9e48, 0xd20821d8, 0xd20821d4. Keine Schreibbefehle an Firmware-Adressen.
5. DISKBOOT.BIN dekodiert = Loader, und der Kern ist darin enthalten.
6. Sperrliste: Die 76 aufgerufenen bzw. benannten Adressen haben keine Schnittmenge.

### Adressliste nach Regel 5 (Stufe c) – Status aller Einträge: verifiziert
Die gesamte Liste von Stufe a/b (Prompt 5) gilt weiter. Neu dazu kommen:
| Adresse | Art | Beleg | Status |
|---|---|---|---|
| 0xe0456a0d | Vergleich im Hook (task_InitFileModules) | Prompt 4, FUN_e0456a5c | verifiziert |
| e03a5a2a, e04e46d4, e039d90c (PostLogicalEventToUI), e04e46fc | aufrufen (1:1 aus task_InitFileModules) | Originalbefehle | verifiziert |
| 0xdffc5adf | aufrufen (CreateTask „KeyLog“) | Prompt 4 | verifiziert |
| 0xdffc598f | aufrufen (SleepTask) | siehe oben | verifiziert |
| 0xe039dc4d / 0xe039e11d / 0xe039dc8b | aufrufen (Open/Write/Close) | siehe oben | verifiziert |
| 0xe057bb2b | aufrufen (GetSystemTime) | siehe oben | verifiziert |
| 0x6c510..0x6c518 | nur lesen | physw_status | verifiziert |
| 0x9e48 | nur lesen | ReleaseSw-Zustand | verifiziert |
| 0xd20821d8 / 0xd20821d4 | nur lesen (GPIO-Eingang S1/S2) | FUN_e0039d18 (Prompt 4) | verifiziert |

**Ergebnis: kein Eintrag „vermutet“. Ein Kamera-Build ist nach Regel 5 zulässig.**

### S2: Ableitung und Sicherheit
- **Aus dem g1x3-Code (Prompt 4):** S2 = GPIO 0xd20821d4, Bit 0x10000, 0 = gedrückt. Interrupt 0xfa, Pegel 0xd501303c, Nachricht 2 an ReleaseSw.
  Im Canon-Zustand ist es Bit1 von 0x9e48, Codes 0x80000001 PressSW2 / 1 UnpressSW2.
- **g7x2-Vergleich:** Dort laufen S1/S2 über physw idx0 (0x00100000/0x00080000), also auf einem anderen Weg.
  Gemeinsam sind die UI-Codes: Event 0x00/0x01 = PressSW1/PressSW2, bei g1x3 kodiert als 0x80000000/0x80000001.
  Der g7x2-Vergleich bestätigt also die Bedeutung der Codes, nicht die Leitung.
- **Sicherheit:** hoch, dass GPIO 0xd20821d4 und IRQ 0xfa der S2-Kontakt sind. S1 und S2 sind in FUN_e0039d18 paarweise und symmetrisch eingerichtet.
  Der Logger prüft die S1-Hälfte direkt an der Hardware: Halbdruck → S1-GPIO 0 und RSW Bit0.
  Verhält sich S1 genau wie vorhergesagt, ist die symmetrische S2-Hälfte sehr wahrscheinlich ebenfalls richtig. Sicherheit dann etwa 90 %.
- **Gegenprobe ohne Risiko:**
  1. Im Log beim vollen Durchdrücken auf ein Flackern von S2=0 achten. Ein defekter Kontakt schließt oft kurz → direkter Beleg.
  2. In Stufe d ist die Einspeisung selbst ungefährlich. Schlimmstenfalls passiert nichts, oder es entsteht ein Foto.
     Getestet wird zuerst im Aufnahmemodus. **Achtung, seit Stufe c2 speichert Canon auch auf der LOCK-Karte** (Fotos landen auf der Karte).
     Zurück jederzeit per Akku raus.

### Kameratest Stufe c (Nutzer, 2026-11-12): kein KEYLOG.TXT auf der Karte
Der Nutzer hat das Testprotokoll durchgeführt. Auf der Karte lagen danach nur CBDUMPER.LOG, DISKBOOT.BIN (3665 B) und PRIMARY.BIN.
**Ursache (belegt):** Mit dem Schieber auf LOCK hält Canon die Karte für schreibgeschützt.
- `GetSDProtect_FW` @0xe0040a19 lädt `physw_status` (0x6c510) und springt nach 0xe003b0fc.
  Dort wird der Tabellenzeiger @0xe003b178 = **0xe063ee34** (physw-Tabelle) gelesen. Das Byte an +8 (Eintrag 0xe063ee3c,
  info 0x01000205, Event 0x30a) ist 0x05, also Wort 0, Bit 5. Zurückgegeben wird **(physw_status[0] >> 5) & 1**.
- Das passt zum Sigfinder (`stubs_entry.S.err`: SD_READONLY_FLAG 0x20, IDX 0, levent 0x30a) und zu g7x2 (0x20, IDX 1, levent 0x30a).
- Jeder CHDK-Port löscht dieses Bit nach jedem Einlesen (`core/kbd_common.c:290`). Ohne das kann auf einer LOCK-Karte nichts
  geschrieben werden. Das gilt auch für **Fotos im Endprodukt**. Stufe c hatte diesen Schritt bewusst nicht (reiner Leser).
- Der erste Lesevorgang der Firmware (FUN_e004030c, aus sub_e0558dc2, einmalig vor PhySw) bleibt unverändert, wie bei g7x2.
  Den ersten echten Durchlauf von PhySw sieht Canon schon ohne das Bit (Wechsel → Event 0x30a).

## Prompt 6 – Stufe c2: Lese-Logger + SD-Schreibschutzbit löschen (2026-11-12)
Gebaut, **noch nicht auf der Kamera**. Ordner `chdk/g1x3_stufe_c2/`, Build `./chdkmake.sh make -C g1x3_stufe_c2`.
DISKBOOT.BIN 4033 B, sha256 744ce4a2…c3f3.
- **Einzige Änderung am Canon-Ablauf:** PhySw läuft in `mykbd_task`, einer 1:1-Kopie von task_PhySw.
  Darin ruft `wrap_kbd_p1_f` (der Anfang von kbd_p1_f) `my_kbd_read_keys` auf: Firmware-kbd_read_keys,
  dann `physw_status[0] &= ~0x20`. Weiter geht es in der Firmware bei kbd_p1_f_cont.
  **Alle Tastenbits bleiben unverändert.** Kein ALT-Modus, kein CHDK-Core.
- Logger wie Stufe c. p0 im Log ist jetzt das Rohwort der Hardware (`physw0_raw`). Bei LOCK muss dort Bit 0x20 = 1 stehen.
- Neues Lebenszeichen ~3 s nach dem Einhängen der Karte: **LED 2 s an** = Datei geschrieben, **2 s schnelles Blinken** = Open/Write fehlgeschlagen,
  **gar nichts** = Logger hat das fs_ready-Signal nie bekommen.
- Prüfungen: Disassembly von sub_e004074a_my, mykbd_task, wrap_kbd_p1_f und my_kbd_read_keys gegen die Firmware verglichen.
  Literal mykbd_task = 0x339a05 (Thumb-Bit gesetzt). `ldr r4,=0x82b4` wird zu movw (gleicher Wert). Keine offenen Symbole.

### Adressliste nach Regel 5 (Stufe c2, neue Einträge) – Status aller Einträge: verifiziert
Alle Einträge aus Stufe a/b und c gelten unverändert (siehe oben). Neu:
| Adresse | Was | Beleg | Status |
|---|---|---|---|
| sub_e004074a → sub_e004074a_my | PhySw anlegen, 1:1 kopiert. Ersetzt ist nur r3 = mykbd_task statt 0xe0040725 | capdis 0xe004074a..0xe004077e, einziger Aufrufer task_Startup @0xe00204fc | verifiziert |
| sub_e003cf80, sub_e002f038 (IsFactoryMode), sub_e003cf24 (OpLog.Start), sub_dffcc3d6 (CreateTaskStrictly_alt), sub_e0039f7c | unverändert aus der Kopie | capdis + funcs_by_address.csv | verifiziert |
| 0x82b4 | PhySw-Steuerblock: [0] Laufflag, [4] Task-ID, [8] Wartezeit | Literal @0xe0040af0, Nutzung in task_PhySw und sub_e004074a | verifiziert |
| 0xe0040af8 | String "PhySw" | Dump-Bytes | verifiziert |
| task_PhySw 0xe0040725 | Vorlage für mykbd_task: Schleife Sleep([r4+8]) → kbd_p1_f → ==1 ? kbd_p2_f → [r4] != 0 ? weiter : ExitTask | Ghidra-Disassembly 0xe0040724..0xe0040746 | verifiziert |
| SleepTask 0xdffc598f | Aufruf in task_PhySw @0xe004072c (bl 0xdffc598e) | = Stub aus Stufe c | verifiziert |
| kbd_p2_f 0xe0040478 | Aufruf in task_PhySw @0xe0040738 | Sigfinder kbd_p2_f 0xe0040479 | verifiziert |
| ExitTask 0xdffc5cd6 | Ende von task_PhySw (b.w @0xe0040746) | funcs_by_address.csv ExitTask 0xdffc5cd7 | verifiziert |
| kbd_p1_f 0xe0040a7d | `push {r1-r7,lr}; movs r4,#0; bl kbd_read_keys`, wird in wrap_kbd_p1_f nachgebildet | capdis | verifiziert |
| kbd_p1_f_cont 0xe0040a85 | Rest von kbd_p1_f, endet mit `pop {r1-r7,pc}`. Liest die Hardware nicht erneut | capdis 0xe0040a84..0xe0040aec | verifiziert |
| kbd_read_keys 0xe0040a47 | physw_status[0..2] = 0, GetKbdState, kbd_read_keys_r2. Aufrufer: nur kbd_p1_f und die einmalige Erstlesung FUN_e004030c | capdis + Ghidra-Xrefs | verifiziert |
| physw_status[0] Bit 0x20 | SD-Schreibschutz (1 = geschützt), wird gelöscht | GetSDProtect_FW → 0xe003b0fc, Tabelle 0xe063ee34+8 (Byte 0x05) | verifiziert |
| LED 0xD20801DC (0x024D0002 / 0x024C0003) | Lebenszeichen nach dem ersten Schreibversuch | Prompt 5 (Loader) | verifiziert |

### Kameratest Stufe c2 (Nutzer, 2026-11-12): bestanden, Log ausgewertet
Log gesichert: `logs/KEYLOG_stufe_c2_20261112_151047.TXT` (206 Zeilen, 4,8–39,7 s nach dem Start). Der Nutzer hat außerdem ein Video aufgenommen
(DCIM/241___11/MVI_4316.MP4). Canon schreibt jetzt also auf die LOCK-Karte. **Der Schreibschutz-Fix wirkt.**
- **LED-Zeichen nicht gesehen**, obwohl das Schreiben geklappt hat (erste Zeile 4770 ms = Zeitpunkt des Signals).
  0xD20801DC ist im laufenden Betrieb also **nicht als sichtbares Lebenszeichen belegt**. Canon steuert die Lampe vermutlich selbst.
  Für Diagnosen künftig nicht auf diese LED verlassen.
- **SD-Schreibschutz:** p0 (Rohwert) = 0x0000B820 durchgehend, Bit 0x20 = 1 bei LOCK → bestätigt „1 = geschützt“.
- **Zeitbasis:** GetSystemTime in ms, alle Stempel sind Vielfache von 10 ms (Abfrage alle 10 ms).

| Taste | Wo | Polarität | Beleg im Log |
|---|---|---|---|
| **S1 (Halbdruck)** | GPIO 0xd20821d8 Bit 0x10000; Canon-Zustand 0x9e48 Bit0; zusätzlich Roh-Bit **p2 0x00000002** | GPIO und p2: **0 = gedrückt**. RSW Bit0: **1 = gedrückt** | 7 Halbdrücke, z. B. 7340–8750, 10120–11780. GPIO, RSW und p2-Bit wechseln immer gemeinsam (±1 Probe). Für p2 0x2 gibt es keinen Eintrag in der physw-Tabelle, Canon erzeugt daraus also kein Event |
| **Q/SET** | **p2 0x00002000** | **0 = gedrückt** | allein: 16710–17110 und 18180–18320 (S1 los) |
| **S1 + Q/SET** | | | S1 18810–21010, darin Q/SET 20340–20950 (S1 vorher 1,5 s stabil) → genau der Fall für Stufe d |
| **Videotaste (Kontrolle)** | **p1 0x00010000** (Tabelle 0xe063eea4, Event 0x02 = g7x2 KEY_VIDEO) | **0 = gedrückt** | 24480–25220 und 28300–28680 (Video Start/Stopp → MP4) |
| **S2** | GPIO 0xd20821d4, RSW Bit1, vermutlich auch p2 0x00000001 | – | **In keiner einzigen Probe gedrückt.** Auch beim Durchdrück-Versuch kein Flackern: Der Kontakt ist vollständig tot. p2 Bit0 bleibt durchgehend 1 und ist vermutlich das S2-Gegenstück zu p2 Bit1. **vermutet**, wird nicht gebraucht |

**Folgerungen für Stufe d:**
- Die S2-Ableitung (GPIO 0xd20821d4 / IRQ 0xfa / RSW Bit1) bleibt bei etwa 90 %. Der Weg über S1 ist auf der Hardware bestätigt,
  und die S2-Hälfte ist im Code symmetrisch. Ein echter S2-Kontakt stört die Einspeisung nicht (er kommt nie).
- **Lese-Artefakte:** Einzelne Proben mit p1 = p2 = 0, oder p2 ohne 0x02000000 / 0x00C00000, entstehen, weil kbd_read_keys
  die Wörter erst nullt und dann neu füllt. Der Logger liest in einem anderen Task mitten hinein.
  → In Stufe d physw_status **nur innerhalb von my_kbd_read_keys** auswerten und ändern, nach dem vollständigen Einlesen, wie CHDK.
- Q/SET verstecken = in my_kbd_read_keys p2 Bit 0x2000 auf 1 setzen (los). Das liegt im PhySw-Ablauf, der in c2 schon getestet ist.
- S1 zum Erkennen: RSW Bit0 (0x9e48) oder p2 Bit 0x2, beide im selben Takt wie der GPIO.

**Prompt 6 erledigt.** Prompt 7 (Tastenlogik) beginnt, sobald der Nutzer es sagt und sich für eine S2-Einspeisung (A/B/C, siehe oben) entschieden hat.

## Prompt 7 – Stufe d: S1 gehalten + Q/SET = S2 (Option A, Nutzerentscheidung 2026-11-12)
Gebaut, **noch nicht auf der Kamera**. Ordner `chdk/g1x3_stufe_d/`, Build `./chdkmake.sh make -C g1x3_stufe_d`.
DISKBOOT.BIN 4545 B, sha256 8cadd550…2180. Diff gegenüber Stufe c2: `chdk/g1x3_stufe_d/stufe_d.diff`.

### Aufbau
- **PhySw** (`mykbd_task`, aus c2): `my_kbd_read_keys` in `kbd_d.c` läuft nach dem vollständigen Einlesen und vor kbd_p1_f_cont.
  1. Es löscht p0 0x20 (SD-Schreibschutz, wie c2).
  2. Tastenlogik nach der Tabelle in CLAUDE.md:
     - S1 = Canon-Zustand 0x9e48 Bit0, ≥ 50 ms stabil (GetSystemTime).
     - Q/SET = p2 0x2000 (0 = gedrückt).
     - Neue Q/SET-Flanke bei stabilem S1 → `qset_hidden = 1`, `vs2_active = 1`, TryPostMessageQueue(*(0x9e44), **4**).
     - S1 los → vs2_active = 0, Q/SET bleibt versteckt.
     - Q/SET los → vs2_active = 0 und qset_hidden = 0.
     - Solange qset_hidden gilt: p2 |= 0x2000, Canon sieht Q/SET also „los“.
     - Erst Q/SET, dann S1 → keine Flanke bei stabilem S1 → nichts. Q/SET ging normal an Canon.
- **PhySw_ReleaseSw** (`task_PhySw_ReleaseSw_my` in boot.c): 1:1-Kopie von 0xe0039eef. Angelegt über die 1:1-Kopie `sub_e0039f7c_my`
  (einziger Aufrufer: sub_e004074a_my).
  Zusätze, alle mit `+` markiert:
  - **Nachricht 4:** nur wenn vs2_active und Canon-Zustand Bit0 (S1) gesetzt → `FUN_e0039e0c(0x80000001)`.
    Das ist genau der Aufruf, den FUN_e0039e98 bei S2-Pegel 0 macht. Zustand 1 → 3, Ausgabe PressSW2 (Tabelle 0xe063f074, Byte 0x43).
  - **Nachricht 3 (10-ms-Poll):** Ist vs2_active gesetzt, wird Bit0 des S2-Pegels (0xd501303c) als 0 = gedrückt behandelt.
    Wird vs2_active 0, sieht Canons eigener Poll „S2 los“ und schickt UnpressSW2. Wenn S1 auch los ist, folgt UnpressSW1,
    in derselben Reihenfolge wie beim echten Auslöser.
  - Die Zustandsmaschine FUN_e0039e0c wird weiterhin nur aus diesem einen Task aufgerufen, es gibt also keine Race-Gefahr.
    Echte S1/S2-Interrupts (Nachricht 1/2) laufen unverändert.
- **Logger** wie c2, plus Spalten V = vs2_active und H = qset_hidden. p0/p2 sind Rohwerte. LED-Zeichen entfernt.

### Prüfungen am Build
- objdump von sub_e0039f7c_my und task_PhySw_ReleaseSw_my mit der capdis-Referenz verglichen: identisch bis auf die `+`-Zeilen
  und r3 = task_PhySw_ReleaseSw_my (Literal 0x339a8d, Thumb-Bit gesetzt).
  - Die Zusätze benutzen nur r1/r2. r2 ist dort frei, weil der Schleifenkopf es neu setzt. r0 (S1-Pegel) bleibt unberührt.
- In stubs_auto gibt es kein STUB(e0039f7c) mehr, das Original wird nicht mehr angesprungen. Keine offenen Symbole.

### Adressliste nach Regel 5 (Stufe d, neue Einträge) – Status aller Einträge: verifiziert
Alle Einträge aus a/b, c und c2 gelten unverändert. Neu:
| Adresse | Was | Beleg | Status |
|---|---|---|---|
| loc_e0039f7c → sub_e0039f7c_my | PhySw_ReleaseSw anlegen, 1:1, nur r3 ersetzt | capdis 0xe0039f7c..0xe0039fbe. Einziger Aufrufer: b.w @0xe004077a | verifiziert |
| task_PhySw_ReleaseSw 0xe0039eef → task_PhySw_ReleaseSw_my | Vorlage der Kopie | capdis 0xe0039eee..0xe0039f7a. Xref nur über das Literal in loc_e0039f7c | verifiziert |
| 0x9e3c-Block | [4] Task-ID, [8] Queue, [0xc] Zustand (=0x9e48), [0x10] Startzustand | FUN_e0039d18, loc_e0039f7c, FUN_e0039e0c | verifiziert |
| 0x6f754-Block | Zeitstempel/Zähler S1/S2 (nur in der Kopie benutzt, wie Canon) | ISR/FUN_e0039e98/Poll | verifiziert |
| 0xd5013038 / 0xd501303c | Pegel S1/S2, Bit0 = 0 gedrückt (nur gelesen) | Tabelle 0xe063f084, FUN_e0039d18 | verifiziert |
| ReceiveMessageQueue 0xdffcbfb4, PostMessageQueue 0xdffcc090, CreateTaskStrictly_alt 0xdffcc3d6, DebugAssert 0xdffc5e24 | unverändert aus den Kopien | capdis + funcs_by_address.csv | verifiziert |
| sub_e04f285e, Callback 0xe0039e03 | 10-ms-Timer → Nachricht 3 (unverändert) | capdis | verifiziert |
| FUN_e0039e98, FUN_e0039e0c | Entprellen bzw. Zustandsmaschine | capdis, Tabelle gelesen | verifiziert |
| 0xe063f074 | Übergangstabelle `81 c2 00 00 / 01 43 10 01 / 03 02 02 30 / 03 03 02 21` | Ghidra read_memory | verifiziert |
| 0xe063f084 | Eingänge {0xea, 0xd5013038, Code 0, 1} / {0xfa, 0xd501303c, Code 1, 1}. Press = Code \| 0x80000000 | Ghidra read_memory | verifiziert |
| TryPostMessageQueue 0xdffcc113 | Nachricht 4 an *(0x9e44) | funcs_by_address.csv. Die ISRs 0xe0039cd4/cf6 schicken damit 1/2 an dieselbe Queue | verifiziert |
| Nachricht 4 | neu, vom Original ignoriert (dort `cmp #3; bne loop`) | capdis | verifiziert |
| p2 0x2000 (Q/SET), 0x9e48 Bit0 (S1) | Polarität 0 = gedrückt bzw. 1 = S1 | Kameralog Stufe c2 | verifiziert |
**Keine „vermutet“-Einträge** → Build für die Kamera nach Regel 5 zulässig. Auf die Karte kommt er nur mit Freigabe des Nutzers.

### Testplan Stufe d (Aufnahmemodus, Karte auf LOCK, Canon speichert Fotos auf die Karte)
1. Start wie gewohnt. Kamera normal, Q/SET allein → Q-Menü öffnet sich (Q/SET unverändert).
2. Halbdruck halten (Fokus), ~1 s warten, dann Q/SET kurz drücken → **Foto**. Danach öffnet sich **kein** Q-Menü.
3. Halbdruck halten, Q/SET gedrückt halten (Serienbild-Modus) → Serie, solange beides gehalten wird.
4. Halbdruck halten, Q/SET drücken, dann **zuerst den Auslöser loslassen** → Aufnahme endet. Q/SET danach loslassen → kein Q-Menü.
5. **Erst Q/SET, dann Halbdruck** → kein Foto, das Q-Menü geht auf (normal).
6. Kontrolle: Videotaste, Menü, Wiedergabe normal.
7. Etwa 5 s warten, ausschalten. KEYLOG.TXT zeigt dann für jede Aktion RSW (Bit1 = Canon-S2), V und H.

**Fehlschlag erkennen:** Die Kamera hängt, startet neu, löst ungewollt aus, oder Q/SET bzw. andere Tasten reagieren nicht mehr.
**Zurück:** Akku raus. Dann eine normale Karte rein, oder diese Karte mit Schieber auf „unlock“ → Canon startet normal. Es bleibt nichts dauerhaft verändert.

### Kameratest Stufe d (Nutzer, 2026-11-12): **bestanden** – „das hat alles geklappt“
Log: `logs/KEYLOG_stufe_d_*.TXT` (426 Zeilen, 4,8–87 s). Auf der Karte: 7 Fotos IMG_4317–4320 und 4322–4324 (CR2+JPG)
sowie ein Video MVI_4321.MP4 (Videotaste).
- **7 Kombinationen S1 + Q/SET** (10,6 / 14,5 / 19,0 / 28,0 / 63,5 / 69,1 / 70,8 s) → **genau 7 Fotos**.
  Die Abstände passen zu den Dateizeiten (16:19:10/14/18/26, 16:20:02/08/10).
  - In derselben 10-ms-Probe springt RSW von 1 auf **3** (Canon sieht S2), mit V = 1 und H = 1.
  - Q/SET los → V = 0, eine Probe später RSW 3 → 1 (UnpressSW2 durch Canons Poll), danach S1 normal.
  - Kombination bei 28,0 s 1,2 s gehalten: S2 die ganze Zeit gemeldet (Einzelbildmodus → 1 Foto).
- **Q/SET allein** (8,7 / 9,2 / 30,2 / 39,3 / 80,5 s): RSW unverändert, H = 0 → Q-Menü normal.
- Videotaste 44,0 / 45,7 s → Start/Stopp, MVI_4321.
- Nicht im Log zu sehen, laut Nutzer aber in Ordnung: „S1 vor Q/SET loslassen“ (kein V = 0/H = 1-Zustand im Log) und „erst Q/SET, dann S1“.
  Beides war vermutlich zu schnell für eine eigene Probe. Serienbild nicht geprüft (Einzelbildmodus).
- Echter S2-Kontakt (GPIO 0xd20821d4) blieb durchgehend 1. Die S2-Ableitung ist damit **bestätigt**: Canon reagiert auf den S2-Zustand mit einem Foto.

**Prompt 7 erledigt. Das Projektziel ist auf der Kamera erreicht.** Offen und optional:
- eine Endversion ohne Logger (dann keine KEYLOG.TXT mehr auf der Karte)
- ein Serienbild-Test
- die GIF-Idee

## Endversion und Startbild (2026-11-12, nach Prompt 7)
### Endversion `chdk/g1x3_final/` (gebaut, noch nicht auf der Karte)
= Stufe d ohne Logger: kein KeyLog-Task, kein InitFileModules-Hook (CreateTask_my wieder identisch mit Stufe a/b),
logger.c/Open/Write/Close entfernt. kbd_d.c und die PhySw/ReleaseSw-Kopien sind unverändert (getestet in Stufe d).
DISKBOOT.BIN 2961 B, sha256 35bf072c…06e6, zweimal gebaut mit gleichem Ergebnis, Kern-Blob im Loader geprüft. Diff: `chdk/g1x3_final/final.diff`.

### Startbild – Analyse (Nutzerwunsch: Snoopy statt Canon-Bild, nur RAM)
- Task **„StartupImage“** (task_StartupImage 0xe03a5e09, angelegt über CreateTask in FUN_e03a5e18, läuft also durch unseren Hook).
- Darin ruft FUN_e044c430 einmalig **FUN_e01d5b90 (AvData.c)** auf:
  - Für jede ID 0..7 wird ein RAM-Puffer per umalloc angelegt (Größe aus Tabelle 0xe0ab6d8c, ID 0 = **0x2f93 = 12179 B**).
  - Die ROM-Daten des aktuellen Satzes werden hineinkopiert.
  - Zeiger-Array im RAM: 0xe8844−0x54 = **0xe87f0** [ID]. Gewählter Satz je ID: 0xe8844 [ID].
- ROM-Tabelle FUN_e01fd314(ID, Satz): Einträge zu 36 B {ID, Zeiger, Größe, …} ab 0xe0ab6dac, 0x120 B pro Satz.
  - ID 0, Satz 0 = **Canon-Logo** 0xe0a741a0 (JPEG 640×480, 4:2:0, 5182 B).
  - ID 0, Satz 1 = **„PowerShot G1 X Mark III“** 0xe0a89f94 (JPEG 720×480, 4:2:2, 12179 B).
  - ID 1..7 sind vermutlich Töne (Größen 0x7ba4 … mit 0x15dc0).
- Abruf über FUN_e044c46c(…, Satz) → FUN_e01d5cc8 kopiert neu aus dem ROM, wenn sich der Satz ändert → FUN_e01d5c52 liefert den RAM-Zeiger
  plus die **Größe aus dem ROM-Eintrag**.
- **Folgerung:** Ein eigenes JPEG lässt sich im RAM einsetzen, ohne ROM oder Flash zu berühren.
  - Das JPEG muss in den Puffer passen (≤ 12179 B) und unter der gemeldeten Größe bleiben (Satz 0: 5182 B, Satz 1: 12179 B).
  - Zeitpunkt: nach FUN_e01d5b90 und nach einem eventuellen Neukopieren durch FUN_e01d5cc8, vor dem Anzeigen.
    Dazu task_StartupImage als Kopie ersetzen, oder den gewählten Satz passend vorbelegen.
- **Offen:**
  - Welches der beiden Bilder erscheint beim Einschalten? Das fragen wir den Nutzer.
  - Akzeptiert Canons Dekoder ein fremdes Baseline-JPEG? Test auf der Kamera.
  - Die Adressen dieses Abschnitts stehen noch nicht in einer Regel-5-Liste → **noch kein Kamera-Build**.

### Startbild – Umsetzung `chdk/g1x3_snoopy/` (Endversion + Snoopy; gebaut, noch nicht auf der Karte)
Der Nutzer sieht beim Einschalten das **Canon-Logo** (Satz 0, ID 0, 5182 B). Bild: `startbild/snoopy.png` (vom Nutzer).
JPEG über `startbild/make_startbild.py`:
- 640×480, baseline, 4:2:0 wie das ROM-Logo. Snoopy 360 px hoch, Weichzeichner 1,2, auf Weiß.
- Qualität 12, **4929 B ≤ 5182 B**.
- Das Skript prüft SOF0, Größe, Abtastung und EOI und schreibt `startbild_jpg.c`.

DISKBOOT.BIN 8161 B, sha256 69c5d2e4…cf0e. Diff gegenüber g1x3_final: `chdk/g1x3_snoopy/startbild.diff`.

**Ablauf:**
1. CreateTask_my ersetzt zusätzlich task_StartupImage (0xe03a5e09) durch `task_StartupImage_my`.
2. Diese Kopie entspricht 1:1 0xe03a5e08..0xe03a5e16 und 0xe03a5c1e..0xe03a5c40 (bis einschließlich `bl sub_e044c430`).
3. Dann folgt `startbild_patch()` mit gesicherten r0–r3, dann `ldr pc,=0xe03a5c45` zurück in die Firmware. Register- und Stackzustand sind dort wie im Original.
4. `startbild_patch` schreibt nur, wenn alle Bedingungen gelten:
   - Puffer ≠ 0
   - Satz[0] = 0
   - die ersten 64 Byte im Puffer sind das ROM-Logo

   Dann: JPEG in den RAM-Puffer, Rest bis 0x143e mit 0 füllen, dcache_clean_range.
5. Danach ruft Canon FUN_e017c64e(0x2000, …) auf. Mit der Einstellung 0x4013 = 1 ist der Satz 0, und es wird nicht neu kopiert. Canon dekodiert unseren Puffer mit Größe 0x143e.

**Prüfungen:**
- objdump von CreateTask_my, task_StartupImage_my und startbild_patch: Die Literale lauten 0xe03a5e09, 0x339b21 (Thumb), 0xe03a61ec, 0xe03a5c45, 0xe87f0 und 0xe0a7419f..df.
- dcache_clean_range bekommt r0 = Puffer und r1 = 0x143e.
- Der Kern liegt im Loader-Blob, das JPEG liegt im Kern.

#### Adressliste nach Regel 5 (Startbild, neue Einträge) – Status aller Einträge: verifiziert
| Adresse | Was | Beleg | Status |
|---|---|---|---|
| task_StartupImage 0xe03a5e09 | Ziel des Hooks | FUN_e03a5e18: `ldr r3,=0xe03a5e09; …; bl sub_dffc5ade` (CreateTask_FW, unser Hook) | verifiziert |
| 0xe03a5e08..0xe03a5e16, 0xe03a5c1e..0xe03a5c44 | 1:1 kopiert (LogCameraEvent 0xe03a2b0c, „_SImage“ 0xe03a61ec, sub_e0362fdc, sub_e0482ad8, sub_e018a38c, sub_e03865d2, sub_e03a5bfc, sub_e044c430) | capdis | verifiziert |
| 0xe03a5c45 | Rücksprung (bl sub_e04955e4), Register r4/r5/r6/r8 und sp wie im Original | capdis | verifiziert |
| FUN_e044c430 → FUN_e01d5b90 | AvData-Init (einmalig, Flag), legt Puffer an und kopiert Satz 0 | Ghidra | verifiziert |
| 0xe8844 / 0xe87f0 | Satz-Array bzw. Puffer-Zeiger-Array (= 0xe8844 − 0x54) | capdis FUN_e01d5b90 (`ldr r2,=0xe8844`, `sub sl,#0x54`) | verifiziert |
| 0xe0a741a0, 0x143e | Canon-Logo im ROM und seine Größe (ROM-Eintrag 0xe0ab6dac: {0, 0xe0a741a0, 0x143e}) | read_memory | verifiziert |
| 0x2f93 | Puffergröße ID 0 (FUN_e01fd366(0) → 0xe0ab6d8c) | capdis + Dump | verifiziert |
| dcache_clean_range 0xe03a9971 | Cache zurückschreiben | funcs_by_address.csv, Loader | verifiziert |
**Keine „vermutet“-Einträge.** Nicht beweisbar ohne Kameratest: ob Canons Dekoder unser (standardkonformes) JPEG anzeigt.

#### Testplan Startbild
1. Einschalten wie immer (Karte auf LOCK). Erwartet: **Snoopy statt Canon-Logo**, danach normaler Betrieb.
2. Die Auslöse-Funktion wie in Stufe d kurz prüfen (Halbdruck + Q/SET → Foto, Q/SET allein → Q-Menü).
- **Möglicher Fehlschlag (harmlos):** schwarzes oder kein Startbild, oder ein verzerrtes Bild. Danach läuft die Kamera normal weiter.
- **Echter Fehlschlag:** Die Kamera hängt beim Start → Akku raus, normale Karte (oder Schieber „unlock“) → Canon startet normal.
  Rückfall: `chdk/g1x3_final/DISKBOOT.BIN` (ohne Startbild).

#### Kameratest Startbild (Nutzer, 2026-11-12): **funktioniert** – Snoopy erscheint statt des Canon-Logos
Canons Dekoder akzeptiert das fremde Baseline-JPEG (Pillow, 4:2:0, mit JFIF-Header).
Wunsch danach: schwarzer Hintergrund und etwas kleiner (schärfer). Umgesetzt in `make_startbild.py`:
- Außenfläche per Flood-Fill schwarz, Snoopy weiß
- 320 px hoch, Weichzeichner 1,0, **Qualität 21** (vorher 12), 5037 B
- Nur die Bilddaten haben sich geändert, der Code ist gleich
- DISKBOOT.BIN 8273 B, sha256 80776957…a85c. Noch nicht auf der Karte.
- **Neues Motiv (Nutzer, 2026-11-12):** `startbild/snoopy_kamera.png` (Snoopy mit Kamera und Woodstock auf Blau).
  - Ganzer Bildschirm blau (3,146,250), das Rauschen im Hintergrund ist geglättet.
  - Motiv auf 50 % verkleinert, Weichzeichner 0,4. Unterschrift unten rechts entfernt.
  - **Qualität 29**, 5005 B. Nur die Bilddaten haben sich geändert, der Code ist gleich.

### Startbild bis 12 KB über Satz 1 (2026-11-12, Nutzerwunsch: weiße, nachgezogene Version ohne Artefakte)
**Ausgangslage:** Bei 5182 Byte (Satz 0) hat Snoopy auf 80 % nur Qualität 7 bis 11. Ohne Limit sind etwa 20 KB nötig.

**Gefunden im g1x3-Dump:**
- Es gibt zwei Bildsätze (Startbild-Einstellung 1/2, `FUN_e044c46c` prüft ≤ 2, Satz = Einstellung − 1).
- Satz 1, ID 0 (Eintrag 0xe0ab6ecc) = {0, 0xe0a89f94, **0x2f93**}. Das ist ein JFIF-JPEG mit 12179 Byte, genauso groß wie der RAM-Puffer von ID 0 (Größentabelle 0xe0ab6d8c, Eintrag 0).
- Ablauf: `FUN_e01d5b90` (Init) kopiert immer Satz 0 und setzt 0xe8844 = 0.
  - Die Anzeige `FUN_e017c64e` ruft `FUN_e044c46c(0x2000, Einstellung)` → `FUN_e01d5cc8`.
  - Diese kopiert den Satz nur neu aus dem ROM, wenn `*(0xe8844 + id*4) != Satz` ist (Disassembly 0xe01d5ce0..e6, `FUN_e01fd310` liefert konstant 2).
  - `FUN_e01d5c52` meldet danach Puffer-Zeiger (0xe87f0) und **Größe des gewählten Satzes** (ROM-Eintrag +8, Zähler +0x12 = 0) an den Dekoder.

**Umsetzung (`startbild.c`):**
- Wie bisher nur, wenn der Puffer das Canon-Logo enthält und 0xe8844 = 0 ist.
- Den ganzen Puffer (0x2f93) mit unserem JPEG und Nullen füllen, `dcache_clean_range`, danach **0xe8844 = 1** (Satz 1).
- Einstellung „2“: keine Neukopie, der Dekoder bekommt 12179 Byte gemeldet → Snoopy.
- Einstellung „1“: Canon kopiert das Logo neu → Canon-Logo (harmloser Rückfall).
- Einstellung „Aus“: nichts.
- 0xe8844 ist reiner RAM-Zustand von AvData, keine Einstellung und kein Flash.

**Bild (`make_startbild.py`):**
- Vorlage 8× vergrößert, Linien über max(R,G,B) < 175 erkannt.
- Linien und Flächen getrennt geglättet, Krümel entfernt, Linienfarbe je Linie (Tinte oder Dunkelblau).
- Weißer Hintergrund, Motiv 80 %, erst am Ende verkleinert.
- **Qualität 59, 11996 B** (Limit 12079).

**Prüfung objdump `startbild_patch`:**
- Literale 0xe87f0, 0xe8844 (= +0x54), 0xe0a7419f..df.
- `movw 0x2f93` für Limit und Schleife.
- `dcache_clean_range(r0 = Puffer, r1 = 0x2f93)`, danach `str 1 → 0xe8844`.

**Kern:**
- 14872 B (text 2840 + data 12004). Der Canon-Heap beginnt dahinter (in Stufe c waren es 65 KB mehr, ohne Probleme).
- DISKBOOT.BIN 15233 B, sha256 630103e7…6aea69. **Auf der Karte (2026-11-12)**. Der vorige Build (blau, 5 KB, sha 60b3b3e8…) liegt in `chdk/rueckfall/DISKBOOT_snoopy_blau_5kb.BIN`.

#### Adressliste nach Regel 5 (neue oder geänderte Einträge) – Status: verifiziert
| Adresse | Was | Beleg | Status |
|---|---|---|---|
| 0xe8844 (schreiben, Wert 1) | Satz im Puffer von ID 0, AvData-RAM | `FUN_e01d5cc8` @0xe01d5d3e `str.w r8,[r9,r7,lsl#2]`, r9 = 0xe8844 (Literal 0xe01d5d44) | verifiziert |
| 0xe0ab6ecc | Satz 1, ID 0: {0, 0xe0a89f94, 0x2f93}, Zähler 0 | read_memory (0xe0ab6dac + 0x120, Satz-Abstand in `FUN_e01fd314`) | verifiziert |
| 0x2f93 | Puffergröße ID 0 = Größe Satz 1 | 0xe0ab6d8c (`FUN_e01fd366`: DAT_e01fd384 − 0x20) | verifiziert |
| `FUN_e01d5c52` | meldet die Größe aus dem Satz-Eintrag, Zeiger aus 0xe87f0 | Disassembly 0xe01d5c66..0xe01d5c84 | verifiziert |
Alle übrigen Adressen wie in der Startbild-Adressliste oben. **Keine „vermutet“-Einträge.**
Ohne Kameratest offen: ob das Menü „Startbild“ die Auswahl „2“ anbietet. Fehlt sie, erscheint das Canon-Logo (harmlos).

#### Testplan
1. Kamera mit dieser Karte starten. Im Menü (Einrichtung) „Startbild“ auf **2** stellen (normale Canon-Einstellung).
2. Aus- und wieder einschalten. Erwartet: weißer, nachgezogener Snoopy.
3. Auslöse-Funktion kurz prüfen (Halbdruck + Q/SET).
- Startbild „1“ → Canon-Logo (erwarteter Rückfall). Hängt die Kamera → Akku raus, normale Karte → Canon startet normal.
- Rückfall-Build: die vorige Snoopy-Version (sha 60b3b3e8…, blau, 5 KB) oder `chdk/g1x3_final/`.
- **Neue Bildvariante (Nutzer, 2026-11-12):** schwarzer Hintergrund, weißer Snoopy, kräftig gelber Woodstock (255,214,0).
  - Erkennungsfarben (SRC_WHITE, SRC_YEL) und Ausgabefarben sind getrennt. Vorher wurde Hellgelb sonst als Weiß erkannt.
  - **Qualität 81, 12025 B.** Schwarz ist günstiger, weil die Konturen mit dem Hintergrund verschmelzen.
  - Code unverändert (text 2840). DISKBOOT.BIN 15265 B, sha256 125b3ccf…c62718. Noch nicht auf der Karte.
- **Aufgeräumt (Nutzer: „nicht clean“, 2026-11-12):**
  - Gelb nur bei Woodstock und Weiß nur bei Snoopy (Grenze Vorlage x = 400).
  - Gras und Schatten (dunkelblaue Linien) sowie Schattenstriche unter y 285 entfernt.
  - Feiner grauer Rand (70,70,80; 1,2 px) um die Figur, damit Kamera und Ohr vor Schwarz sichtbar bleiben.
  - **Qualität 75, 12019 B.** DISKBOOT.BIN 15257 B, sha256 09c38529…873cd0. Noch nicht auf der Karte.
  - Zum Vergleich mit `STARTBILD_FORM=ellipse|kreis`: Figur auf weißer Ellipse bzw. weißem Kreis vor Schwarz (Qualität 40 bzw. 34).
    Vorschauen: `startbild/vorschau_{schwarz,ellipse,kreis}.jpg`.
- **15 % kleiner (Nutzer, 2026-11-12):** SCALE 0.68, **Qualität 83, 12000 B.** DISKBOOT.BIN sha256 064df356…831c0b, **auf der Karte**.
  Die vorige weiße Version (sha 630103e7…) liegt in `chdk/rueckfall/DISKBOOT_snoopy_weiss_12kb.BIN`.

### Startmodus Aufnahme und Canon-Schriftzug (Nutzerwunsch 2026-11-12)
**Objektiv fährt aus:**
- Bisher galt der Startmodus-Hack in `sub_e0020108_my` (vom g7x2-Port übernommen): Ein/Aus-Taste GPIO 0x168 noch gedrückt → Aufnahme, sonst Wiedergabe.
- Beim Kartenstart ist die Taste meist schon los, deshalb startete die Kamera immer im Wiedergabemodus (Objektiv blieb drin).
- Neu (nur `chdk/g1x3_snoopy/`): Ist die **Wiedergabetaste** GPIO 0x150 gedrückt → Wiedergabe 0x400000, sonst Aufnahme 0x200000.
- Canon macht dasselbe Mapping in `sub_e0040842` → `sub_e003aafe` (param_1 = 0x168 → 0x200000, param_2 = 0x150 → 0x400000).
- objdump: `ldr r0,=0xd2082150; ldr; tst #0x10000; movne 0x200000; moveq 0x400000; str → 0x9e24`.
- Folge: Wird die Kamera mit der Wiedergabetaste eingeschaltet und die Taste ist beim CHDK-Neustart schon los, startet sie im Aufnahmemodus.
- `chdk/g1x3_final/` ist unverändert (alter Hack).

**Canon-Schriftzug:**
- Aus Canons eigenem Startbild (ROM 0xe0a741a0, decodiert `startbild/canon_logo_rom.png`) ausgeschnitten.
- JPEG-Kanten geglättet (8×, Schwelle), auf 75 % verkleinert, 40 px unter der Figur. Figur und Schriftzug zusammen mittig.
- **Qualität 67, 11958 B.** DISKBOOT.BIN 15193 B, sha256 190104a5…fe83bd. **Auf der Karte (2026-11-12).** Vorgänger (064df356…) in `chdk/rueckfall/DISKBOOT_snoopy_schwarz_ohne_logo.BIN`.

#### Adressliste nach Regel 5 (neu) – Status: verifiziert
| Adresse | Was | Beleg | Status |
|---|---|---|---|
| 0xd2082150 (lesen) | Wiedergabetaste, Bit 0x10000, 0 = gedrückt | `sub_e0039cc6` → `0xe047213a`: `ldr r1,=0xd2082000` (Literal 0xe0472150), `ldr r0,[r0,r1]`, `lsls #15` = Bit 16. Aufruf mit 0x150 in `sub_e0040842`, Ergebnis → param_2 von `sub_e003aafe` → 0x400000 | verifiziert |
| 0x9e24 (schreiben 0x200000/0x400000) | Startmodus-Variable | wie Stufe a/b (Prompt 5), Werte aus `sub_e003aafe` | verifiziert |
**Keine „vermutet“-Einträge.**

#### Testplan
1. Mit Ein/Aus einschalten → Snoopy + Canon-Schriftzug, danach **Aufnahmemodus, Objektiv fährt aus**.
2. Wiedergabetaste gedrückt halten und einschalten → Wiedergabemodus (Taste so lange halten, bis das Bild kommt).
3. Halbdruck + Q/SET → Foto, Q/SET allein → Q-Menü.
- Rückweg wie immer: Akku raus, normale Karte → Canon normal. Rückfall-Builds in `chdk/rueckfall/`.

#### Kameratest 2026-11-12 (Build 190104a5…, Start im Aufnahmemodus + 12-KB-Startbild): **Fehlschlag**
- Nutzer: „Objektiv fährt aus, fährt nach 5 Sekunden wieder ein, blinkt die ganze Zeit, kein Snoopy“.
- Zwei Änderungen seit dem letzten erfolgreichen Test: (1) Startbild über Satz 1 (12 KB, Einstellung „2“), (2) Startmodus Aufnahme.
- Befund im Dump: 0x9e24 (= 0x9e1c+8) wird nur von `sub_e003aafe` geschrieben und von `kbd_read_keys_r2` bei jeder Abfrage
  in physw eingeblendet (ODER). Es wird nie gelöscht (Xrefs auf das Literal 0xe003ad30: nur aafe, ab92, abd4, ac2c, kbd_read_keys_r2).
  Canon setzt dort zusätzlich Bits aus GPIO 0x16c (0x100000), 0x2c0 (0x20000), 0x194 (0x80000, Zuweisung) und 0x1e4 (0x40000).
  Der CHDK-Hack schreibt nur 0x200000 bzw. 0x400000. Ob das beim Aufnahme-Start stört, ist **offen**.
- **Diagnose-Build:** `START_REC` aus (alter Hack, Start in Wiedergabe), 12-KB-Startbild mit Schriftzug bleibt.
  DISKBOOT.BIN sha256 d23beff9…2c12f2, **auf der Karte (2026-11-12)**. Erscheint Snoopy → Startbild ok, Ursache ist der Aufnahme-Start. Sonst liegt es am Satz-1-Trick.
- **Ergebnis Diagnose-Build (Nutzer, 2026-11-12): Snoopy erscheint** (12 KB über Satz 1, Einstellung „2“, mit Canon-Schriftzug).
  → Der Satz-1-Trick funktioniert. Der Fehlschlag (Objektiv fährt nach ~5 s ein, Lampe blinkt dauernd, kein Startbild) kommt vom **Start im Aufnahmemodus** (0x9e24 = 0x200000 ohne gedrückte Ein/Aus-Taste).
- Nächster Test ohne neuen Build: Mit dem alten Hack beim Einschalten die Ein/Aus-Taste gedrückt halten, bis das Bild kommt → 0x200000 wie beim CHDK-Standardweg.
  Klappt das ebenfalls nicht, ist der Aufnahme-Start in diesem Port grundsätzlich kaputt (nicht nur mein Hack).
- **Test mit gehaltener Ein/Aus-Taste (Nutzer, 2026-11-12): läuft normal** (Snoopy, Aufnahmemodus, Objektiv bleibt draußen).
  → Startmodus 0x200000 funktioniert, solange die Ein/Aus-Taste beim Start wirklich gedrückt ist.
    Nur das erzwungene 0x200000 bei losgelassener Taste (`START_REC`) führt zu Einfahren, Dauerblinken und keinem Startbild.
    Vermutung (nicht belegt): Ein anderer Teil des Startablaufs prüft die echte Taste (GPIO 0x168) und bricht bei Widerspruch ab.
  → **Stand `chdk/g1x3_snoopy/` = Endversion:** `START_REC` aus. Startbild 12 KB mit Canon-Schriftzug (Einstellung „2“).
    Start in Wiedergabe; Ein/Aus halten, bis das Bild kommt → Aufnahmemodus. DISKBOOT.BIN sha256 d23beff9…2c12f2 (auf der Karte).
  Automatischer Aufnahme-Start ohne Halten ist **offen** (bräuchte z. B. einen Moduswechsel nach dem Start; nicht untersucht).
