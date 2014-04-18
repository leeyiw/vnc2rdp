TARGET = librdp2vnc.so
CC = gcc
CFLAGS = -c -Wall -fPIC
DEBUG_FLAGS = -g
RELEASE_FLAGS = -O2
OBJS = rdp_server.o rdp_conn.o
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

clean:
	-rm -rf $(TARGET) $(OBJS)
	cd $(LIBRDP2VNC_DIR) && make clean
