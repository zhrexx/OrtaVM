CC = gcc
CFLAGS = -g -ggdb

.PHONY: all
all: orta fcfx xd 

orta: src/orta.c src/orta.h
	$(CC) $(CFLAGS) src/orta.c -o orta

fcfx: src/fcfx.c src/orta.h
	$(CC) $(CFLAGS) src/fcfx.c -o fcfx

clean:
	rm *.pre.x *.xbin xtoa fcfx orta

xd: src/xd.c
	$(CC) $(CFLAGS) src/xd.c -o xd

release:
	xxd -i std.x src/std.h
