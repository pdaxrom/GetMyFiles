VERSION		:= $(shell ./getversion.sh)
WWWROOT		= /var/lib/getmyfiles/htdocs
CONFIG_DIR	= /var/lib/getmyfiles/etc


SYSTEM := $(shell uname)

ifneq (mingw32, $(SYSTEM))
TARGET_S = getmyfiles-server

TARGET_C = getmyfiles-client

LIBS = -g -lssl -lpthread

ifeq (y, $(STATIC))
LIBS += -lcrypto -lz -ldl -static
endif
else
TARGET_S =

TARGET_C = getmyfiles-client.exe

CC = i686-mingw32-gcc

LIBS = -g -lssl -lcrypto -lz -lwsock32 -liphlpapi -lgdi32
endif

ifeq (Darwin, $(SYSTEM))
LIBS += -lcrypto
endif

all: $(TARGET_S) $(TARGET_C)

PREFIX = /opt/simple-nx

CFLAGS = -g -Wall -Ilib -DWWWROOT=\"$(WWWROOT)\" -DCONFIG_DIR=\"$(CONFIG_DIR)\" -DVERSION=$(VERSION)

ifeq (y,$(DEBUG))
CFLAGS += -DDEBUG
endif

OBJS_L = lib/tcp.o lib/udp.o lib/aes.o

OBJS_S = server.o utils.o http.o urldecode.o getaddr.o

OBJS_C = client.o urldecode.o utils.o http.o httpd.o getaddr.o

$(TARGET_S): $(OBJS_L) $(OBJS_S)
	$(CC) -o $@ $^ $(LIBS)

$(TARGET_C): $(OBJS_L) $(OBJS_C)
	$(CC) -o $@ $^ $(LIBS)

mime.h:
	./create-mime.assign.pl > $@

clean:
	rm -f $(TARGET_S) $(OBJS_S) $(TARGET_C) $(OBJS_C) $(OBJS_L)

distclean: clean

install-server: $(TARGET_S)
	install -D -m 755 $(TARGET_S) $(DESTDIR)/usr/sbin/$(TARGET_S)
	install -d $(DESTDIR)$(CONFIG_DIR)
	install -D -m 644 htdocs/js/p2p.js $(DESTDIR)$(WWWROOT)/js/p2p.js

package:
	dpkg-buildpackage -rfakeroot -b -tc || true

install-site:
	tar jcvf ../getmyfiles-site.tbz --exclude=.svn -C site .
