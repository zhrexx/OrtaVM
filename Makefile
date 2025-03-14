CC = gcc
CFLAGS = -O2 -static

all:
	$(CC) $(CFLAGS) orta.c -o orta


