CC := gcc
# CC=x86_64-w64-mingw32-gcc
# CFLAGS = -O2 -static -Ofast -Os -s -g0 -flto
CFLAGS = -g -ggdb 
LDFLAGS = -L. -lxlib -lm 
SRCDIR = src
BINDIR := bin
TARGETS = orta fcfx xd repl xtoa nyva

PCOUNT = 0
GIT_HASH := $(shell git rev-parse HEAD 2>/dev/null || echo "unknown")
OVERSION = 1.0
COMPILE = @echo "[$(PCOUNT)] CC $<"; $(CC) $(CFLAGS) $< $(LDFLAGS) -DGITHASH='$(GIT_HASH)' -D_VERSION=$(OVERSION) -o $(BINDIR)/$@; $(eval PCOUNT=$(shell echo $$(($(PCOUNT)+1))))
INSTALL_DIR = /usr/local/bin

.PHONY: all clean release debug dir static install

all: dir $(TARGETS)

dir:
	@mkdir -p $(BINDIR)

orta: $(SRCDIR)/orta.c $(SRCDIR)/orta.h
	$(COMPILE)

fcfx: $(SRCDIR)/fcfx.c $(SRCDIR)/orta.h
	$(COMPILE)

xd: $(SRCDIR)/xd.c $(SRCDIR)/orta.h
	$(COMPILE)

repl: $(SRCDIR)/repl.c $(SRCDIR)/orta.h
	$(COMPILE)

xtoa: $(SRCDIR)/xtoa.c $(SRCDIR)/orta.h
	$(COMPILE)

nyva: $(SRCDIR)/nyva.c 
	$(COMPILE)

rom: $(SRCDIR)/rom.zig
	zig build-exe src/rom.zig -femit-bin=$(BINDIR)/rom


liborta: bin/liborta.so bin/liborta.a

bin/orta.o: $(SRCDIR)/orta.h 
	gcc -fPIC -x c -c $(SRCDIR)/orta.h -o $(BINDIR)/orta.o

bin/liborta.so: bin/orta.o
	gcc -shared -o bin/liborta.so bin/orta.o

bin/liborta.a: bin/orta.o
	ar rcs bin/liborta.a bin/orta.o

clean:
	rm -rf $(BINDIR)
	rm -f *.pre.x *.xbin xtoa

release:
	xxd -i std.x > $(SRCDIR)/std.h

debug: CFLAGS += -DDEBUG
debug: all

static: CFLAGS += -static
static: all

install:
	@echo "Installing executables to $(INSTALL_DIR)"
	@mkdir -p $(INSTALL_DIR)
	@for target in $(TARGETS); do \
		echo "Installing $$target"; \
		install -m 755 $(BINDIR)/$$target $(INSTALL_DIR)/; \
	done
