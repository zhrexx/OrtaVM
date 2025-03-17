CC = gcc
CFLAGS = -g -ggdb

.PHONY: all
all: orta fcfx 

orta: src/orta.c src/orta.h
	$(CC) $(CFLAGS) src/orta.c -o orta

fcfx: src/fcfx.c src/orta.h
	$(CC) $(CFLAGS) src/fcfx.c -o fcfx

libx:
	nasm -felf64 src/libx.asm -o libx.o

clean:
	rm *.pre.x *.xbin libx.o xtoa fcfx orta


release:
	xxd -i std.x src/std.h
