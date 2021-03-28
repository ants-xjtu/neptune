#include "NFlink.h"
#include <stdlib.h>
#include <elf.h>

void NFinit(struct NF_link_map *l, int argc, char *argv[], char **env)
{
    if(l->l_init)
        return;
    l->l_init = 1;
    void (*init_func)(int, char **, char **);
    
    if(l->l_info[DT_INIT] != NULL)
    {
        //if there is an init function address, call it now
        Elf64_Addr *addr = (Elf64_Addr *)(l->l_info[DT_INIT]->d_un.d_ptr + l->l_addr);
        //void (*init_func)() = (void *)addr;
        init_func = (void *)addr;
        init_func(argc, argv, env);
    }
    if(l->l_info[DT_INIT_ARRAY] != NULL)
    {
        //if there is a init array, call it first
        Elf64_Addr *addr = (Elf64_Addr *)(l->l_info[DT_INIT_ARRAY]->d_un.d_ptr + l->l_addr);
        int arraysz = (Elf64_Xword)l->l_info[DT_INIT_ARRAYSZ]->d_un.d_val / sizeof(Elf64_Addr);
        for(int i = 0; i < arraysz / sizeof(Elf64_Addr); i++)
        {
            //void (*init_func)() = (void *)addr[i];
            init_func = (void *)addr[i];
            init_func(argc, argv, env);
        }
    }
}