/* map the ELF into memory
 * first, find and open the file from fs
 * then, parse the file and create link map on heap
 * finally, map it into memory
 * 
 * 
 */

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "NFlink.h"

/* helper to get an aligned address */
#define ALIGN_DOWN(base, size) ((base) & -((__typeof__(base))(size)))
#define ALIGN_UP(base, size) ALIGN_DOWN((base) + (size)-1, (size))
/* according to dl-load.h, this is the correct way to map a so */
#define MAP_COPY (MAP_PRIVATE | MAP_DENYWRITE)

/* struct filebuf
{
    long len; //store the length of bytes in buf
    char buf[832] __attribute__ ((aligned (__alignof(Elf64_Ehdr))));
    //size==832 is a heuristic in dl-load.c which will meet the requirement of most ELFs
    //note that buf is exclusively for ELF header, even at the end of it may exist some fragments of phdr, we don't care
}; */

/* struct to store PT_LOAD info */
struct loadcmd
{
    Elf64_Addr mapstart, mapend, dataend, allocend;
    Elf64_Off mapoff;
    int prot; /* PROT_* bits.  */
};

static void fill_info(struct NF_link_map *l)
{
    /* this function fill the l_info member of the link map
     * Also, it is implemented under the logic of trying not to include unnecessary info, like link_map itself
     * 
     * In one line, it parse the raw info in l_ld into l_info, which is indexed
     * All the DT_* entries signify the dynamic entry type
     */
    Elf64_Dyn *dyn = l->l_ld;
    Elf64_Dyn **info = l->l_info;

    while (dyn->d_tag != DT_NULL)
    {
        if ((Elf64_Xword)dyn->d_tag < DT_NUM)
            info[dyn->d_tag] = dyn;
        else if ((Elf64_Xword)dyn->d_tag == DT_RELACOUNT)
            //info[ DT_NUM + (DT_VERNEEDNUM - dyn->d_tag)] = dyn; //this is a quick fix for relacount
            info[34] = dyn;
        else if ((Elf64_Xword)dyn->d_tag == DT_GNU_HASH)
            info[35] = dyn;
        //upd: Use fixed index instead of lots of unreadable macros
        else if ((Elf64_Xword)dyn->d_tag == DT_VERSYM)
            info[36] = dyn;
        else if ((Elf64_Xword)dyn->d_tag == DT_VERNEED)
            info[37] = dyn;
        else if ((Elf64_Xword)dyn->d_tag == DT_VERDEF)
            info[38] = dyn;
        //TODO: optimize this huge branch if the performance is poor
        ++dyn;
        /* OS specific flags are currently omitted */
    }
/* at this time, the link map is loaded. So dynamic sections using ptr should be rebased here */
#define rebase(tag)                             \
    do                                          \
    {                                           \
        if (info[tag])                          \
            info[tag]->d_un.d_ptr += l->l_addr; \
    } while (0)
    rebase(DT_SYMTAB);
    rebase(DT_STRTAB);
    rebase(DT_RELA);
    rebase(DT_JMPREL);
    rebase(35); //DT_GNU_HASH
    rebase(36); //DT_VERSYM is an index to all symbols
    rebase(37); //DT_VERNEED is a place to store all the version needed
    rebase(38); //DT_VERDEF is ... definitions
}

static void setup_hash(struct NF_link_map *l)
{
    /* it would be impossible if we don't use hash table when searching
        So I set up the hash table here 
        Currently only support gnu hash */
    uint32_t *hash;

    /* borrowed from dl-lookup.c:_dl_setup_hash */
    Elf32_Word *hash32 = (Elf32_Word *)l->l_info[35]->d_un.d_ptr;
    l->l_nbuckets = *hash32++;
    Elf32_Word symbias = *hash32++;
    Elf32_Word bitmask_nwords = *hash32++;

    l->l_gnu_bitmask_idxbits = bitmask_nwords - 1;
    l->l_gnu_shift = *hash32++;

    l->l_gnu_bitmask = (Elf64_Addr *)hash32;
    hash32 += 64 / 32 * bitmask_nwords;

    l->l_gnu_buckets = hash32;
    hash32 += l->l_nbuckets;
    l->l_gnu_chain_zero = hash32 - symbias;
}

//get rid of ehdr because filebuf will not be in NFmap now. It is restricted to NFusage.
static void map_segments(struct NF_link_map *l, void *addr, struct loadcmd *loadcmds, int nloadcmds,
                         bool has_holes, int fd, size_t maplength)
//                    bool has_holes, int fd, size_t maplength, Elf64_Ehdr *e)
{
    /* here we mmap the segments into memory
     * The original implementation is like:
     *     First, do a mmap with length==maplength, assuring a piece of contiguious memory
     *     Then, each segment are mmap'd with MAP_FIXED on, right in the memory claimed before
     */
    /* At least now, I assume [addr, addr + maplength] is always clean */
    struct loadcmd *c = loadcmds;
    /* I'm not doing this because addr is specified */
    //l->l_map_start = (Elf64_Addr) mmap(addr, maplength, c->prot, MAP_COPY|MAP_FILE, fd, c->mapoff);
    if (nloadcmds < 2)
    {
        printf("Wrong type of file %s: LOAD segment less than 2\n", l->l_name);
        exit(-1);
    }

    void *tmp;
    if (addr)
    {
        //mmap(addr, maplength, c->prot, MAP_COPY|MAP_FILE|MAP_FIXED, fd, c->mapoff);
        //pread(fd, addr, maplength, c->mapoff);
        int stat = mprotect(addr, maplength, c->prot | PROT_WRITE);
        //this violate the original protection, which is bad...
        //but we are always legal to write on a heap
        if (stat)
        {
            printf("error when calling mprotect, and the error code is %d\n", errno);
            exit(-1);
        }
        l->l_map_start = (Elf64_Addr)addr;
    }
    else
    {
        //tmp = mmap(addr, maplength, c->prot, MAP_COPY|MAP_FILE, fd, c->mapoff);
        //l->l_map_start = (Elf64_Addr)tmp; //deal with addr when it is 0
        printf("please map the so in a malloc'd region\n");
        exit(-1);
    }

    l->l_map_end = l->l_map_start + maplength;
    l->l_addr = l->l_map_start - c->mapstart;
    //mmap(addr, maplength, c->prot, MAP_COPY|MAP_FILE|MAP_FIXED, fd, c->mapoff);

    if (has_holes)
    {
        /* This is bad because it assume only the first LOAD contains executable pages
         * While in fact it is not
         */
        //mprotect((void *)(l->l_map_start + c->mapend - c->mapstart),
        //         loadcmds[nloadcmds - 1].mapstart - c->mapend,
        //         PROT_NONE);
        mprotect((void *)l->l_addr + loadcmds[nloadcmds - 2].mapend,
                 loadcmds[nloadcmds - 1].mapstart - loadcmds[nloadcmds - 2].mapend,
                 PROT_NONE);
    }

    while (c < &loadcmds[nloadcmds])
    {
        //mmap((void *) (l->l_addr + c->mapstart), c->mapend - c->mapstart, c->prot,
        //    MAP_FILE|MAP_COPY|MAP_FIXED,
        //    fd, c->mapoff);
        if (pread(fd, (void *)(l->l_addr + c->mapstart), c->mapend - c->mapstart, c->mapoff) < 0)
        {
            printf("error when reading library %s, and the errno is %d", l->l_name, errno);
            exit(-1);
        }
        mprotect((void *)(l->l_addr + c->mapstart), c->mapend - c->mapstart, c->prot | PROT_WRITE); //may have issues

        if (l->l_phdr == 0 &&
            l->l_phoff >= c->mapoff &&
            (size_t)(c->mapend - c->mapstart + c->mapoff) >= l->l_phoff + l->l_phnum * sizeof(Elf64_Phdr))
            /* find the segment that contains PHT */
            /* note that PHT may or may not be a seperate segment, and may or may not be included by a LOAD seg */
            l->l_phdr = (void *)(uintptr_t)(c->mapstart + l->l_phoff - c->mapoff);

        /* only .bss will cause this */
        if (c->allocend > c->dataend)
        {
            // Elf64_Addr zero, zeroend, zeropage;
            // zero = l->l_addr + c->dataend; //where zero should start
            // zeroend = l->l_addr + c->allocend;
            // zeropage = (zero + 4095) & (~4095);

            // if (zeroend < zeropage)
            //     zeropage = zeroend;

            // if (zeropage > zero)
            // {
            //     /* In this case we have some .bss here */
            //     memset((void *)zero, '\0', zeropage - zero);
            // }

            // if (zeroend > zeropage)
            // {
            //     /* need new page to store tons of bss variables here */
            //     //mmap((void *)zeropage, ALIGN_UP(zeroend, 4096) - zeropage,
            //     //    c->prot, MAP_ANON|MAP_PRIVATE|MAP_FIXED, -1, 0);
            //     memset((void *)zeropage, 0, ALIGN_UP(zeroend, 4096) - zeropage);
            // }
            memset((void *)(l->l_addr + c->dataend), 0, ALIGN_UP(l->l_addr + c->allocend, 4096) - (l->l_addr + c->dataend));
        }
        //if(c == &loadcmds[0])
        //{
        //restore the permission of the original load segment
        mprotect((void *)(l->l_addr + c->mapstart), c->mapend - c->mapstart, c->prot);
        //}
        ++c;
    }
}

/* load an NF_link_map specified by nl->map */
//struct NF_link_map *NF_map(const char *file, int mode, void *addr)
struct NF_link_map *NF_map(struct NF_list *nl, int mode, void *addr)
{
    //int fd;

    /* TODO: actually serach directories here */
    //fd = open(file, O_RDONLY, 0);
    /* it may help to know the SONAME and REALNAME of a so */

    //struct filebuf fb;

    /* just cram into buf as much as you can */
    /* use a do-while can assure this event won't be interrupted */
    //size_t retlen = read(fd, fb.buf, sizeof(fb.buf) - fb.len);

    /* the information we get from ehdr is merely phnum, total length of all program headers, and offset for program headers
     * so we dump filebuf in NF_list, instead it is added as NF_link_map members
     * also ``Elf64_Addr`` is always sufficient for ``maplength``, for it is a memory length
     */
    //Elf64_Ehdr *ehdr = (Elf64_Ehdr *)nl->fb.buf;
    //size_t maplength = ehdr->e_phnum * sizeof(Elf64_Phdr);

    /* create a new link map, and fill in header table info */
    //struct NF_link_map *l = calloc(sizeof(struct NF_link_map), 1);
    //l->l_phnum = ehdr->e_phnum;

    /* note that in dl-load: open_verify 
     * That function open an file with a little bit check to the phdrs
     * And the same thing just appear once again in _dl_map_object_from_fd
     */
    /* store the phdrs on heap for further loading */
    //Elf64_Phdr *phdr = malloc(maplength);
    //pread(nl->fd, (void *)phdr, maplength, ehdr->e_phoff);

    struct NF_link_map *l = nl->map;

    //debug interface is temporarily disabled
    // fprintf(stderr, "Debug: at NFmap.c:268, the address of symbol file %s should be %p\n", l->l_name,
    // (void *)(l->l_text_start + (Elf64_Addr)addr));

    Elf64_Phdr *phdr = malloc(l->l_phlen);
    pread(nl->fd, (void *)phdr, l->l_phlen, l->l_phoff);

    struct loadcmd loadcmds[l->l_phnum];
    int nloadcmds = 0;
    bool has_holes = false;

    Elf64_Phdr *ph;
    /* travese the segments! */
    for (ph = phdr; ph < &phdr[l->l_phnum]; ++ph)
    {
        switch (ph->p_type)
        {
        case PT_LOAD:
        {
            /* load and add it to loadcmds */
            struct loadcmd *c = &loadcmds[nloadcmds++]; //get the current loaction in loadcmds
            /* TODO: pagesize are set to 4096 for simplicity */
            c->mapstart = ALIGN_DOWN(ph->p_vaddr, 4096);
            c->mapend = ALIGN_UP(ph->p_vaddr + ph->p_filesz, 4096);
            c->dataend = ph->p_vaddr + ph->p_filesz;
            c->allocend = ph->p_vaddr + ph->p_memsz;
            c->mapoff = ALIGN_DOWN(ph->p_offset, 4096);

            /* hey ELF, is it your last segment? */
            if (nloadcmds > 1 && c[-1].mapend != c->mapstart)
                has_holes = true;

            /* setting the protection of this segment. may have errors */
            c->prot = 0;
            c->prot |= (ph->p_flags & PF_R) >> 2;
            c->prot |= ph->p_flags & PF_W;
            c->prot |= (ph->p_flags & PF_X) << 2;

            break;
        }
        case PT_PHDR:
            /* the PHT itself, usually the first one */
            /* or it may not exist at all, in which case it says "use ld.so to find it in a LOAD segment" */
            l->l_phdr = (void *)ph->p_vaddr;
            break;

        case PT_DYNAMIC:
            /* the dynamic section */
            l->l_ld = (void *)ph->p_vaddr;
            l->l_ldnum = ph->p_memsz / sizeof(Elf64_Dyn);
            break;

        default:
            break;
        }
    }

    /* now map LOAD segments into memory */
    size_t maplength = loadcmds[nloadcmds - 1].allocend - loadcmds[0].mapstart;
    map_segments(l, addr, loadcmds, nloadcmds, has_holes, nl->fd, maplength);

    if (l->l_phdr == NULL)
        /* This is expected. I don't know why l_phdr is not allocated 
     * It's probably because of capatiabilty issue, sigh
     */
        l->l_phdr = phdr;

    /* After loading, the absolute address is set, now relocate the PHT */
    l->l_phdr = (Elf64_Phdr *)((Elf64_Addr)l->l_phdr + l->l_addr);

    /* rebasing ld to prevent SIGSEGV */
    l->l_ld = (Elf64_Dyn *)((Elf64_Addr)l->l_ld + l->l_addr);
    /* indexing the l_ld into l_info */
    fill_info(l);

    setup_hash(l);

    return l;
}
