OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

PHDRS
{
    null    PT_NULL    FLAGS(0) ;                   /* Null segment */
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
    rodata  PT_LOAD    FLAGS((1 << 2)) ;            /* Read only */
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
}
 
SECTIONS
{
    /* relocate to the higher half */
    . = 0xffffffff80100000;
    _kernel_start = .;
 
    .text : {
        *(.text*)
    } :text
 
    . = ALIGN(4096);
 
    /* We place the .stivalehdr section containing the header in its own section, */
    /* and we use the KEEP directive on it to make sure it doesn't get discarded. */
    .stivalehdr : {
        KEEP(*(.stivalehdr))
    } :rodata
 
    .rodata : {
        *(.rodata*)
    } :rodata
 
    /* Move to the next memory page for .data */
    . = ALIGN(4096);
 
    .data : {
        *(.data*)
    } :data
 
    _bss_start = .;
    .bss : {
        *(COMMON)
        *(.bss*)
    } :data
    _bss_end = .;

    . = ALIGN(4096);
    _kernel_end = .;
}
