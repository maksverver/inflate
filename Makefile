CFLAGS=-O2
LDFLAGS=

all: zcat

zcat: inflate.o zcat.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	-rm -f *.o zcat

