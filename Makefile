CFLAGS=-O2 -Wall -Wextra
LDFLAGS=

all: zcat

zcat: inflate.o zcat.o crc32.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	-rm -f *.o zcat

