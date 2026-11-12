# g1x3 (PowerShot G1 X Mark III) - Firmware GM1.00A
# Values and evidence: docs/NOTES_de.md (Prompt 3-5).
PLATFORMOS=dryos
# "DRYOS version 2.3, release #0060+p1" @ dump 0x1A0E8 (verified via string)
PLATFORMOSVER=60
DIGIC = 70
THUMB_FW=1
# start of firmware, from CBDUMPER.LOG "E0000000 dry" (verified)
ROMBASEADDR=0xE0000000

# P-ID 0x32D7 = 13015, halfword @0xE1F20270 in the g1x3 dump (verified, Prompt 5).
# Same location as on g7x2 (there 0x32BC = 12988 = its PLATFORMID).
PLATFORMID=13015

# dancingbits table {4,7,5,6,1,3,2,0} @0xe001a10c = version 17 (verified, Prompt 4)
NEED_ENCODED_DISKBOOT=17

# next line is for the compatibility check, as comment
# PLATFORMID @ 0xe1f20270
# LED0 of Canon's LED init (offset 0x1dc, on 0x024d0002 / off 0x024c0003), see docs/NOTES_de.md Prompt 5
# BLINK_LED_CONTROL=5
# BLINK_LED_GPIO=0xD20801DC
