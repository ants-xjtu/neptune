/* data structure to store the load time information of a so
 * 
 * The design logic for this file is adding elements when it is inevitably needed
 */

#ifndef _NF_link
#define _NF_link 1

/* for uint64_t to indicate address */
#include <stdint.h>
#include <elf.h> //is this necessary?
#include <stdlib.h>
#include <sys/types.h>

#define MAX_PRELOAD_NUM 8

struct NF_link_map
{
    uint64_t l_addr; //the base address of a so
    char *l_name;    //the absolute path
    //the dynamic section?
    Elf64_Dyn *l_ld;
    struct NF_link_map *l_prev, *l_next;

    uint64_t l_map_start, l_map_end; //the start and end location of a so
    uint16_t l_phnum;                //program header count
    uint16_t l_ldnum;                //load segment count
    Elf64_Addr l_phlen;              //total length of the program headers
    Elf64_Addr l_phoff;
    Elf64_Phdr *l_phdr; //the address of program header table

    /* dynamic section info */
    Elf64_Dyn *l_info[DT_NUM + 30]; //FIXME to be more general

    /* symbol hash table args */
    //Elf_Symndx
    uint32_t l_nbuckets;
    Elf32_Word l_gnu_bitmask_idxbits;
    Elf32_Word l_gnu_shift;
    const Elf64_Addr *l_gnu_bitmask;
    const Elf32_Word *l_gnu_buckets;
    const Elf32_Word *l_gnu_chain_zero;

    const char **l_runpath;             //user-specified custom search path
    struct NF_link_map **l_search_list; //a list of direct reference of this object
    Elf64_Word l_verdef[45]; //store the hash of all the defined versions
    Elf64_Word l_verneed[40]; //store the hash of all the needed versions
    //TODO: most of so do not have to include a DT_VERDEF, so lots of space is wasted. FIXME
    //also 45 is just an arbitrary number because libstdc++ define 41 of them
    int l_init;
    Elf64_Addr l_text_start; //for add-symbol-file debugging
    Elf64_Addr l_shoff;
    Elf64_Half l_shstr;
};

/* move the definition of filebuf from NFmap.c here */
struct filebuf
{
    long len;
    char buf[832] __attribute__((aligned(__alignof(Elf64_Ehdr))));
};

/* data structure to store the search path information, filled in NFusage, and later used in NFmap */
struct NF_list
{
    struct NF_link_map *map;
    struct NF_list *next;
    int done;

    int fd;
    Elf64_Addr len;
    //const char *name; //get the name from map->l_name
    //struct filebuf fb; // i don't really want to put such a large element inside the list, fix this later
};

extern struct NF_list *head, *tail;
extern void *libc_handle;
// WARNING: make sure you clear this list when you finish mapping a so
extern struct NF_link_map *preloadMap[MAX_PRELOAD_NUM];

#endif
