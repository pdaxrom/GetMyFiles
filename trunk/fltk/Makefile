SYSTEM		:= $(shell uname)

ifneq (mingw32, $(SYSTEM))
LIBS		= -g -lssl -lpthread
ifeq (y, $(STATIC))
LIBS		+= -lcrypto -lz -ldl -static
endif
else
EXEEXT		= .exe
CC		= i386-mingw32-gcc
CXX		= i386-mingw32-g++
LIBS		= -g -lssl -lcrypto -lz -lwsock32 -lgdi32
endif

ifeq (Darwin, $(SYSTEM))
LIBS		+= -lcrypto
endif

CFLAGS = -g -Wall -I. -I.. -I../lib -DCLIENT_GUI
ifeq (y,$(DEBUG))
CFLAGS += -DDEBUG
endif

FLTKCONFIG	= fltk-config
CXX		= $(shell $(FLTKCONFIG) --cxx)
CXXFLAGS	= $(CFLAGS) $(shell $(FLTKCONFIG) --cxxflags)
LINKFLTK	= $(LIBS) $(shell $(FLTKCONFIG) --ldstaticflags)

TARGET = itsmyfiles$(EXEEXT)

all: $(TARGET)

OBJS_G = main.o gui.o
OBJS_C = ../client.o ../urldecode.o
OBJS_L = ../lib/tcp.o ../lib/udp.o ../lib/aes.o

$(TARGET): $(OBJS_G) $(OBJS_C) $(OBJS_L)
	$(CXX) -o $@ $^ $(LINKFLTK)

# clean everything
clean:
	rm -f $(OBJS_G) $(OBJS_C) $(OBJS_L) $(TARGET) itsmyfiles.app/Contents/MacOS/$(TARGET)

package: all
ifeq (Darwin, $(SYSTEM))
	install -m 755 $(TARGET) itsmyfiles.app/Contents/MacOS/$(TARGET)
endif

.SUFFIXES: .cxx .h .fl .o $(EXEEXT)

.cxx.o:
	$(CXX) -I.. $(CXXFLAGS) -c $< -o $@
