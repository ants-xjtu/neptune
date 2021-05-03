#include "Common.h"

// https://code.woboq.org/userspace/glibc/sysdeps/unix/sysv/linux/x86/arch-pkey.h.html
/* Return the value of the PKRU register.  */
static inline unsigned int
pkey_read(void)
{
    unsigned int result;
    __asm__ volatile(".byte 0x0f, 0x01, 0xee"
                     : "=a"(result)
                     : "c"(0)
                     : "rdx");
    return result;
}

/* Overwrite the PKRU register with VALUE.  */
static inline void
pkey_write(unsigned int value)
{
    __asm__ volatile(".byte 0x0f, 0x01, 0xef"
                     :
                     : "a"(value), "c"(0), "d"(0));
}

int UpdatePkru(int key, unsigned int rights, unsigned int *pkru)
{
    if (key < 0 || key > 15 || rights > 3)
    {
        // __set_errno(EINVAL);
        return -1;
    }
    unsigned int mask = 3 << (2 * key);
    // unsigned int pkru = pkey_read();
    *pkru = (*pkru & ~mask) | (rights << (2 * key));
    // pkey_write(pkru);
    return 0;
}

void UpdatePkey(unsigned int workerId)
{
    if (!enablePku)
    {
        return;
    }
    unsigned int pkru = pkey_read();
    UpdatePkru(runtimePkey, PKEY_DISABLE_WRITE, &pkru);
    for (int i = 0; moonDataList[i].id != -1; i += 1)
    {
        UpdatePkru(
            moonDataList[i].pkey,
            moonDataList[i].id == workerDataList[workerId].current
                ? 0
                : PKEY_DISABLE_WRITE,
            &pkru);
    }
    pkey_write(pkru);
}

void DisablePkey(int force)
{
    if (!enablePku && !force)
    {
        return;
    }
    unsigned int pkru = pkey_read();
    UpdatePkru(runtimePkey, 0, &pkru);
    for (int i = 0; moonDataList[i].id != -1; i += 1)
    {
        UpdatePkru(moonDataList[i].pkey, 0, &pkru);
    }
    pkey_write(pkru);
}
