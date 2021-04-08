#ifndef PRIVATE_LOAD_H
#define PRIVATE_LOAD_H

#include <stddef.h>

void InitLoader(int argc, char *argv[], char *envp[]);

// Globally add preload library
// the library will be treated as appearing in the front of dependence list of all loaded libraries
void PreloadLibrary(void *handle);

struct PrivateLibrary
{
    const char *file;
    void *loadAddress;
    size_t length;
};

// Logically load a library, `file` is filled, `loadAddress` and `length` is empty
// `length` will be filled when return
void LoadLibrary(struct PrivateLibrary *library);

// Actually load library and make it ready to be used
// `loadAddress` should be filled to be the start address of allocated memory for the library
int DeployLibrary(struct PrivateLibrary *library);

// Resolve symbol inside library
void *LibraryFind(struct PrivateLibrary const *library, const char *name);

#endif