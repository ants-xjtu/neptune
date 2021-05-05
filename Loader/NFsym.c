/* Search and return a symbol inside a NF_link_map
 * May turn to hash table in the future
 * But now, for simplicity, I just search it linearly
 */

#include "NFlink.h"
#include <elf.h>
#include <stdlib.h> //for NULL
#include <string.h>

void *NFsym(void *ll, const char *s)
{
    struct NF_link_map *l = ll;
    Elf64_Sym *symtab = (void *)l->l_info[DT_SYMTAB]->d_un.d_ptr;
    const char *strtab = (void *)l->l_info[DT_STRTAB]->d_un.d_ptr;

    Elf64_Sym *curr_sym = symtab;
    //strtab is at higher address, so this can serve as a end point
    while ((void *)curr_sym < (void *)strtab)
    {
        Elf64_Word name = curr_sym->st_name;
        if (!strcmp(strtab + name, s))
        {
            //found it
            //FIXME: you may want to check if the symbol is valid. E.g., is it UND?
            //symbol table cannot be modified. All we have done is just finishing got at around 0x200100
            if (curr_sym->st_value == 0)
            {
                // und symbol, find in got
                // I'm rebuilding the GOT because nfsym is a rare operation
                Elf64_Addr gotStart = l->l_info[DT_RELA]->d_un.d_ptr;
                Elf64_Addr gotSize = l->l_info[DT_RELASZ]->d_un.d_val;
                Elf64_Addr nrelative = 0;
                if(l->l_info[34])
                    nrelative = l->l_info[34]->d_un.d_val;
                Elf64_Rela *dataStart = (Elf64_Rela *)gotStart + nrelative;
                Elf64_Rela *it = dataStart;
                while (it < (Elf64_Rela *)gotStart + gotSize)
                {
                    Elf64_Xword idx = it->r_info;
                    Elf64_Sym *tmp_sym = &symtab[idx >> 32];
                    Elf64_Word gotName = tmp_sym->st_name;
                    const char *gotRealName = strtab + gotName;
                    if (strcmp(gotRealName, s) == 0)
                    {
                        // return the real, rebased address
                        void **dest = (void **)(l->l_addr + it->r_offset);
                        // printf("name: %s, find real symbol address %p in GOT\n", s, *dest);
                        return *dest;
                    }
                    it++;
                }
                
            }
            // printf("name: %s, st_value: %#lx\n", s, curr_sym->st_value);
            return (void *)(curr_sym->st_value + l->l_addr);
        }
        curr_sym++;
    }
    return NULL; //find nothing
}