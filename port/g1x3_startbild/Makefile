# ENDVERSION + EIGENES STARTBILD g1x3 100a (GM1.00A): wie g1x3_final, zusaetzlich eigenes JPEG statt Canon-Logo.
# Baut: Mini-Kern (boot.c, kbd_d.c, startbild.c, startbild_jpg.c) -> loader/g1x3 -> DISKBOOT.BIN (dancingbits 17).
# startbild_jpg.c erzeugt ../startbild/make_startbild.py. In der Kamera "Startbild: 2" einstellen.
# Aufruf: ./build.sh startbild   (im Docker: make -C g1x3_startbild)
topdir=../
PLATFORM=g1x3
PLATFORMSUB=100a
export PLATFORM PLATFORMSUB
include $(topdir)makefile_cam.inc

LDOPTS=-nostdlib -Wl,--allow-shlib-undefined -Wl,-EL,-T,$(tools)/link-boot.ld -Wl,-N,-Ttext,$(MEMISOSTART)

all: DISKBOOT.BIN

stubs_auto.S: boot.c
	@echo $^ '->' $@
	cat $^ | \
	    grep -E '^[[:space:][:alpha:](]*["][[:space:]]*[Bb][a-wA-W]*[\.]*[a-wA-W]*[[:space:]]+sub_[0-9A-Fa-f]+[[:space:]]*[\\/"]' | \
	    sed 's/.*[Bb][[:alpha:]\.]*[[:space:]][[:space:]]*sub_\([0-9A-Fa-f]*\).*/\1/' | \
	    $(SORT) | uniq | \
	    awk 'BEGIN {print "/* THIS FILE IS GENERATED */\n\n#include \"stubs_asm.h\"\n"} \
	        {printf("STUB(%s)\n",$$1)}' > stubs_auto.S

boot.o: boot.c
	@echo $< '->' $@
	$(CC) $(CFLAGS) -nostdinc -c -o $@ $<

main.elf: entry.o boot.o kbd_d.o startbild.o startbild_jpg.o stubs_auto.o stubs_c.o
	@echo '->' $@
	$(CC) $(CFLAGS) -o $@ -Wl,--start-group $^ -lgcc -Wl,--end-group $(LDOPTS)
	( $(NM) $@ | grep ' U ' > $@.syms ) && echo "error: unresolved symbols in $@ (see $@.syms)" && exit 1 || exit 0
	$(SIZE) $@

main.bin: main.elf

loader: main.bin
	$(MAKE) -C $(topdir)loader/g1x3 clean
	$(MAKE) -C $(topdir)loader/g1x3 CORE_FILE=$(CURDIR)/main.bin all

$(tools)/dancingbits$(EXE):
	$(MAKE) -C $(tools) dancingbits$(EXE)

DISKBOOT.BIN: loader $(tools)/dancingbits$(EXE)
	@echo dance '->' DISKBOOT.BIN ver $(NEED_ENCODED_DISKBOOT)
	$(ENCODE_DISKBOOT) $(topdir)loader/g1x3/main.bin DISKBOOT.BIN $(NEED_ENCODED_DISKBOOT)

clean:
	rm -f *.o *.bin *.elf *.syms *.dump stubs_auto.S DISKBOOT.BIN
	$(MAKE) -C $(topdir)loader/g1x3 clean

.PHONY: all loader clean
include $(topdir)bottom.inc
