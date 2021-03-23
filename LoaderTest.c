#include "PrivateLoad.h"
#include <dlfcn.h>

int main(int argc, const char *argv[]) {
    SetLibrary((void *)0x888888888000);
    LibraryOpen("/lib/x86_64-linux-gnu/libm.so.6", RTLD_NOW);
    return 0;
}