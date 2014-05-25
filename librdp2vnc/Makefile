TARGET = librdp2vnc.so
CC = gcc
CFLAGS = -c -Wall -fPIC
DEBUG_FLAGS = -g
RELEASE_FLAGS = -O2
OBJS = session.o vnc.o rdp.o sec.o mcs.o x224.o tpkt.o packet.o capabilities.o \
	log.o input.o
LDFLAGS = -shared

.PHONY: debug release all clean

debug: MAKE += debug
debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

release: MAKE += release
release: CFLAGS += $(RELEASE_FLAGS)
release: all

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS)
session.o: session.c session.h
	$(CC) $(CFLAGS) $(INCLUDE) session.c
vnc.o: vnc.c vnc.h
	$(CC) $(CFLAGS) $(INCLUDE) vnc.c
rdp.o: rdp.c rdp.h
	$(CC) $(CFLAGS) $(INCLUDE) rdp.c
sec.o: sec.c sec.h
	$(CC) $(CFLAGS) $(INCLUDE) sec.c
mcs.o: mcs.c mcs.h
	$(CC) $(CFLAGS) $(INCLUDE) mcs.c
x224.o: x224.c x224.h
	$(CC) $(CFLAGS) $(INCLUDE) x224.c
tpkt.o: tpkt.c tpkt.h
	$(CC) $(CFLAGS) $(INCLUDE) tpkt.c
packet.o: packet.c packet.h
	$(CC) $(CFLAGS) $(INCLUDE) packet.c
capabilities.o: capabilities.c capabilities.h
	$(CC) $(CFLAGS) $(INCLUDE) capabilities.c
log.o: log.c log.h
	$(CC) $(CFLAGS) $(INCLUDE) log.c
input.o: input.c input.h
	$(CC) $(CFLAGS) $(INCLUDE) input.c

clean:
	-rm -rf $(TARGET) $(OBJS)
