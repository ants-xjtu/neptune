CLANG    := clang-10
OPT      := opt-10
LLINK    := llvm-link-10
LLVMCONF := llvm-config-10

CC      := $(CLANG)
CFLAGS := -c -emit-llvm -fPIC -Wall -Wextra -O3 -I .

CXX     := g++
CXXFLAGS  := -Wall -fPIC -g `$(LLVMCONF) --cxxflags` -O2 -I. -ggdb

LDFLAGS := -g -shared
OBJDIR  ?= ./obj
LIB      := libGmem-pass.so

OBJS := Gmem Auxiliary
OBJS := $(patsubst %,$(OBJDIR)/%.o,$(OBJS))

.PHONY: all clean

all: $(OBJDIR)/$(LIB)

$(OBJDIR)/$(LIB): $(OBJS)
	$(CXX) -o $@ $(OBJDIR)/Gmem.o $(LDFLAGS)

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -o $@ $<
	
$(OBJDIR):
	mkdir -p $@

clean:
	rm -rf $(OBJDIR)