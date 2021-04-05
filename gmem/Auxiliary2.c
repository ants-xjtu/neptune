#include <stdint.h>

uintptr_t _Gmem_Heap_Lower_Bound = 0x700000000000, _Gmem_Heap_Upper_Bound = 0x7fffffffffff, _Gmem_Stack_Lower_Bound, _Gmem_Stack_Upper_Bound;
uintptr_t _Gmem_SFI_START = 0x700000000000, _Gmem_SFI_MASK = 0x7fffffffffff;