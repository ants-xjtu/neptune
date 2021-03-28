/* Do relocation for a loaded elf
 * Thanks to the info in l_info, we now know many extra information, like the address of synbol table
 * 
 * Note that l_info is just a index for fast, or random access
 * In limited, user version of link_map, l_ld exists. This contains enough info for symbol lookup
 * even we are not dynamic linker.
 * 
 * Simply breaking the relocation into 2 types: mess/do not mess with the GOT
 * Former one is called relative relocation: though variables and functions inside a so is access via rip
 *      other libraries will search its symbol table 
 * Latter one is more familiar: We first find the map containing the symbol, find its actual address, 
 *      and fill the GOT 
 * 
 * readelf --relocs can show the relocation entries
*/

#include <dlfcn.h>
#include <elf.h>
#include <link.h>  //we have to search link_map here
#include <stdint.h>
#include <stdio.h>
#include <string.h>  //for strcmp

#include "NanoNF.h"

extern Elf64_Addr REAL_MALLOC;  //ask for external, real malloc address

/* a uniform structure for .data and .text relocation */
struct uniReloc {
    Elf64_Addr start;
    Elf64_Addr size;
    Elf64_Xword nrelative;  //count of relative relocs, omitted in .text
    int lazy;               //lazy reloc, omitted in .data
} ranges[2] = {{0, 0, 0, 0}, {0, 0, 0, 0}};

struct rela_result {
    struct NF_link_map *l;
    Elf64_Sym *s;
    Elf64_Addr addr;
};

/* A struct to re-establish the hash table for a opened link_map
    This is dumb, but I haven't found a better way */
struct hash_table {
    uint32_t l_nbuckets;
    Elf32_Word l_gnu_bitmask_idxbits;
    Elf32_Word l_gnu_shift;
    const Elf64_Addr *l_gnu_bitmask;
    const Elf32_Word *l_gnu_buckets;
    const Elf32_Word *l_gnu_chain_zero;
};

static void rebuild_hash(struct link_map *l, struct hash_table *h) {
    Elf64_Dyn *dyn = l->l_ld;
    while (dyn->d_tag != DT_GNU_HASH)
        dyn++;

    Elf32_Word *hash32 = (void *)dyn->d_un.d_ptr;
    h->l_nbuckets = *hash32++;
    Elf32_Word symbias = *hash32++;
    Elf32_Word bitmask_nwords = *hash32++;

    h->l_gnu_bitmask_idxbits = bitmask_nwords - 1;
    h->l_gnu_shift = *hash32++;

    h->l_gnu_bitmask = (Elf64_Addr *)hash32;
    hash32 += __ELF_NATIVE_CLASS / 32 * bitmask_nwords;

    h->l_gnu_buckets = hash32;
    hash32 += h->l_nbuckets;
    h->l_gnu_chain_zero = hash32 - symbias;
}

/* hash a string, borrowed from dl-lookup.c */
static uint_fast32_t
dl_new_hash(const char *s) {
    uint_fast32_t h = 5381;
    for (unsigned char c = *s; c != '\0'; c = *++s)
        h = h * 33 + c;
    return h & 0xffffffff;
}

static int lookup_linkmap(struct NF_link_map *l, const char *name, struct rela_result *result, const ProxyRecord *records,
                          Elf64_Word rela_hash) {
    for (int i = 0; records[i].pointer; i += 1) {
        if (strcmp(records[i].name, name) == 0) {
            result->addr = (Elf64_Addr)records[i].pointer;
            return 1;
        }
    }

    if (strcmp(l->l_name, "libc.so.6") == 0) {
        /* an early interception of libc, so that though libc is loaded as a NF_link_map,
           it is never be used. The actual job is done by dlopen and dlsym
           Use this technique to clobber all the DSO(only the direct ones for dlopen handles dependencies) 
           that you think to be wrong */
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        void *res;
        if (res = dlsym(handle, name)) {
            result->addr = (Elf64_Addr)res;
            return 1;
        } else {
            return 0;
        }
    }
    // if(strcmp(l->l_name, "libpcre2-8.so.0") == 0)
    // {
    //     /* an early interception of libc, so that though libc is loaded as a NF_link_map,
    //        it is never be used. The actual job is done by dlopen and dlsym
    //        Use this technique to clobber all the DSO(only the direct ones for dlopen handles dependencies)
    //        that you think to be wrong */
    //     void *handle = dlopen("libpcre2-8.so.0", RTLD_LAZY);
    //     void *res;
    //     if(res = dlsym(handle, name))
    //     {
    //         result->addr = (Elf64_Addr)res;
    //         return 1;
    //     }
    //     else
    //     {
    //         return 0;
    //     }

    // }

    /* search the symbol table of given link_map to find the occurrence of the symbol
        return 1 upon success and 0 otherwise */
    Elf64_Dyn *dyn = l->l_ld;
    while (dyn->d_tag != DT_STRTAB)
        ++dyn;
    /* string table is in a flatten char array */
    const char *strtab = (void *)dyn->d_un.d_ptr;
    while (dyn->d_tag != DT_SYMTAB)
        ++dyn;
    /* dyn is now at the symbol table entry */
    /* note that only dynamic symbol table will be mapped, and DT_SYMTAB also implies the dynamic one */
    Elf64_Sym *symtab = (void *)dyn->d_un.d_ptr;

    /* playing with hash table here, borrowed from dl-lookup.c */
    /* But first of all, re-set the hash table here */
    /* we don't need to rebuild the hash because we're relocating with NF_link_map now */
    //struct hash_table h;
    //rebuild_hash(l, &h);

    uint_fast32_t new_hash = dl_new_hash(name);
    Elf64_Sym *sym;
    const Elf64_Addr *bitmask = l->l_gnu_bitmask;
    uint32_t symidx;

    Elf64_Addr bitmask_word = bitmask[(new_hash / __ELF_NATIVE_CLASS) & l->l_gnu_bitmask_idxbits];
    unsigned int hashbit1 = new_hash & (__ELF_NATIVE_CLASS - 1);
    unsigned int hashbit2 = ((new_hash >> l->l_gnu_shift) & (__ELF_NATIVE_CLASS - 1));
    if ((bitmask_word >> hashbit1) & (bitmask_word >> hashbit2) & 1) {
        Elf32_Word bucket = l->l_gnu_buckets[new_hash % l->l_nbuckets];
        if (bucket != 0) {
            const Elf32_Word *hasharr = &l->l_gnu_chain_zero[bucket];
            do {
                if (((*hasharr ^ new_hash) >> 1) == 0) {
                    symidx = hasharr - l->l_gnu_chain_zero;
                    /* now, symtab[symidx] is the current symbol
                        hash table has done all work and can be stripped */
                    const char *symname = strtab + symtab[symidx].st_name;
                    /* FIXME: You may also want to check the visibility and strong/weak of the found symbol
                        but... not now */
                    /* FIXME: Please make sure no local symbols like "tmp" will be accessed here! */
                    if (!strcmp(symname, name)) {
                        // commented out by Qcloud1223 on Mar.27, for the original symbol interface
                        // is quite arbitrary
                        // Elf64_Versym *cidx = (Elf64_Versym *)l->l_info[36]->d_un.d_ptr;
                        // Elf64_Word real_hash = l->l_verdef[cidx[symidx] & 0x7fff];
                        // if(rela_hash == real_hash || real_hash == 0 || rela_hash == 0)
                        // {
                        result->s = &symtab[symidx];
                        result->addr = result->s->st_value + l->l_addr;
                        return 1;
                        // }
                        // printf("find a symbol with same name but not same version when reloating %s using %s", name, l->l_name);
                    }
                }
            } while ((*hasharr++ & 1u) == 0);
        }
    }
    return 0;  //not this link_map
}

static void do_reloc(struct NF_link_map *l, struct uniReloc *ur, const ProxyRecord *records) {
    Elf64_Rela *r = (void *)ur->start;
    Elf64_Rela *r_end = r + ur->nrelative;
    Elf64_Rela *end = (void *)(ur->start + ur->size);  //the end of .rel.dyn
    if (ur->nrelative) {
        /* do relative reloc here */

        /* start points to the beginning of .rel.dyn, and the thing in memory should be parsed as Rela entries */

        for (Elf64_Rela *it = r; it < r_end; it++) {
            Elf64_Addr *tmp = (void *)(l->l_addr + it->r_offset);  //get the address we need to fill
            *tmp = l->l_addr + it->r_addend;                       //fill in the blank with rebased address
        }
    }

    /* currently, I only want to search libc.so */
    //void *handle = dlopen("libc.so.6", RTLD_LAZY);

    /* do actual reloc here */
    Elf64_Sym *symtab = (Elf64_Sym *)l->l_info[DT_SYMTAB]->d_un.d_ptr;
    const char *strtab = (const char *)l->l_info[DT_STRTAB]->d_un.d_ptr;
    for (Elf64_Rela *it = r_end; it < end; it++) {
        Elf64_Xword idx = it->r_info;
        Elf64_Sym *tmp_sym = &symtab[idx >> 32];  //from dynamic symbol table get the symbol
        Elf64_Word name = tmp_sym->st_name;
        const char *real_name = strtab + name;  //from string table get the real name of the symbol
        Elf64_Versym *versym = (Elf64_Versym *)l->l_info[36]->d_un.d_ptr;
        int rela_idx = versym[idx >> 32];
        Elf64_Word rela_hash = l->l_verneed[rela_idx & 0x7fff];

        const unsigned long int r_type = it->r_info & 0xffffffff;
        if (r_type == R_X86_64_IRELATIVE) {
            //the address for ifunc is l->l_addr + it->r_addend
            Elf64_Addr value = l->l_addr + it->r_addend;
            value = ((ElfW(Addr)(*)(void))value)();
            void *dest = (void *)(l->l_addr + it->r_offset);
            //pointing to the right address
            *(Elf64_Addr *)dest = value;
            continue;
        }

        struct NF_link_map **curr_search = l->l_search_list;
        while (*curr_search) {
            struct rela_result result;
            int res = lookup_linkmap((struct NF_link_map *)*curr_search, real_name, &result, records, rela_hash);
            if (res) {
                /* check different types and fix the address for rela entry here */
                /* two main types: JUMP_SLO and GLOB_DAT are in one case fallthrough, so we don't switch for now */
                void *dest = (void *)(l->l_addr + it->r_offset);  //destination address to write
                *(Elf64_Addr *)dest = result.addr + it->r_addend;
                //dlclose(curr_search); //dlopened in NFdeps before
                break;  //first hit wins
            }
            ++curr_search;
        }
    }
    //dlclose(handle);
}

void NFreloc(struct NF_link_map *l, const ProxyRecord *records) {
    /* set up range[0] for relative reloc and global vars */
    if (l->l_info[DT_RELA]) {
        ranges[0].start = l->l_info[DT_RELA]->d_un.d_ptr;
        ranges[0].size = l->l_info[DT_RELASZ]->d_un.d_val;
        ranges[0].nrelative = l->l_info[34]->d_un.d_val;  //relacount is now at 34
    }
    /* set up range[1] for function reloc */
    if (l->l_info[DT_PLTREL])  //TODO: Also check reloc type: it is REL/RELA? Only 2 options?
    {
        ranges[1].start = l->l_info[DT_JMPREL]->d_un.d_ptr;
        ranges[1].size = l->l_info[DT_PLTRELSZ]->d_un.d_val;
        //TODO: check lazy or not here
    }

    /* do actucal reloc here using ranges set up */
    for (int i = 0; i < 2; ++i)
        do_reloc(l, &ranges[i], records);

    /* UPD: this is totally wrong, for unload search list should be in the destructor of the NF */
    /* NFclose function should take care of this */
    /* now the reloc is done, dlclose the dlopened objs before */
    /* struct link_map **lm = l -> l_search_list;
    while(*lm)
    {
        dlclose(*lm);
        lm++;
    } */
    //TODO: add a mprotect to protect the already relocated entries from writing
}