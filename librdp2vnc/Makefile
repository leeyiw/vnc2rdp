TARGET = librdp2vnc.so
CC = gcc
CFLAGS = -c -Wall -fPIC
DEBUG_FLAGS = -g
RELEASE_FLAGS = -O2
OBJS = rdp_server.o rdp_conn.o sec.o mcs.o x224.o tpkt.o packet.o
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
rdp_server.o: rdp_server.c rdp_server.h
	$(CC) $(CFLAGS) $(INCLUDE) rdp_server.c
rdp_conn.o: rdp_conn.c rdp_conn.h
	$(CC) $(CFLAGS) $(INCLUDE) rdp_conn.c
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

clean:
	-rm -rf $(TARGET) $(OBJS)
