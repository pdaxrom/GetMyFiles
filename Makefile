SYSTEM := $(shell uname)

ifneq (mingw32, $(SYSTEM))
TARGET_S = itsmyfiles-server

TARGET_C = itsmyfiles-client

LIBS = -g -lssl -lpthread

ifeq (y, $(STATIC))
LIBS += -lcrypto -lz -ldl -static
endif
else
TARGET_S =

TARGET_C = itsmyfiles-client.exe

CC = i386-mingw32-gcc

LIBS = -g -lssl -lcrypto -lz -lwsock32 -lgdi32
endif

ifeq (Darwin, $(SYSTEM))
LIBS += -lcrypto
endif

all: $(TARGET_S) $(TARGET_C)

PREFIX = /opt/simple-nx

CFLAGS = -g -Wall -Ilib -DCONFIG_DIR=\".\"

ifeq (y,$(DEBUG))
CFLAGS += -DDEBUG
endif

OBJS_L = lib/tcp.o lib/udp.o lib/aes.o

OBJS_S = server.o

OBJS_C = client.o urldecode.o

$(TARGET_S): $(OBJS_L) $(OBJS_S)
	$(CC) -o $@ $^ $(LIBS)

$(TARGET_C): $(OBJS_L) $(OBJS_C)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET_S) $(OBJS_S) $(TARGET_C) $(OBJS_C) $(OBJS_L)
