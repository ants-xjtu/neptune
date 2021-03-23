#ifndef PRIVATE_LOAD_H
#define PRIVATE_LOAD_H

void SetLibrary(void *loadAddress);

void *LibraryEnd();

void *LibraryOpen(const char *file, int mode);

void *LibraryFind(void *handle, const char *name);

#endif