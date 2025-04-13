CFLAGS != pkg-config --cflags x11 xi
CFLAGS+=-O2 -Wall -g
LDLIBS != pkg-config --libs x11 xi

all: xlossage

clean:
	rm -f xlossage
