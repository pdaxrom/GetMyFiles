VERSION		:= $(shell ../getversion.sh ../)
SYSTEM		:= $(shell uname)

ifneq (mingw32, $(SYSTEM))
ifeq (y, $(STATIC))
LIBS		+= -g -lssl -lcrypto -lpthread -lz -ldl -static
else
LIBS		= -g -Wl,-Bstatic -lssl -lcrypto -Wl,-Bdynamic -lpthread -ldl
endif
ifeq (Darwin, $(SYSTEM))
CC		 = i686-apple-darwin10-gcc
CXX		 = i686-apple-darwin10-g++
LIBS		+= -lcrypto
else
LIBS		+= -lXpm
endif
else
EXEEXT		= .exe
CC		= i686-w64-mingw32-gcc
CXX		= i686-w64-mingw32-g++
LIBS		= -L../extern/inst/lib -static -g -lssl -lcrypto -lwsock32 -liphlpapi -lgdi32
WINDRES		= i686-w64-mingw32-windres
STRIP		= i686-w64-mingw32-strip
endif

CFLAGS = -g -Wall -I. -I.. -I../extern/inst/include -I../lib -DCLIENT_GUI -DVERSION=$(VERSION)
ifeq (y,$(DEBUG))
CFLAGS += -DDEBUG
endif

FLTKCONFIG	= fltk-config
#CXX		= $(shell $(FLTKCONFIG) --cxx)
CXXFLAGS	= $(CFLAGS) $(shell $(FLTKCONFIG) --cxxflags)
LINKFLTK	= $(LIBS) $(shell $(FLTKCONFIG) --ldstaticflags)

TARGET = getmyfiles$(EXEEXT)

all: $(TARGET)

OBJS_G = main.o gui.o
OBJS_C = ../client.o ../urldecode.o ../http.o ../utils.o ../httpd.o ../getaddr.o ../connctrl.o
OBJS_L = ../lib/tcp.o ../lib/udp.o ../lib/aes.o

ifeq (mingw32, $(SYSTEM))
OBJS_R = resource.res
endif

$(TARGET): $(OBJS_G) $(OBJS_C) $(OBJS_L) $(OBJS_R)
	$(CXX) -o $@ $^ $(LINKFLTK)

# clean everything
clean:
	rm -f $(OBJS_G) $(OBJS_C) $(OBJS_L) $(OBJS_R) $(TARGET) getmyfiles.app/Contents/MacOS/$(TARGET) getmyfiles.app/Contents/Info.plist getmyfiles.app/Contents/Info.pliste

distclean: clean

package: all
ifeq (Darwin, $(SYSTEM))
	install -m 755 $(TARGET) getmyfiles.app/Contents/MacOS/$(TARGET)
	cp getmyfiles.app/Contents/Info.plist.in getmyfiles.app/Contents/Info.plist
	sed -ie "s|@VERSION@|$(VERSION)|" getmyfiles.app/Contents/Info.plist
	rm -rf /tmp/getmyfiles.app
	cp -R getmyfiles.app /tmp/
	rm -f /tmp/getmyfiles.app/Contents/Info.plist.in
	rm -f /tmp/getmyfiles.app/Contents/Info.pliste
	find /tmp/getmyfiles.app -name ".svn" | xargs rm -rf -
	rm -f /tmp/getmyfiles-$(VERSION)-osx.zip
	cd /tmp && zip -r9 getmyfiles-$(VERSION)-osx.zip getmyfiles.app
	mv /tmp/getmyfiles-$(VERSION)-osx.zip ../../
endif
ifeq (mingw32, $(SYSTEM))
	$(STRIP) $(TARGET)
	rm -f ../../getmyfiles-$(VERSION)-win32.zip
	zip -9 ../../getmyfiles-$(VERSION)-win32.zip $(TARGET)
	echo "OutFile \"../../getmyfiles-$(VERSION)-win32-installer.exe\"" > getmyfiles.nsis.inc
	iconv -f UTF-8 -t CP1251 license-russian.txt > license-russian.txt.1251
	makensis getmyfiles.nsis
	rm -f getmyfiles.nsis.inc
	rm -f license-russian.txt.1251
endif
ifeq (Linux, $(SYSTEM))
	cp debian/changelog.in debian/changelog
	sed -i -e "s|@VERSION@|$(VERSION)|" debian/changelog
	sed -i -e "s|@DATE@|$(shell date -R)|" debian/changelog
	dpkg-buildpackage -rfakeroot -b -tc || true
	rm -f debian/changelog
	mv -f ../*.changes ../../
	mv -f ../*.deb     ../../
endif

.SUFFIXES: .cxx .h .fl .o .rc .res $(EXEEXT)

.cxx.o:
	$(CXX) -I.. $(CXXFLAGS) -c $< -o $@

.rc.res:
	$(WINDRES) -i $< -O coff -o $@

install: all
	install -D -m 755 $(TARGET) $(DESTDIR)/usr/bin/$(TARGET)
	install -D -m 644 icon/icon.png $(DESTDIR)/usr/share/pixmaps/getmyfiles.png
	install -D -m 644 getmyfiles.desktop $(DESTDIR)/usr/share/applications/getmyfiles.desktop
