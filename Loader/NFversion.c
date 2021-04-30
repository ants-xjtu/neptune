//setting up the l_verdef and l_verneed, so that when checking if symbols' versions match,
//we go to the l_verdef of dependency and the l_verneed of currently relocating link map

#include "NFlink.h"
#include <stdlib.h>
#include <elf.h>

void NFversion(struct NF_link_map *l)
{
    //const char* str = l->l_info[DT_STRTAB]->d_un.d_ptr;
    Elf64_Dyn *dyn_def = l->l_info[38];
    Elf64_Dyn *dyn_need = l->l_info[37];

    if(dyn_def != NULL)
    {
        Elf64_Verdef *def = (Elf64_Verdef *)dyn_def->d_un.d_ptr;
        /* here is how all the things inside an ELF go:
           a Elf64_VERDEF will be followed by several ELF64_Veraux, specified by vd_cnt
           the aux object stores the real name of the version, which we don't really care for we have hash
           itself is stored in a linked list, with vd_next pointing to the next one
         */
        while(1)
        {
            Elf64_Half ndx = def->vd_ndx & 0x7fff; //don't ask me why... ELF spec is weird
            //Elf64_Half cnt = def->vd_cnt;
            l->l_verdef[ndx] = def->vd_hash;
            if(def->vd_next == 0)
                break;

            def = (Elf64_Verdef *)((char *)def + def->vd_next);
        }
    }
    if(dyn_need != NULL)
    {
        Elf64_Verneed *need = (Elf64_Verneed *)dyn_need->d_un.d_ptr;
        while(1)
        {
            /* it is more understandable at VERNEED 
               several aux follow the Elf64_VERNEED because they are from the same dependency
             */
            Elf64_Word next_aux = need->vn_aux;
            Elf64_Word next_need = need->vn_next;
            Elf64_Vernaux *aux = (Elf64_Vernaux *)((char *)need + next_aux);
            while(1)
            {
                l->l_verneed[aux->vna_other & 0x7fff] = aux->vna_hash; //this might be error-prone
                if(aux->vna_next == 0)
                    break;
                aux = (Elf64_Vernaux *)((char *)aux + aux->vna_next);
            }
            if(need ->vn_next == 0)
                break;
            need = (Elf64_Verneed *)((char *)need + need->vn_next);
        }
    }

}