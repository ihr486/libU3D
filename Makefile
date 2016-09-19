#Makefile for libU3D demo

CXXSRCS := $(wildcard *.cpp)
HDRS := $(wildcard *.hpp)
CXXOBJS := $(CXXSRCS:%.cpp=%.o)
OBJS := $(CXXOBJS)

CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++03 -O2 $(shell sdl2-config --cflags) $(shell pkg-config --cflags glew)
LDFLAGS := -lm $(shell sdl2-config --libs) -lSDL2_image $(shell pkg-config --libs glew)

BIN := u3d

.PHONY: all check clean install

all: $(BIN)

check:
	$(CXX) $(CXXFLAGS) -fsyntax-only $(CXXSRCS)

clean:
	-@rm -vf $(OBJS) $(BIN)

install: $(BIN)
	install --mode=755 --target-directory=/usr/local/bin $<

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
