CC = gcc
CFLAGS = -g -ggdb 
LDFLAGS = -L. -lxlib 
SRCDIR = src
BINDIR = bin

TARGETS = orta fcfx xd repl xtoa

.PHONY: all clean release debug dir

all: dir $(TARGETS)

dir:
	@mkdir -p $(BINDIR)

orta: $(SRCDIR)/orta.c $(SRCDIR)/orta.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $(BINDIR)/$@ 

fcfx: $(SRCDIR)/fcfx.c $(SRCDIR)/orta.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $(BINDIR)/$@

xd: $(SRCDIR)/xd.c $(SRCDIR)/orta.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $(BINDIR)/$@

repl: $(SRCDIR)/repl.c $(SRCDIR)/orta.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $(BINDIR)/$@

xtoa: $(SRCDIR)/xtoa.c $(SRCDIR)/orta.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $(BINDIR)/$@
clean:
	rm -rf $(BINDIR)
	rm -f *.pre.x *.xbin xtoa

release:
	xxd -i std.x > $(SRCDIR)/std.h

debug: CFLAGS += -DDEBUG
debug: all
