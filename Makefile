include config.mak

CFLAGS:=-fno-strict-aliasing -Wall -g $(CFLAGS)
# use it for gcc >= 4.7.0
#CFLAGS+=-Wno-unused-but-set-variable
#CFLAGS+=-Werror
ifdef TARGET_GPROF
CFLAGS+= -p
LDFLAGS+= -p
endif
ifdef TARGET_ARCH_X86
#CFLAGS+=-fomit-frame-pointer
ifeq ($(GCC_MAJOR),2)
CFLAGS+=-m386 -malign-functions=0
else
CFLAGS+=-march=i386 -falign-functions=0
endif
endif
DEFINES=-DHAVE_QE_CONFIG_H

########################################################
# do not modify after this

ifdef CONFIG_DLL
LIBS+=-ldl
# export some qemacs symbols
LDFLAGS+=-Wl,-E
endif
LIBS+=-lm

TARGETS+=qe

OBJS=qe.o charset.o buffer.o \
     input.o display.o util.o hex.o list.o cutils.o \
     unix.o tty.o unihex.o clang.o latex-mode.o xml.o \
     bufed.o shell.o dired.o unicode_join.o charsetmore.o \
     charset_table.o indic.o qfribidi.o unihex.o \
     qeend.o

all: $(TARGETS) plugins

.PHONY: plugins
plugins:
	@echo "(MAKE) plugins"
	@make -C plugins all

qe_g: $(OBJS) $(DEP_LIBS)
	@echo "(LINK) $@"
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

qe: qe_g
	@cp $< $@
	@echo "(STRIP) $@"
	@$(STRIP) $@
	@ls -l $@

qe.o: qe.c qe.h qfribidi.h

charset.o: charset.c qe.h

buffer.o: buffer.c qe.h

tty.o: tty.c qe.h

qfribidi.o: qfribidi.c qfribidi.h

%.o : %.c
	@echo "(CC) $<"
	@$(CC) $(DEFINES) $(CFLAGS) -o $@ -c $<

clean:
	make -C plugins clean
	rm -f *.o *~ TAGS gmon.out core \
           qe qe_g qe.exe qfribidi kmaptoqe ligtoqe \
           fbftoqe fbffonts.c

distclean: clean
	rm -f config.h config.mak

install: qe qe.1 kmaps ligatures
	install -m 755 qe $(prefix)/bin/qemacs
	ln -sf qemacs $(prefix)/bin/qe
	mkdir -p $(prefix)/share/qe
	install kmaps ligatures $(prefix)/share/qe
	install qe.1 $(prefix)/man/man1

plugin_install:
	make -C plugins install

TAGS: force
	etags *.[ch]

force:

#
# tar archive for distribution
#

FILES=Changelog COPYING README TODO qe.1 config.eg \
Makefile qe.tcc qemacs.spec \
hex.c charset.c qe.c qe.h tty.c \
html.c indic.c unicode_join.c input.c qeconfig.h \
qeend.c unihex.c arabic.c kmaptoqe.c util.c \
bufed.c qestyles.h buffer.c ligtoqe.c \
qfribidi.c clang.c latex-mode.c xml.c dired.c list.c qfribidi.h \
charsetmore.c charset_table.c cptoqe.c \
libfbf.c fbfrender.c cfb.c fbftoqe.c libfbf.h fbfrender.h cfb.h \
display.c display.h mpeg.c shell.c \
docbook.c unifont.lig kmaps xterm-146-dw-patch \
ligatures \
image.c VERSION \
cutils.c cutils.h unix.c

# qhtml library
FILES+=libqhtml/Makefile libqhtml/css.c libqhtml/cssid.h \
libqhtml/cssparse.c libqhtml/xmlparse.c libqhtml/htmlent.h \
libqhtml/css.h libqhtml/csstoqe.c \
libqhtml/docbook.css libqhtml/html.css 

FILE=qemacs-$(shell cat VERSION)

tar:
	rm -rf /tmp/$(FILE)
	mkdir -p /tmp/$(FILE)
	cp -r . /tmp/$(FILE)
	( cd /tmp ; tar zcvf $(FILE).tar.gz $(FILE) )
	rm -rf /tmp/$(FILE)
