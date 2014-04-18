LIBRDP2VNC_DIR = librdp2vnc

TARGET = rdp2vnc
CC = gcc
CFLAGS = -c -Wall
DEBUG_FLAGS = -g
RELEASE_FLAGS = -O2
OBJS = rdp2vnc.o log.o
INCLUDE = -I$(LIBRDP2VNC_DIR)
LDFLAGS = -L$(LIBRDP2VNC_DIR) -lrdp2vnc

.PHONY: debug release all librdp2vnc clean

debug: MAKE += debug
debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

release: MAKE += release
release: CFLAGS += $(RELEASE_FLAGS)
release: all

all: $(TARGET)
$(TARGET): $(OBJS) librdp2vnc
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)
rdp2vnc.o: rdp2vnc.c
	$(CC) $(CFLAGS) $(INCLUDE) rdp2vnc.c
log.o: log.c log.h
	$(CC) $(CFLAGS) $(INCLUDE) log.c

librdp2vnc:
	cd $(LIBRDP2VNC_DIR) && $(MAKE)

clean:
	-rm -rf $(TARGET) $(OBJS)
	cd $(LIBRDP2VNC_DIR) && make clean
