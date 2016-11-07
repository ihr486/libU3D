CXXSRCS := viewer.cc
OBJS := $(CXXSRCS:%.cc=$(OBJDIR)/%.o)

BIN := ../viewer

.PHONY: all clean install

all: $(BIN)

clean:
	-@rm -vf $(BIN)

install: $(BIN)
	install --mode=755 --target-directory=/usr/local/bin $<

$(BIN): $(OBJS) ../libu3d.a
	$(CXX) $(CXXFLAGS) -o $@ $^ -L.. -lu3d $(LDFLAGS)

$(OBJDIR)/%.o: %.cc
	$(CXX) -I../src $(CXXFLAGS) -c -o $@ $<
