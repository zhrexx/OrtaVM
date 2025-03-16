CC = gcc
CFLAGS = -g -ggdb

.PHONY: all
all: orta fcfx xtoa

orta: src/orta.c src/orta.h
	$(CC) $(CFLAGS) src/orta.c -o orta

fcfx: src/fcfx.c src/orta.h
	$(CC) $(CFLAGS) src/fcfx.c -o fcfx

xtoa: src/xtoa.c 
	$(CC) $(CFLAGS) src/xtoa.c -o xtoa

libx:
	nasm -felf64 src/libx.asm -o libx.o

clean:
	rm *.pre.x *.xbin libx.o xtoa fcfx orta
