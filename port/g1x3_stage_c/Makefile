# STAGE c test build (read-only logger) g1x3 100a (GM1.00A)
# Builds: mini core (Canon restart only, boot.c) -> loader/g1x3 -> DISKBOOT.BIN (dancingbits 17)
# No CHDK core, no key hook. In addition: task "KeyLog" (read only) -> A/KEYLOG.TXT
# Usage: ./build.sh stage_c
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

main.elf: entry.o boot.o logger.o stubs_auto.o stubs_c.o
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
