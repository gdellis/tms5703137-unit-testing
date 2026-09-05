/*----------------------------------------------------------------------------*/
/* sys_link.cmd - TMS570LS3137 memory map, as HALCoGen 4.07 generates it       */
/* (stand-in copy, see ../README.md)                                          */
/*----------------------------------------------------------------------------*/

--retain="*(.intvecs)"

MEMORY
{
    VECTORS (X)  : origin=0x00000000 length=0x00000020
    FLASH0  (RX) : origin=0x00000020 length=0x0017FFE0
    FLASH1  (RX) : origin=0x00180000 length=0x00180000
    STACKS  (RW) : origin=0x08000000 length=0x00001500
    RAM     (RW) : origin=0x08001500 length=0x0003EB00
}

SECTIONS
{
    .intvecs : {} > VECTORS
    .text    : {} > FLASH0 | FLASH1
    .const   : {} > FLASH0 | FLASH1
    .cinit   : {} > FLASH0 | FLASH1
    .pinit   : {} > FLASH0 | FLASH1
    .bss     : {} > RAM
    .data    : {} > RAM
    .sysmem  : {} > RAM
    .stack   : {} > STACKS
}
