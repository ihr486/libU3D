#Makefile for libU3D demo

ifeq ($(TMPDIR),)
OBJDIR := /tmp/libU3D
else
OBJDIR := $(TMPDIR)/libU3D
endif

CXXSRCS := $(wildcard *.cc)
OBJS := $(CXXSRCS:%.cc=$(OBJDIR)/%.o)

EMULATOR := $(shell uname -o)
PROCESSOR := $(shell uname -m)
ifeq ($(EMULATOR),Cygwin)
ifeq ($(PROCESSOR),i686)
CXX := i686-w64-mingw32-g++
PKGCONFIG := i686-w64-mingw32-pkg-config
else
CXX := x86_64-w64-mingw32-g++
PKGCONFIG := x86_64-w64-mingw32-pkg-config
endif
CXXFLAGS := -MMD -MP -Wall -Wextra -std=c++03 -O2 $(shell $(PKGCONFIG) --cflags sdl2) -I/usr/x86_64-w64-mingw32/sys-root/mingw/include/GL
LDFLAGS := -Wl,--subsystem,windows -lm $(shell $(PKGCONFIG) --libs sdl2 SDL2_image) -lglew32 -lopengl32
else
CXX := g++
PKGCONFIG := pkg-config
CXXFLAGS := -MMD -MP -Wall -Wextra -std=c++03 -g $(shell pkg-config --cflags sdl2 glew)
LDFLAGS := -lm $(shell pkg-config --libs sdl2 SDL2_image glew)
endif

BIN := u3d

.PHONY: all check clean install

all: $(BIN)

check:
	$(CXX) $(CXXFLAGS) -fsyntax-only $(CXXSRCS)

clean:
	-@rm -rf $(OBJDIR) $(BIN)

install: $(BIN)
	install --mode=755 --target-directory=/usr/local/bin $<

$(OBJDIR)/%.o: %.cc | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR):
	-@mkdir -p $@

-include $(shell find $(OBJDIR) -name '*.d')
