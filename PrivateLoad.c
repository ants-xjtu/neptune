#define _GNU_SOURCE
#include <dlfcn.h>

#include "PrivateLoad.h"
#include "Loader/NanoNF.h"

static int loaderArgc;
static char **loaderArgv;
static char **loaderEnvp;

void InitLoader(int argc, char *argv[], char *envp[])
{
    loaderArgc = argc;
    loaderArgv = argv;
    loaderEnvp = envp;
}

void PreloadLibrary(void *handle)
{
    NFAddHandle(handle);
}

void LoadLibrary(struct PrivateLibrary *library)
{
    library->length = NFusage_worker(library->file, RTLD_LAZY);
}

int DeployLibrary(struct PrivateLibrary *library)
{
    // TODO: find every library on the dependency tree, and mprotect it
    library->loadAddress = dlopen(library->file, RTLD_DEEPBIND | RTLD_NOW);
    // everything seems to have a solution if we use dlmopen in primary experiments.
    // library->loadAddress = dlmopen(1, library->file, RTLD_NOW);
    return library->loadAddress == NULL;
}

void *LibraryFind(struct PrivateLibrary const *library, const char *name)
{
    return dlsym(library->loadAddress, name);
}