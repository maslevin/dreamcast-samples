# KallistiOS 2.0.0
#
# vqfb/Makefile
# (c)2015 Matt Slevinsky
#

all: rm-elf vqfb.elf

KOS_ROMDISK_DIR = romdisk
include $(KOS_BASE)/Makefile.rules

OBJS = main.o simplexnoise.o romdisk.o

clean:
	rm -f vqfb.elf $(OBJS)

rm-elf:
	rm -f vqfb.elf
	rm -f romdisk.img

vqfb.elf: $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $@ $(KOS_START) $^ -lstdc++ $(KOS_LIBS)

run: vqfb.elf
	$(KOS_LOADER) vqfb.elf

dist:
	rm -f $(OBJS) romdisk.*
	$(KOS_STRIP) vqfb.elf
	$(KOS_OBJCOPY) -R .stack -O binary vqfb.elf vqfb.bin
