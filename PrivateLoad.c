#define _GNU_SOURCE
#include <dlfcn.h>

#include "PrivateLoad.h"
#include "loader/NanoNF.h"

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
    const ProxyRecord records[] = {{.pointer = NULL, .name = NULL}};
    library->loadAddress = NFopen(
        library->file, RTLD_LAZY, library->loadAddress, records, loaderArgc, loaderArgv, loaderEnvp);
    return library->loadAddress == NULL;
}

void *LibraryFind(struct PrivateLibrary const *library, const char *name)
{
    return NFsym(library->loadAddress, name);
}