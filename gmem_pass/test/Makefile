CLANG    := clang-10
OPT      := opt-10
LLINK    := llvm-link-10
LLVMCONF := llvm-config-10

RW         ?= w
CHECK_METHOD ?= ifelseheap
CHECK_EXT_CALL_ARGS ?= true

CFLAGS  := -Wall -g -O3 -I../
OPTFLAGS := -Xclang -load -Xclang ../obj/libGmem-pass.so

LDFLAGS := -lpcre
OBJDIR  ?= ./obj

PROGS := test test_io testpcre testpcrecore

.PHONY: all clean

all: $(PROGS)

%: %.c $(OBJDIR)
	$(CLANG) $(OPTFLAGS) $(CFLAGS) -o $(OBJDIR)/$@ $< ../Auxiliary.c $(LDFLAGS)

$(OBJDIR):
	mkdir -p $@

clean:
	rm -rf $(OBJDIR)