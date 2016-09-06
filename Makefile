#Makefile for libU3D demo

CXXSRCS := $(wildcard *.cpp)
CXXOBJS := $(CXXSRCS:%.cpp=%.o)
OBJS := $(CXXOBJS)

CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++11 -Os $(shell pkg-config --cflags libjpeg libpng)
LDFLAGS := -lm $(shell pkg-config --libs libjpeg libpng)

BIN := u3d

.PHONY: all check clean install

all: $(BIN)

check:
	$(CXX) $(CXXFLAGS) -fsyntax-only $(CXXSRCS)

clean:
	-@rm -vf $(OBJS) $(BIN)

install: $(BIN)
	install --mode=755 --target-directory=/usr/local/bin $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
