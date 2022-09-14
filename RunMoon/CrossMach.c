#include "Common.h"
#define _FILE_OFFSET_BITS 64

#define TOTAL_SLOTS 128
#define ACTIVE_NFS 16

// the range below is generally not used
// and support 2^44 = 16TB
// NB: only perfect MOON can use this
static void *StartAddr = (void *)0x600000000000;
static void *EndAddr   = (void *)0x700000000000;
// page does not have to be 1000, but we buffer it every 1000
static char pageBuf[4096];

struct MapEntry
{
    uint64_t startAddr;
    uint64_t length;
    char perm[5];
};

// singleton to save all maps of the MOON to dump
struct MapEntry map_entry_buffer[128];

// path.split('/')[-2]
static const char *FindDir(const char *path)
{
    const char *curr = path;
    const char *slash1 = path, *slash2;
    while (*curr != '\0')
    {
        if (*curr == '/')
        {
            slash2 = slash1;
            slash1 = curr;
        }
        curr++;
    }
    return slash2;
}

// note that directory is '/' splited, and thus cripple strcmp
static int MatchDir(const char *dst, const char *src)
{
    const char *dstDir = FindDir(dst);
    const char *srcDir = FindDir(src);
    do
    {
        if (*dstDir != *srcDir)
            return 0;
        dstDir++;
        srcDir++;
    } while (*dstDir != '/' && *srcDir != '/');
    if (*dstDir == '/' && *srcDir == '/')
        return 1;
    else
        return 0;
}

// get directory from existing moonId, or config
static char *GetDumpDir(int moonId, int configID)
{
    int configId = (moonId == -1) ? configID : moonDataList[moonId].config;
    const char *moonName = CONFIG[configId].path;
    char filename[256] = "";
    char *namePtr;
    const char *dirPtr = FindDir(moonName);

    strcat(filename, "./dump");
    namePtr = filename + strlen(filename);
    do
    {
        *namePtr = *dirPtr;
        namePtr++;
        dirPtr++;
    } while (*dirPtr != '/');
    *namePtr = '/'; namePtr++;
    *namePtr = '\0';

    return strdup(filename);
}

static void ReadMap(FILE *map, FILE *mem, int moonId, const char *moonPath, void *end)
{
    char lineBuf[256];
    uint64_t addrStart, addrEnd = 0, prevEnd = 0;
    char perm[5];
    uint32_t offset;
    uint16_t devHigh, devLow;
    unsigned int inode;
    char pathname[128];
    int nConsumed;
    uint64_t mapLength;

    int dump = 0;
    char outName[256] = "";
    char *outDir = GetDumpDir(moonId, 0);
    const char *tmp = FindDir(moonPath);

    printf("[ReadMap] initialize output directory: %s\n", outDir);
    strcat(outName, outDir);
    char *namePtr = outName + strlen(outDir);
    free(outDir);

    while (fgets(lineBuf, sizeof(lineBuf), map))
    {
        sscanf(lineBuf, "%lx-%lx %4c %x %hx:%hx %u%n", &addrStart, &addrEnd, perm, &offset, &devHigh, &devLow, &inode, &nConsumed);
        mapLength = addrEnd - addrStart;
        if (addrEnd > (uint64_t) end)
            break;
        if (devHigh || devLow || inode)
        {
            sscanf(lineBuf+nConsumed, "%s", pathname);
            if (MatchDir(FindDir(pathname), FindDir(moonPath)))
                dump = 1;
            // printf("0x%lx-0x%lx %s\n", addrStart, addrEnd, pathname);
        }
        if(dump)
        {
            // we are the first map, otherwise each map has to be consistent
            if (prevEnd && addrStart != prevEnd)
            {
                fprintf(stderr, "[ReadMap] previous map ends at: %lx, but a new map is trying to start at: %lx\n", prevEnd, addrStart);
                return;
            }
            // startaddr_length_prot.dump
            char toCat[64] = "";
            // strcat use \0 to find string end, so it's very important to set it
            *namePtr = '\0';
            sprintf(toCat, "%lx_", addrStart);
            strcat(outName, toCat);
            sprintf(toCat, "%lx_", mapLength);
            strcat(outName, toCat);
            strcat(outName, perm);
            strcat(outName, ".dump");
            printf("[ReadMap] outputing dump file: %s\n", outName);

            // now dumping...
            FILE *dumpFile = fopen(outName, "wb");
            uint64_t totalBytes = 0;
            while (mapLength)
            {
                fseek(mem, addrStart + totalBytes, SEEK_SET);
                size_t byteRead = fread(pageBuf, 1, 4096, mem);
                assert(byteRead % 4096 == 0);
                totalBytes += byteRead;
                fwrite(pageBuf, 1, 4096, dumpFile);
                mapLength -= 4096;
            }
            fclose(dumpFile);
            prevEnd = addrEnd;
        }
    }
}

// dump a moon, but do not care about SFC
static void DumpMoonWorker(int moonId)
{
    int configId = moonDataList[moonId].config;
    const char *moonName = CONFIG[configId].path;
    int workerId = rte_lcore_id();
    
    FILE *map = fopen("/proc/self/maps", "r");
    if (map == NULL)
    {
        fprintf(stderr, "[DumpMoonWorker] cannot open map file\n");
        return;
    }
    FILE *mem = fopen("/proc/self/mem", "rb");
    if (mem == NULL)
    {
        fprintf(stderr, "[DumpMoonWorker] cannot open mem file\n");
        return;
    }

    // dump everything from the first mapping to MOON's arena
    void *arenaEnd = moonDataList[moonId].workers[workerId].arenaEnd;
    ReadMap(map, mem, moonId, moonName, arenaEnd);
}

static void DumpRegWorker(int moonId, unsigned instanceId)
{
    char *dumpDir = GetDumpDir(moonId, 0);
    char regFile[256] = "";

    strcat(regFile, dumpDir);
    strcat(regFile, "RegFile");
    printf("[DumpRegWorker] Writting regfile to %s\n", regFile);
    // though doable, using pointer from previous frame is dangerous
    DumpStack(regFile, instanceId);
}


static void MapRegWorker(int configId, unsigned instanceId)
{
    // TODO: check why calling malloc in new process lead to segfault
    char *dumpDir = GetDumpDir(-1, configId);
    char regFile[256] = "";

    strcat(regFile, dumpDir);
    strcat(regFile, "RegFile");

    LoadStack(regFile, instanceId);
}

static void MapMoonWorker(int configId)
{
    DIR *md;
    char md2[48];
    struct dirent *f;
    uint64_t addrStart;
    uint64_t mapLength;
    char perm[5];
    int prot;
    int fd;
    char filename[256];
    int numMapped = 0;
    int numFiles = 0;

    // moonDir should be a slash-terminated directory
    char *moonDir = GetDumpDir(-1, configId);

    printf("[MapMoonWorker] retrieving pages from MOON\n");
    if (NULL == (md = opendir(moonDir)))
    {
        fprintf(stderr, "[MapMoonWorker] cannot open dump file directory: %s\n", moonDir);
        return;
    }
    
    while ((f = readdir(md)) != NULL)
    {
        numFiles++;
        printf("[MapMoonWorker] opening file %s\n", f->d_name);
        if (!strcmp(f->d_name, "."))
            continue;
        if (!strcmp(f->d_name, ".."))
            continue;
        if (!strcmp(f->d_name, "RegFile"))
            continue;

        filename[0] = '\0';
        strcat(filename, moonDir);
        strcat(filename, f->d_name);
        // since we will always deal with MAP_PRIVATE, open only needs read perm
        fd = open(filename, O_RDONLY);
        if (fd == -1)
        {
            fprintf(stderr, "[MapMoonWorker] cannot open file: %s\n", filename);
            return;
        }
        sscanf(f->d_name, "%lx_%lx_%4c", &addrStart, &mapLength, perm);
        
        prot = 0;
        prot |= (perm[0] == 'r')? PROT_READ: 0;
        prot |= (perm[1] == 'w')? PROT_WRITE: 0;
        prot |= (perm[2] == 'x')? PROT_EXEC: 0;
        // printf("[MapMoonWorker] addrStart=%lx, mapLength=%lx, prot=%s\n", addrStart, mapLength, perm);
        if (MAP_FAILED == mmap((void *)addrStart, mapLength, prot, 
                MAP_FILE | MAP_PRIVATE | MAP_FIXED, fd, 0))
        {
            fprintf(stderr, "[MapMoonWorker] mmaped failed with addrStart=%lx, mapLength=%lx, prot=%s\n", 
                addrStart, mapLength, perm);
            close(fd);
            return;
        }
        close(fd);
        numMapped += 1;
    }
    printf("[MapMoonWorker] finish mapping of %d pages out of %d files\n", numMapped, numFiles);
    if (errno)
        printf("WARNING: errno at: %d\n", errno);
}

void DumpMoon(int moonId, unsigned instanceId)
{
    // TODO: adjust SFC topo inside moonDataList
    DumpMoonWorker(moonId);
    DumpRegWorker(moonId, instanceId);
}

void MapMoon(int configId, unsigned instanceId)
{
    // hard code for now:
    /*
        HM1: 3 -> 7; detach 7
        HM2: 3 -> 7; map 7(overwrite)
    */ 
    int workerId = rte_lcore_id();
    MapMoonWorker(configId);

    MapRegWorker(configId, instanceId);
    
    moonDataList[0].switchTo = 1;
    moonDataList[1].switchTo = -1;
    moonDataList[1].workers[workerId].instanceId = instanceId;
}

// TODO: reduce code redundency
// cancel the write permission of a MOON
// also, record the address span of the MOON to dump
void BlockMoon(int moonId)
{
    int configId = moonDataList[moonId].config;
    const char *moonName = CONFIG[configId].path;
    int workerId = rte_lcore_id();
    
    FILE *map = fopen("/proc/self/maps", "r");
    if (map == NULL)
    {
        fprintf(stderr, "[BlockMoon] cannot open map file\n");
        return;
    }

    char lineBuf[256];
    uint64_t addrStart, addrEnd = 0, prevEnd = 0;
    char perm[5];
    uint32_t offset;
    uint16_t devHigh, devLow;
    unsigned int inode;
    char pathname[128];
    int nConsumed;
    uint64_t mapLength;
    void *end = moonDataList[moonId].workers[workerId].arenaEnd;
    // use start address of arena to mark the stack
    // yet the mapping always starts 0x1000 eariler
    // so that we hard code it for now.
    // TODO: fix the hard code
    void *start = (void *)((uint64_t)moonDataList[moonId].workers[workerId].arenaStart - 0x1000);
    int dump = 0;
    int recordEntryCtr = 0;

    while (fgets(lineBuf, sizeof(lineBuf), map))
    {
        sscanf(lineBuf, "%lx-%lx %4c %x %hx:%hx %u%n", &addrStart, &addrEnd, perm, &offset, &devHigh, &devLow, &inode, &nConsumed);
        mapLength = addrEnd - addrStart;
        if (addrEnd > (uint64_t) end)
            break;
        if (devHigh || devLow || inode)
        {
            sscanf(lineBuf+nConsumed, "%s", pathname);
            if (MatchDir(FindDir(pathname), FindDir(moonName)))
                dump = 1;
            // printf("0x%lx-0x%lx %s\n", addrStart, addrEnd, pathname);
        }
        if(dump)
        {
            // we are the first map, otherwise each map has to be consistent
            if (prevEnd && addrStart != prevEnd)
            {
                fprintf(stderr, "[BlockMoon] previous map ends at: %lx, but a new map is trying to start at: %lx\n", prevEnd, addrStart);
                return;
            }
            if (perm[1] == 'w')
            {
                // writeable implies readable
                int pr = 0;
                pr |= (perm[2] == 'x')? PROT_EXEC: 0;
                // we find the main stack. Never play with its permission 
                if (addrStart == (uint64_t)start)
                {
                    printf("[BlockMoon] Find main stack of moon and escape it\n");
                    goto post_protect;
                }
                if (unlikely(mprotect((void *)addrStart, mapLength, pr | PROT_READ) == -1))
                {
                    fprintf(stderr, "[BlockMoon] mprotect failed with: addr=%p, len=%lu, prot=%d", (void *)addrStart, mapLength, pr);
                    return;
                }
            }
        post_protect:
            prevEnd = addrEnd;
            map_entry_buffer[recordEntryCtr].startAddr = addrStart;
            map_entry_buffer[recordEntryCtr].length = mapLength;
            // the destination lies in .bss, thus is zero terminated on default
            strncpy(map_entry_buffer[recordEntryCtr].perm, perm, 4);
            recordEntryCtr++;
            if (unlikely(recordEntryCtr >= 128))
            {
                fprintf(stderr, "[BlockMoon] The number of pre-defined map entries reachs limit, return in advance\n");
                return;
            }
        }
    }
    fclose(map);
    printf("[BlockMoon] Successfully set a MOON with non-writable\n");
}

// dump all the memory mappings recorded in map_entry_buffer
// in a zero-copy way, i.e. write from an immediate address instead of read+write
// together with new `BlockMoon`, reading maps file and dumping them are decoupled
void PrecopyMoon(const char *baseDir)
{
    int recordEntryCtr = 0;
    while (map_entry_buffer[recordEntryCtr].length != 0 && recordEntryCtr < 128)
    {
        char filename[128] = "";
        strcpy(filename, baseDir);
        sprintf(filename + strlen(filename), "%" PRIx64 "_%" PRIx64 "_%s.dump", 
            map_entry_buffer[recordEntryCtr].startAddr,
            map_entry_buffer[recordEntryCtr].length,
            map_entry_buffer[recordEntryCtr].perm);
        printf("[main] creating dump file at: %s\n", filename);
        int dumpFd = open(filename, O_RDWR | O_CREAT, 0666);
        if (unlikely(dumpFd == -1))
        {
            fprintf(stderr, "[PrecopyMoon] open memory dump %s failed\n", filename);
            abort();
        }
        // if the mapping is not readable, we just create an empty file for it
        if (map_entry_buffer[recordEntryCtr].perm[0] == '-')
        {
            if (unlikely(fallocate(dumpFd, FALLOC_FL_ZERO_RANGE, 0, map_entry_buffer[recordEntryCtr].length) == -1))
            {
                perror("[PrecopyMoon] cannot fallocate new file");
                abort();
            }
        }
        else
        {
            ssize_t bytesWritten = 0;
            do
            {
                bytesWritten += write(dumpFd,
                    (void *)(map_entry_buffer[recordEntryCtr].startAddr + bytesWritten), 
                    map_entry_buffer[recordEntryCtr].length - bytesWritten);
                if (unlikely(bytesWritten == -1))
                {
                    perror("[PrecopyMoon] cannot write to file");
                    abort();
                }
            } while (bytesWritten != map_entry_buffer[recordEntryCtr].length);
        }

        close(dumpFd);
        recordEntryCtr++;
    }
    printf("[PrecopyMoon] Done.\n");
}
