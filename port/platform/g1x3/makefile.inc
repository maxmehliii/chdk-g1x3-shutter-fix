# g1x3 (PowerShot G1 X Mark III) - Firmware GM1.00A
# Werte mit Beleg siehe /NOTES.md (Prompt 3-5).
PLATFORMOS=dryos
# "DRYOS version 2.3, release #0060+p1" @ dump 0x1A0E8 (verifiziert per String)
PLATFORMOSVER=60
DIGIC = 70
THUMB_FW=1
# start of firmware, aus CBDUMPER.LOG "E0000000 dry" (verifiziert)
ROMBASEADDR=0xE0000000

# P-ID 0x32D7 = 13015, Halbwort @0xE1F20270 im g1x3-Dump (verifiziert, Prompt 5).
# Gleiche Stelle wie bei g7x2 (dort 0x32BC = 12988 = deren PLATFORMID).
PLATFORMID=13015

# dancingbits-Tabelle {4,7,5,6,1,3,2,0} @0xe001a10c = Version 17 (verifiziert, Prompt 4)
NEED_ENCODED_DISKBOOT=17

# next line is for the compatibility check, as comment
# PLATFORMID @ 0xe1f20270
# LED0 der Canon-LED-Init (Offset 0x1dc, an 0x024d0002 / aus 0x024c0003), siehe NOTES.md Prompt 5
# BLINK_LED_CONTROL=5
# BLINK_LED_GPIO=0xD20801DC
