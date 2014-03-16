CFLAGS=-O2 -Wall -g `pkg-config --cflags x11 xi`
LDFLAGS=`pkg-config --libs x11 xi`

all: xlossage

clean:
	rm -f xlossage
