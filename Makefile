prefix=/usr/local
MAKE=make
CC=gcc
GCC_MAJOR=3
HOST_CC=gcc
AR=ar
STRIP=strip -s -R .comment -R .note
CFLAGS=-O2
LDFLAGS=
EXTRALIBS=-lm -ldl
VERSION=0.3.4

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
APP_NAME=hoe

########################################################
# do not modify after this

LIBS+=-ldl
# export some qemacs symbols
LDFLAGS+=-Wl,-E
LIBS+=-lm

TARGETS+=$(APP_NAME)

OBJS=qe.o charset.o buffer.o \
     input.o display.o util.o hex.o list.o cutils.o \
     unix.o tty.o unihex.o clang.o latex-mode.o xml.o \
     bufed.o shell.o dired.o unicode_join.o charsetmore.o \
     charset_table.o \
     cscope.o rect_operations.o \
     qeend.o

all: $(TARGETS) plugins

.PHONY: plugins
plugins:
	@echo "(MAKE) plugins"
	@make -C plugins all

$(APP_NAME): $(OBJS) $(DEP_LIBS)
	@echo "(LINK) $@"
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

qe.o: qe.c qe.h

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
           $(APP_NAME) qfribidi

install: $(APP_NAME)
	install -s -m 755 $(APP_NAME) $(prefix)/bin/$(APP_NAME)

plugin_install:
	make -C plugins install

TAGS: force
	etags *.[ch]

force:

#
# tar archive for distribution
#

FILES=Changelog COPYING README qe.1 config.eg Makefile \
hex.c charset.c qe.c qe.h tty.c indic.c unicode_join.c input.c \
qeconfig.h qeend.c unihex.c util.c bufed.c qestyles.h buffer.c \
qfribidi.c clang.c latex-mode.c xml.c dired.c list.c qfribidi.h \
charsetmore.c charset_table.c display.c display.h shell.c VERSION \
cutils.c cutils.h unix.c

FILE=$(APP_NAME)-$(shell cat VERSION)

tar:
	rm -rf /tmp/$(FILE)
	mkdir -p /tmp/$(FILE)
	cp -r . /tmp/$(FILE)
	( cd /tmp ; tar zcvf $(FILE).tar.gz $(FILE) )
	rm -rf /tmp/$(FILE)
