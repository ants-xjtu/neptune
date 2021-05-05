// Massive TODO (2020.12.17 by sgdxbc):
// The integration of proxy functions introduces a major refactor in plan
// Currently, ProxyRecord[] just get inserted in the end of arguments and
// pass down to relocating function all the way, which breaks the modularity
// Rethinking its role is necessary, because this is a new feature only provided
// by NoMalloc, not the original dlopen

/* include the function headers */
#include "NFlink.h"
//commented out "Loader.h" by Qcloud1223. Clearly it's not the best way to arrange the code
//for it mess up the range of user-visible and developer-visible
//#include "Loader.h"
#include <stddef.h>
#include <stdint.h>

typedef size_t Size;
typedef void *Raw;

typedef struct
{
    const char *file;
    Size size;
    Raw address;
    Raw internal;
} Library;

typedef struct
{
    const char *name;
    void *pointer;
} ProxyRecord;

void *NFopen(const char *file, int mode, void *addr, const ProxyRecord *records, int argc, char *argv[], char **env);
void *NFsym(void *l, const char *s);
uint64_t NFusage(void *l);
uint64_t NFusage_worker(const char *name, int mode);
void NFAddPreload(const char *s);
void NFAddHandle(void *handle);
