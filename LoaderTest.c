#define _ISOC11_SOURCE
#include <stdlib.h>
#include <stdio.h>

#include "PrivateLoad.h"

int main(int argc, char *argv[], char *envp[])
{
    InitLoader(argc, argv, envp);
    const char *libraryFileName = "/lib/x86_64-linux-gnu/libm.so.6";

    printf("private loading %s\n", libraryFileName);
    struct PrivateLibrary library;
    library.file = libraryFileName;
    LoadLibrary(&library);
    printf("library size: %lu\n", library.length);

    const size_t ALIGN = 4 << 10; // 4k page
    library.loadAddress = aligned_alloc(ALIGN, library.length);
    printf("allocate at %p\n", library.loadAddress);

    DeployLibrary(&library);
    printf("the library is deployed\n");

    double (*acos)(double) = LibraryFind(&library, "acos");
    printf("acos is at %p, acos(-1.0) = %f\n", acos, acos(-1.0));

    return 0;
}