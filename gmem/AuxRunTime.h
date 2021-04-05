#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

//modified by Qcloud1223 at Mar 26 14:08, set protection field to 0x700000000000 to 0x000000000000
// static uintptr_t _Gmem_Heap_Lower_Bound = 0x000000000000, _Gmem_Heap_Upper_Bound = 0x7fffffffffff, _Gmem_Stack_Lower_Bound, _Gmem_Stack_Upper_Bound;
uintptr_t _Gmem_Heap_Lower_Bound = 0x000000000000, _Gmem_Heap_Upper_Bound = 0x7fffffffffff, _Gmem_Stack_Lower_Bound, _Gmem_Stack_Upper_Bound;
uintptr_t _Gmem_SO_Lower_Bound, _Gmem_SO_Upper_Bound;
static uintptr_t _Gmem_SFI_START = 0x700000000000, _Gmem_SFI_MASK = 0x7fffffffffff;
//static void *last_alloc = NULL;

// void *Gmem_alloc(size_t sz, void *start_addr);

// extern uintptr_t _Gmem_Heap_Lower_Bound, _Gmem_Heap_Upper_Bound, _Gmem_Stack_Lower_Bound, _Gmem_Stack_Upper_Bound;
// extern uintptr_t _Gmem_SFI_START, _Gmem_SFI_MASK;
