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

unsigned int pkru;
int pkey_set(int key, unsigned int rights)
{
    if (key < 0 || key > 15 || rights > 3)
    {
        // __set_errno(EINVAL);
        return -1;
    }
    unsigned int mask = 3 << (2 * key);
    // unsigned int pkru = pkey_read();
    pkru = (pkru & ~mask) | (rights << (2 * key));
    // pkey_write(pkru);
    return 0;
}

void UpdatePkey()
{
    if (!enablePku)
    {
        return;
    }
    pkru = pkey_read();
    pkey_set(runtimePkey, PKEY_DISABLE_WRITE);
    for (int i = 0; moonDataList[i].id != -1; i += 1)
    {
        pkey_set(
            moonDataList[i].pkey,
            moonDataList[i].id == currentMoonId ? 0 : PKEY_DISABLE_WRITE);
    }
    pkey_write(pkru);
}

void DisablePkey()
{
    if (!enablePku)
    {
        return;
    }
    pkru = pkey_read();
    pkey_set(runtimePkey, 0);
    for (int i = 0; moonDataList[i].id != -1; i += 1)
    {
        pkey_set(moonDataList[i].pkey, 0);
    }
    pkey_write(pkru);
}
