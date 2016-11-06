CXXSRCS := viewer.cc

BIN := ../viewer

.PHONY: all clean install

all: $(BIN)

clean:
	-@rm -vf $(BIN)

install: $(BIN)
	install --mode=755 --target-directory=/usr/local/bin $<

$(BIN): $(CXXSRCS) ../libu3d.a
	$(CXX) -I../src $(CXXFLAGS) -o $@ $^ -L.. -lu3d $(LDFLAGS)
