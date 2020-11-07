#
# QEmacs sample Makefile to develop plugins
#
# QEmacs plugins can contain modes, bindings, or any other qemacs
# dynamically registerable resource.
#

# modify to get the path of the qemacs sources
QEMACS_PATH=../

RECT_LIB=rect_op_plugin.so
RECT_OBJS=rect_op_plugin.o

CSCOPE_LIB=cscope_plugin.so
CSCOPE_OJBS=cscope_plugin.o

INCLUDES=-I$(QEMACS_PATH)
DEFINES=-DQE_MODULE
LIB_INSTALL_PATH:=$(HOME)/.qe/
CC=gcc
CFLAGS=-O2 -Wall -g -fPIC $(INCLUDES) $(DEFINES)

TARGETS= $(RECT_LIB) $(CSCOPE_LIB)

# by default, the plugin is copied in ~/.qe/ some that qemacs can load
# it automatically.
all: $(RECT_LIB) $(CSCOPE_LIB)

$(RECT_LIB): $(RECT_OBJS) Makefile rect_op_plugin.c
	$(CC) $(CFLAGS) -c -o $(RECT_OBJS) rect_op_plugin.c
	@echo "(LINK) $@"
	@$(CC) -shared -o $@ $(RECT_OBJS)

$(CSCOPE_LIB): $(CSCOPE_OBJS) Makefile cscope_plugin.c
	$(CC) $(CFLAGS) -c -o $(CSCOPE_OJBS) cscope_plugin.c
	@echo "(LINK) $@"
	@$(CC) -shared -o $@ $(CSCOPE_OJBS)

clean:
	rm -f *.o *.so *~

PHONY: install
install:
	install -m 644 ${RECT_LIB} ${LIB_INSTALL_PATH}/${RECT_LIB}
	install -m 644 ${CSCOPE_LIB} ${LIB_INSTALL_PATH}/${CSCOPE_LIB}
