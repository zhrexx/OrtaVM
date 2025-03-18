CC = gcc
CFLAGS = -g -ggdb

.PHONY: all
all: orta fcfx 

orta: src/orta.c src/orta.h
	$(CC) $(CFLAGS) src/orta.c -o orta

fcfx: src/fcfx.c src/orta.h
	$(CC) $(CFLAGS) src/fcfx.c -o fcfx

clean:
	rm *.pre.x *.xbin xtoa fcfx orta


release:
	xxd -i std.x src/std.h
