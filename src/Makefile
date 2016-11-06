CXXSRCS := $(wildcard *.cc)
OBJS := $(CXXSRCS:%.cc=$(OBJDIR)/%.o)

LIB := ../libu3d.a

.PHONY: all clean install

all: $(LIB)

clean:
	-@rm -vf $(LIB)

install: $(LIB)
	install --mode=755 --target-directory=/usr/local/lib $<

$(LIB): $(OBJS)
	ar rvs $@ $^

$(OBJDIR)/%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

-include $(wildcard $(OBJDIR)/*.d)
