#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

static uintptr_t Heap_Lower_Bound = 0x000000000001, Heap_Upper_Bound = 0xffffffffffff, Stack_Lower_Bound, Stack_Upper_Bound;
static uintptr_t SFI_START, SFI_MASK;
static void *last_alloc = NULL;

void *Gmem_alloc(size_t sz, void *start_addr);