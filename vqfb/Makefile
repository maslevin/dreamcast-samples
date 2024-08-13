# KallistiOS 2.0.0
#
# vqfb/Makefile
# (c)2015,2022 Matt Slevinsky
#

KOS_ROMDISK_DIR = romdisk
include $(KOS_BASE)/Makefile.rules

SRCS = main.cpp sample.cpp simplexnoise.cpp simplex_pal.cpp simplex_vqfb.cpp fire_vqfb.cpp
OBJS = $(SRCS:.cpp=.o)
PROJECTNAME = vqfb
ELFFILE = $(PROJECTNAME).elf
BINFILE = $(PROJECTNAME).bin

#DEBUG BUILD SETTINGS
DBGDIR = debug
DBGEXE = $(DBGDIR)/$(ELFFILE)
DBGOBJS = $(addprefix $(DBGDIR)/, $(OBJS))
DBGCFLAGS = -g -O0 -DDEBUG

#RELEASE BUILD SETTINGS
RELDIR = release
RELEXE = $(RELDIR)/$(ELFFILE)
RELBIN = $(RELDIR)/$(BINFILE)
RELOBJS = $(addprefix $(RELDIR)/, $(OBJS))

#CDI BUILD SETTINGS
ISODIR = iso
ISOBIN = $(ISODIR)/1ST_READ.BIN
CDIDIR = cdi
CDIISO = $(CDIDIR)/$(PROJECTNAME).iso
CDIIPBIN = $(CDIDIR)/IP.BIN
CDIBOOTSTRAPISO = $(CDIDIR)/$(PROJECTNAME)-bootstrap.iso
CDIFILE = $(CDIDIR)/$(PROJECTNAME).cdi

.DEFAULT_GOAL := release

prepdbgdir:
	@mkdir -p $(DBGDIR)

prepreldir:
	@mkdir -p $(RELDIR)

prepcdidir:
	@mkdir -p $(CDIDIR) $(ISODIR)

debug: $(DBGEXE)

$(DBGEXE): $(DBGOBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(DBGCFLAGS) $(KOS_LDFLAGS) $(KOS_START) -o $(DBGEXE) $^ -lstdc++ $(KOS_LIBS)

$(DBGDIR)/%.o: %.cpp | prepdbgdir
	$(KOS_CC) -c $(KOS_CFLAGS) -fpermissive $(DBGCFLAGS) $(KOS_LDFLAGS) -o $@ $<

release: $(RELEXE)

$(RELEXE): $(RELOBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) $(KOS_START) -o $(RELEXE) $^ -lstdc++ $(KOS_LIBS)
	$(KOS_STRIP) $(RELEXE)
	$(KOS_OBJCOPY) -R .stack -O binary $(RELEXE) $(RELBIN)

$(RELDIR)/%.o: %.cpp | prepreldir
	$(KOS_CC) -c $(KOS_CFLAGS) -fpermissive $(KOS_LDFLAGS) -o $@ $<

cdi: release | prepcdidir
	/opt/toolchains/dc/kos/utils/scramble/scramble $(RELBIN) $(ISOBIN)
	mkisofs -V $(PROJECTNAME) -l -C 0,11702 -o $(CDIISO) ./$(ISODIR)
	makeip ip.txt $(CDIIPBIN)
	( cat $(CDIIPBIN); dd if=$(CDIISO) bs=2048 skip=16 ) > $(CDIBOOTSTRAPISO)
	cdi4dc $(CDIBOOTSTRAPISO) $(CDIFILE)

clean:
	rm -rf debug release cdi iso