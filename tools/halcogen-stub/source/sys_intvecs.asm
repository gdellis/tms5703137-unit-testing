;-------------------------------------------------------------------------------
; sys_intvecs.asm - stand-in for HALCoGen's vector table (see ../README.md)
;
; Reset branches to the TI run-time library's _c_int00 (stack, .cinit/.bss init,
; main). Every other exception spins. Real HALCoGen output routes IRQ/FIQ through
; the VIM and reset through its own start-up code; this only has to assemble, link
; and put something sensible at address 0.
;-------------------------------------------------------------------------------

    .sect ".intvecs"
    .arm

    .ref _c_int00

    .def resetEntry
resetEntry
    b   _c_int00
undefEntry
    b   undefEntry
svcEntry
    b   svcEntry
prefetchEntry
    b   prefetchEntry
dataEntry
    b   dataEntry
reservedEntry
    b   reservedEntry
irqEntry
    b   irqEntry
fiqEntry
    b   fiqEntry
