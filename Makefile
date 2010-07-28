
CFLAGS += -O3 -ffast-math -Wall -g -std=c99 -pipe -I.

CFLAGS += `pkg-config --cflags sndfile`
LIBS += `pkg-config --libs sndfile`

OBJECTS = main.o

.SUFFIXES: .c .o

all: channelmap

channelmap: $(OBJECTS)
	$(CC) $(LIBS) $(LDFLAGS) $(OBJECTS) -o channelmap

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS)
	rm -f channelmap

