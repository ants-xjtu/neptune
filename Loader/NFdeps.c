/* Examine the dependency of a given so, and write it into somewhere in the link map
    may also configure the search path of it */

#include "NFlink.h"
#include <elf.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

void map_deps(struct NF_link_map *l)
{
    /* Before moving forward, one should first figure out the difference between dependency and DT_NEEDED,
       which is to say, the indirect/direct references.
       In libc, a dependency of a shared object is examined in dl-load.c:_dl_map_object_deps,
       which first deals with DT_NEEDED, and then does a BFS for a flatten list.

       For an NF however, we do not have to take care of BFS because we try to dlopen the DT_NEEDED entries,
       dlopen will resolve it, expect for a missing DT_RUNPATH. Under this case we may just try again with 
       manually fuse the RUNPATH on.
    */
    /* May the layout eventually be like this?
    *     --------------
    *     |   stack    |
    *     --------------
    *     |   NF deps  |
    *     --------------
    *     |   NFs      |
    *     --------------
    *     |   main heap|
    *     --------------
   */
    Elf64_Dyn *i = l->l_ld;
    const char *strtab = (const char *)l->l_info[DT_STRTAB]->d_un.d_ptr;
    int nrunp = 0, nneeded = 0;
    do //do statistics fast
    {
        if (i->d_tag == DT_NEEDED)
            nneeded++;
        if (i->d_tag == DT_RUNPATH)
            nrunp++;
        ++i;
    } while (i->d_tag != DT_NULL);

    l->l_runpath = (const char **)malloc(nrunp * sizeof(char *));
    l->l_search_list = (struct NF_link_map **)calloc(nneeded + 1, sizeof(struct NF_link_map *));

    i = l->l_ld;
    int runpcnt = 0;
    int neededcnt = 0;
    const char **failed = (const char **)calloc(nneeded, sizeof(char *));

    while (i->d_tag != DT_NULL) //the reason for traversing instead of using l->info is that there may be multiple DT_NEEDED
    {
        if (i->d_tag == DT_NEEDED)
        {
            const char *name = strtab + i->d_un.d_val; //get the name of the so from strtab
            void *handle = dlopen(name, RTLD_LAZY);    //is the flag right?
            if (!handle)
            {
                //correctly notice outside the program that an error occured, plz search in customed path
                failed[neededcnt] = name;
            }
            l->l_search_list[neededcnt++] = (struct NF_link_map *)handle;
        }
        else if (i->d_tag == DT_RUNPATH) //RPATH and RUNPATH are similar, only doing the latter here
        {
            l->l_runpath[runpcnt++] = strtab + i->d_un.d_val; //find RUNPATH in strtab
        }
        ++i;
    }

    const char **ch = failed;
    for (int j = 0; j < nneeded; j++)
    {
        if (*ch != NULL)
        {
            //find in the RUNPATH
            int found = 0;
            for (int k = 0; k < nrunp; k++)
            {
                char tmppath[128];
                strcpy(tmppath, l->l_runpath[k]);
                strcat(tmppath, "/");
                strcat(tmppath, failed[j]);
                void *handle_again = dlopen(tmppath, RTLD_LAZY);
                if (handle_again)
                {
                    found = 1;
                    l->l_search_list[j] = (struct NF_link_map *)handle_again;
                    break;
                }
            }
            if (!found)
            {
                //error here, nowhere to find the specified so
            }
        }
        ch++;
    }
}