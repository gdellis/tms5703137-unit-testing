/* Stand-in for HALCoGen's sci.c: satisfies the link, drives nothing. See ../README.md. */
#include "sci.h"

void sciInit(void)
{
}

void sciSendByte(sciBASE_t *sci, uint8 byte)
{
    (void)sci;
    (void)byte;
}

uint32 sciIsTxReady(sciBASE_t *sci)
{
    (void)sci;
    return 1U;
}
