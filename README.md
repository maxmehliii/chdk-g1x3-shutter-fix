# Canon PowerShot G1 X Mark III: Auslöser-Ersatz per CHDK (S2 auf Q/SET)

> **English summary:** The full-press contact (S2) of my G1 X Mark III shutter button broke. Half-press (S1) still worked.
> This is a minimal, RAM-only CHDK port for the G1 X Mark III, firmware **GM1.00A**, that fixes this.
> **Hold half-press, then press Q/SET**, and the camera takes the picture as if the shutter had been pressed fully.
> Everything else behaves exactly like Canon's firmware: no CHDK menu, no ALT mode, no OSD. Nothing is written to flash.
> Optionally you can replace the start-up logo with your own image. Build: `./setup.sh && ./build.sh final`.
> The documentation below is in German. The full reverse-engineering log, with evidence for every address, is in [docs/NOTES.md](docs/NOTES.md).

## Worum es geht

Bei meiner G1 X Mark III ist der zweite Kontakt des Auslösers kaputtgegangen, also das volle Durchdrücken (S2).
Der halbe Druck (S1, Fokus und Belichtung) geht noch. Die Kamera konnte deshalb keine Fotos mehr machen.

Dieses Projekt ersetzt den kaputten Kontakt durch eine andere Taste:

**Auslöser halb drücken und halten, dann Q/SET drücken → die Kamera löst aus.**

Sonst bleibt alles wie bei Canon:
- Q/SET allein öffnet wie immer das Q-Menü.
- Serienbild und Video funktionieren wie mit einem echten Auslöser.
- Es gibt kein sichtbares CHDK: kein Menü, kein ALT-Modus, keine Einblendungen.

Technisch ist es ein **Minimalport von [CHDK](https://chdk.fandom.com/wiki/CHDK)**.
- Die Kamera lädt ihn bei jedem Einschalten von der SD-Karte in den Arbeitsspeicher.
- In den Flash wird nichts geschrieben, auch nicht in Einstellungen oder Kalibrierdaten.
- Ohne die vorbereitete Karte startet die Kamera ganz normal mit Canons Firmware.

Als Extra kann beim Einschalten ein **eigenes Startbild** statt des Canon-Logos erscheinen (ebenfalls nur im RAM).

## Bedienung

| Situation | Was die Kamera sieht |
|---|---|
| Q/SET allein | Q/SET ganz normal (Q-Menü) |
| Auslöser halb gedrückt (mind. ~50 ms), dann Q/SET | halber **und voller** Druck → Foto. Q/SET wird dabei vor Canon versteckt |
| Q/SET gehalten, Auslöser weiter halb gedrückt | voller Druck bleibt (Serienbild, Video wie mit echtem Auslöser) |
| Q/SET losgelassen | voller Druck los, halber Druck wie tatsächlich gedrückt |
| Auslöser zuerst losgelassen | beides los. Q/SET bleibt versteckt, bis es losgelassen ist (kein Q-Menü nach dem Foto) |
| Erst Q/SET, dann halb drücken | **kein Auslösen**. Q/SET ging schon an Canon (Q-Menü) |
| alle anderen Tasten | unverändert |

**Einschalten:**
- Die Kamera startet über die Karte im **Wiedergabemodus**. Halb drücken wechselt wie gewohnt in den Aufnahmemodus.
- Soll sie direkt im **Aufnahmemodus** starten (Objektiv fährt aus), halte die **Ein/Aus-Taste gedrückt, bis das Startbild erscheint**.

## Voraussetzungen und Sicherheit

- **Nur die PowerShot G1 X Mark III mit Firmware GM1.00A.**
  - Der Loader prüft beim Start Plattform-ID und Firmware-Text.
  - Passt eines davon nicht, blinkt die Lampe neben dem Display dauerhaft und CHDK startet nicht. Dann Akku raus.
  - Die Kamera-Firmware nicht aktualisieren, sonst passen die Adressen nicht mehr.
- **Nur RAM.** Der Port ruft keine Funktion auf, die Flash, persistente Parameter oder Kalibrierdaten schreibt.
  Die Liste dieser Funktionen steht in [docs/NOTES.md](docs/NOTES.md) (Prompt 4).
- **Rückweg:** Akku raus, eine normale Karte rein (oder den Schreibschutzschieber auf „offen“). Dann startet Canons Firmware wie immer.
- Nutzung auf eigene Gefahr. Das Projekt hat nichts mit Canon zu tun. Getestet ist es auf **einer** Kamera.

## Nachbau Schritt für Schritt

### 1. Werkzeuge

- macOS oder Linux, `git`, `svn` (Subversion), Docker
  - macOS: `brew install subversion colima docker`, danach `colima start`
- Für eigene Startbilder zusätzlich Python 3 mit `pip install pillow numpy opencv-python`

### 2. CHDK holen und bauen

```bash
git clone <dieses-repository> && cd chdk-g1x3-shutter-fix
./setup.sh            # holt CHDK trunk r6358 nach ./chdk und baut das Docker-Image "chdkbuild"
./build.sh final      # → out/DISKBOOT.BIN (nur Auslöser-Ersatz, Canon-Startbild bleibt)
```

Ein Firmware-Dump ist zum Bauen **nicht** nötig. Alle Adressen stehen schon in `port/`.

Der Build mit CHDK r6358 ist reproduzierbar. Mein auf der Kamera getesteter `final`-Build hat die SHA-256
`35bf072cbedc9b6f74b221f0deed93d7eac9b5cca554f72acb004c974b7306e6`. Dein `out/DISKBOOT.BIN` sollte dieselbe Prüfsumme haben.

### 3. SD-Karte vorbereiten

Die Kamera startet Code von der Karte nur unter drei Bedingungen:

1. **FAT32.** exFAT wird nie als bootfähig erkannt.
   - SDHC-Karten bis 32 GB kannst du in der Kamera formatieren.
   - Größere Karten am Computer als FAT32 mit MBR formatieren.
2. **„BOOTDISK“ an Offset 0x1E0 des FAT32-Bootsektors.** Unter macOS erledigt das `karte/sd_bootdisk.sh`.
   Es prüft vorher, ob es wirklich eine externe SD-Karte mit FAT32 ist, und sichert den Bootsektor:
   ```bash
   diskutil list external              # Disk-ID der Karte finden, z. B. disk4
   ./karte/sd_bootdisk.sh status disk4
   ./karte/sd_bootdisk.sh set disk4    # fragt nach sudo und nach der Disk-ID
   ```
   Unter Linux schreibst du die 8 Bytes `BOOTDISK` an Offset 0x1E0 der Partition (nicht des ganzen Geräts),
   z. B. mit `printf BOOTDISK | sudo dd of=/dev/sdX1 bs=1 seek=480 conv=notrunc`. Vorher unbedingt das richtige Gerät prüfen!
3. **`out/DISKBOOT.BIN` ins Hauptverzeichnis der Karte kopieren**, Karte auswerfen und den **Schreibschutzschieber auf LOCK** stellen.

Der Port löscht das Schreibschutz-Bit in der Tastenabfrage. Canon speichert deshalb trotz LOCK ganz normal Fotos und Videos.
**Diese Karte danach nicht mehr in der Kamera formatieren.** Sonst ist „BOOTDISK“ weg, und Karten über 32 GB werden zu exFAT.

### 4. Testen

1. Karte rein, einschalten. Die Kamera startet normal, über die Karte im Wiedergabemodus.
2. Halb drücken → Aufnahmemodus. Halb gedrückt halten und Q/SET drücken → Foto.
3. Q/SET allein → Q-Menü.

Hängt etwas: Akku raus, normale Karte rein.

## Eigenes Startbild (optional)

Canons Startbild liegt im ROM. Beim Einschalten kopiert die Firmware es in einen RAM-Puffer und zeigt es von dort an.
Der Build `startbild` ersetzt nur diesen RAM-Puffer, und das nur, wenn darin wirklich Canons Bild liegt.
Beim nächsten Start ohne Karte ist alles wieder original.

```bash
python3 startbild/make_startbild.py mein_bild.png                    # Hintergrund schwarz, automatisch eingepasst
python3 startbild/make_startbild.py mein_bild.png --hintergrund ffffff --breite 400
./build.sh startbild
```

**Wichtig:** Im Kamera-Menü **„Startbild“ auf 2 stellen.**
- Canon meldet dem JPEG-Decoder nur bei Startbild 2 die volle Puffergröße von 12179 Byte. Bei Startbild 1 sind es nur 5182 Byte.
- Steht die Einstellung auf 1, erscheint einfach das Canon-Logo. Das ist harmlos.

**Tipps:**
- Das JPEG darf höchstens ~12 KB groß sein. Das Skript sucht automatisch die beste Qualität, die noch passt.
- Flache Farben und klare Linien sehen viel besser aus als Fotos.
- Schwarzer Hintergrund mit hellem Motiv braucht die wenigsten Bytes.
- `startbild/beispiel.png` (eine Blende) ist das Standardbild.

**Canon-Schriftzug unter dem Bild:**
- Er ist nicht im Repository. Das Skript kann ihn aus deinem **eigenen** Firmware-Dump holen:
  `--logo-aus-dump PRIMARY.BIN`.
- Den Dump erzeugt das Canon-Basic-Skript aus dem CHDK-Wiki
  ([Canon Basic/Scripts/Dumper](https://chdk.fandom.com/wiki/Canon_Basic/Scripts/Dumper)).

## Wie es funktioniert

Das ist eine Kurzfassung. Alle Details mit Belegen aus dem Dump stehen in [docs/NOTES.md](docs/NOTES.md).

- **Kartenstart:** Die Firmware startet `DISKBOOT.BIN` von einer FAT32-Karte mit „BOOTDISK“ und Schreibschutz auf LOCK.
  - Die Datei ist mit dancingbits Version 17 verschlüsselt.
  - Der Loader (`port/loader/g1x3`) prüft Kamera und Firmware und kopiert einen kleinen Kern in den RAM.
  - Dieser Kern startet Canons Firmware neu, mit wenigen eingehängten Änderungen. Den großen CHDK-Core gibt es hier nicht.
- **S1/S2 bei der G1 X Mark III:** Die beiden Kontakte laufen nicht über die normale Tastenabfrage `physw_status`.
  Sie laufen über Interrupts und den eigenen Task `PhySw_ReleaseSw` mit Zustandsmaschine.
  - Der Port ersetzt diesen Task durch eine Kopie. Sie versteht eine zusätzliche Nachricht „virtuelles S2 drücken“.
  - Solange das virtuelle S2 aktiv ist, hält die Kopie den S2-Pegel auf „gedrückt“.
- **Tastenlogik** (`kbd_d.c`): Sie läuft in einer Kopie des Tasten-Tasks `PhySw`, direkt nachdem die Firmware die Tasten gelesen hat.
  - Erkennt sie die Q/SET-Flanke bei stabil gehaltenem S1, versteckt sie Q/SET vor Canon und schaltet das virtuelle S2 ein.
  - Loslassen löst alles wieder.
- **Schreibschutz:** Der Kartenstart verlangt LOCK. Deshalb löscht der Port in der Tastenabfrage das Bit, mit dem Canon den Schieber sieht.
- **Startbild:** Siehe oben. Zusätzlich trägt der Port im RAM-Zustand der Firmware ein, dass Bildsatz 2 schon im Puffer liegt.
  Sonst würde Canon den Puffer vor der Anzeige wieder aus dem ROM füllen.
- **Vorgehen in Stufen**, jede auf der Kamera getestet, bevor die nächste begann (`port/g1x3_stufe_*`):
  - a/b: laden und sauber an Canon übergeben
  - c/c2: Tasten nur lesen und auf die Karte loggen (Beispiel-Logs in `docs/logs/`)
  - d: Tasten verändern
  - Grundlage war der CHDK-Port der G7 X Mark II. Jede Adresse wurde im eigenen G1-X-III-Dump neu belegt.

## Ordner

| Ordner | Inhalt |
|---|---|
| `port/` | der g1x3-Port. `build.sh` kopiert ihn in den CHDK-Baum `./chdk` |
| `port/g1x3_final/` | Endversion: nur Auslöser-Ersatz |
| `port/g1x3_startbild/` | Endversion + eigenes Startbild |
| `port/g1x3_stufe_*/` | Teststufen aus der Entwicklung |
| `port/loader/g1x3/`, `port/platform/g1x3/` | Loader und Plattform-Dateien (Stubs, Funktionslisten) |
| `startbild/` | Startbild-Skript und Beispielbild |
| `karte/` | Skript, das die SD-Karte bootfähig markiert (macOS) |
| `docs/` | Entwicklungsprotokoll mit allen Adresslisten, Tasten-Logs |

## Lizenz und Dank

- **Lizenz:** GPL-2.0, wie CHDK (siehe `LICENSE`).
- **Dank:** an das CHDK-Projekt und an die Autoren des G7-X-Mark-II-Ports, der als Vorlage diente.
- **Nicht enthalten:** Canon-Firmware oder Teile davon als Datei (Dump, Bilder). „Canon“ und „PowerShot“ sind Marken von Canon Inc.
