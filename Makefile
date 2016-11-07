#Makefile for libU3D

ifeq ($(TMPDIR),)
export OBJDIR := /tmp/libU3D
else
export OBJDIR := $(TMPDIR)/libU3D
endif

EMULATOR := $(shell uname -o)
PROCESSOR := $(shell uname -m)
ifeq ($(EMULATOR),Cygwin)
ifeq ($(PROCESSOR),i686)
export CXX := i686-w64-mingw32-g++
PKGCONFIG := i686-w64-mingw32-pkg-config
else
export CXX := x86_64-w64-mingw32-g++
PKGCONFIG := x86_64-w64-mingw32-pkg-config
endif
export CXXFLAGS := -MMD -MP -Wall -Wextra -std=c++03 -O2 $(shell $(PKGCONFIG) --cflags sdl2) -I/usr/x86_64-w64-mingw32/sys-root/mingw/include/GL
export LDFLAGS := -Wl,--subsystem,windows -lm $(shell $(PKGCONFIG) --libs sdl2 SDL2_image) -lglew32 -lopengl32
else
export CXX := g++
PKGCONFIG := pkg-config
export CXXFLAGS := -MMD -MP -Wall -Wextra -std=c++03 -O2 $(shell pkg-config --cflags sdl2 glew)
export LDFLAGS := -lm $(shell pkg-config --libs sdl2 SDL2_image glew)
endif

.PHONY: all clean install all-demo all-src

all: all-demo

all-demo: all-src
	$(MAKE) -C demo all

all-src: $(OBJDIR)
	$(MAKE) -C src all

clean:
	$(MAKE) -C demo clean
	$(MAKE) -C src clean
	-@rm -rf $(OBJDIR)

install: 
	$(MAKE) -C src install

$(OBJDIR):
	-@mkdir -p $@
