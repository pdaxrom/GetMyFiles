VERSION		:= $(shell ./getversion.sh)
WWWROOT		= /var/lib/getmyfiles/htdocs
CONFIG_DIR	= /var/lib/getmyfiles/etc


SYSTEM := $(shell uname)

ifneq (mingw32, $(SYSTEM))
TARGET_S = getmyfiles-server

TARGET_C = getmyfiles-client

ifeq (y, $(STATIC))
LIBS = -g -lssl -lcrypto -lpthread -lz -ldl -static
else
LIBS = -g -Wl,-Bstatic -lssl -lcrypto -Wl,-Bdynamic -lpthread -ldl
endif
else
TARGET_S =

TARGET_C = getmyfiles-client.exe

CC = i686-mingw32-gcc

LIBS = -g -lssl -lcrypto -lz -lwsock32 -liphlpapi -lgdi32
endif

ifeq (Darwin, $(SYSTEM))
CC  = i686-apple-darwin10-gcc
CXX = i686-apple-darwin10-g++
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

OBJS_C = client.o urldecode.o utils.o http.o httpd.o getaddr.o connctrl.o

$(TARGET_S): $(OBJS_L) $(OBJS_S)
	$(CC) -o $@ $^ $(LIBS)

$(TARGET_C): $(OBJS_L) $(OBJS_C)
	$(CC) -o $@ $^ $(LIBS)

mime.h:
	./create-mime.assign.pl > $@

#direct-icon.h:
#	cat htdocs/pics/wifi-on.png | hexdump -v -e '/1 "0x%02X, "' > $@

clean:
	rm -f $(TARGET_S) $(OBJS_S) $(TARGET_C) $(OBJS_C) $(OBJS_L)

distclean: clean

install-server: $(TARGET_S)
	install -D -m 755 $(TARGET_S) $(DESTDIR)/usr/sbin/$(TARGET_S)
#	install -D -m 755 init/getmyfiles-server $(DESTDIR)/etc/init.d/getmyfiles-server
	install -d $(DESTDIR)$(CONFIG_DIR)
	install -D -m 644 htdocs/js/p2p.js $(DESTDIR)$(WWWROOT)/js/p2p.js
#	install -D -m 644 htdocs/js/player5.js $(DESTDIR)$(WWWROOT)/js/player5.js
#	install -D -m 644 htdocs/js/imageviewer.js $(DESTDIR)$(WWWROOT)/js/imageviewer.js
#	install -D -m 644 htdocs/js/Jplayer.swf $(DESTDIR)$(WWWROOT)/js/Jplayer.swf
#	cat htdocs/js/jquery.min.js htdocs/js/jquery.jplayer.min.js > $(DESTDIR)$(WWWROOT)/js/jquery-and-jplayer.min.js
#	install -D -m 644 htdocs/pics/play.png $(DESTDIR)$(WWWROOT)/pics/play.png
#	install -D -m 644 htdocs/pics/stop.png $(DESTDIR)$(WWWROOT)/pics/stop.png
#	install -D -m 644 htdocs/pics/wifi.png $(DESTDIR)$(WWWROOT)/pics/wifi.png

package:
	dpkg-buildpackage -rfakeroot -b -tc || true

install-site:
	tar jcvf ../getmyfiles-site.tbz --exclude=.svn -C site .
