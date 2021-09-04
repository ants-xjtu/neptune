#include "Common.h"

// Associate a pkey with the dependency tree of a moon
// TODO1: eliminate the segfault next to PkeyUpdate:
//      Strange enough, when ProtectMoon is enabled, the instruction
//      next to PkeyUpdate will segfault. The same issue is reported 
//      when chaining 2 nDPIs.(By that time ProtectMoon is not even implemented)
//      This can be reproduced by chaining any 2 of the NFs with 
//      ProtectMoon on
// TODO2: make this step more efficient(and correct)
//      This is more like a PoC with badly handled corner cases.
//      Review this file again and improve its robustness and performance

static inline void RunProtect(const char *perm, uint64_t addrStart, uint64_t addrEnd, int pkey)
{
    // fprintf(stderr, "[RunProtect] addrStart: %lu\n", addrStart);
    if (unlikely(perm[3] != 'p'))
    {
        printf("[ProtectLib] Attempting to pkey_mprotect a bad(shared) page. Quit\n");
        abort();
    }
    int prot = 0;
    prot |= (perm[0] == 'r')? PROT_READ : 0;
    prot |= (perm[1] == 'w')? PROT_WRITE : 0;
    prot |= (perm[2] == 'x')? PROT_EXEC : 0;
    // this meets the alignment requirement by design
    size_t len = addrEnd - addrStart;

    if (unlikely(pkey_mprotect((void *)addrStart, len, prot, pkey) < 0))
    {
        printf("[ProtectLib] pkey_mprotect failed with code %d\n", errno);
        abort();
    }
}

// REALLY use pkey_mprotect to cover all the shared libraries an NF uses
static void ProtectLib(const char *libname, FILE *map, int pkey)
{
    char lineBuf[256];
    uint64_t addrStart, addrEnd, prevEnd = 0;
    char perm[5];
    uint32_t offset;
    uint16_t devHigh, devLow;
    unsigned int inode;
    char pathname[128];
    int nConsumed;
    int hit = 0, prevHit = 0;

    // ***get filename from absolute name
    const char *uppername = libname + strlen(libname);
    // though this is impossible, we still need to prevent underflow
    while (*uppername != '/' && uppername >= libname)
        uppername--;
    uppername--;
    // get the directory of the library
    while (*uppername != '/' && uppername >= libname)
        uppername--;
    uppername++;
    // ***end

    while (fgets(lineBuf, sizeof(lineBuf), map))
    {
        sscanf(lineBuf, "%lx-%lx %4c %x %hx:%hx %u%n", &addrStart, &addrEnd, perm, &offset, &devHigh, &devLow, &inode, &nConsumed);
        if (prevHit == 1 && hit == 0)
            return;
        prevHit = hit;
        if (devHigh || devLow || inode)
        {
            sscanf(lineBuf+nConsumed, "%s", pathname);
            // ***get filename from absolute name
            char *c = pathname + strlen(pathname);
            // though this is impossible, we still need to prevent underflow
            while (*c != '/' && c >= pathname)
                c--;
            c--;
            // get the directory of the library
            while (*c != '/' && c >= pathname)
                c--;
            c++;
            // ***end
            if (strcmp(uppername, c))
            {
                // fprintf(stderr, "libname: %s, mapname: %s\n", uppername, c);
                hit = 0;
                continue;
            }
                
            // printf("0x%lx-0x%lx %s\n", addrStart, addrEnd, pathname);
            RunProtect(perm, addrStart, addrEnd, pkey);
            prevEnd = addrEnd;
            hit = 1;
        }
        else if (addrStart == prevEnd)
        {
            // this branch only hit anon pages that are adjacent to last named one
            // printf("0x%lx-0x%lx    [Anon]\n", addrStart, addrEnd);
            RunProtect(perm, addrStart, addrEnd, pkey);
            prevEnd = addrEnd;
            hit = 1;
        }
        else
            hit = 0;
    }
}

// handle pkey for each moon
void ProtectMoon(const char *moonPath, int pkey)
{
    pid_t pid = getpid();

    char buf[32];
    sprintf(buf, "/proc/%d/maps", pid);
    FILE *map = fopen(buf, "r");

    // get directory of current moon
    int len = strlen(moonPath);
    const char *c = moonPath + len;
    while (*c != '/')
    {
        c--;
        len--;
    }
    char dir[128];
    strcpy(dir, moonPath);
    // making sure dir is null-terminated
    dir[len + 1] = 0; 
    printf("[ProtectMoon] entering directory: %s\n", dir);
    DIR *Dir = opendir(dir);
    if (Dir == NULL)
    {
        printf("[ProtectMoon] cannot open moon path\n");
        abort();
    }
    // iterate through every file
    struct dirent *entry;
    while ((entry = readdir(Dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        char absolutePath[512];
        strncpy(absolutePath, dir, 128);
        strcat(absolutePath, entry->d_name);
        // printf("[ProtectMoon] Protecting file: %s\n", absolutePath);
        ProtectLib(absolutePath, map, pkey);
        fseek(map, 0, SEEK_SET);
    }
    printf("%s", DONE_STRING);
    // abort();
}
