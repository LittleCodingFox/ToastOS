SECTIONS
{
    . = 0x80000;
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    PROVIDE(_data = .);
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        d = ALIGN(16);
        /* place these manually, gcc would otherwise waste lots of memory */
        . = ALIGN(4096);
        __bootboot = .;
        . += 4096;
        __environment = .;
        . += 4096;
        __paging = .;
        . += (23*4096);
        __corestack = .;
        . += 4096;
        __bss_end = .;
    }
    _end = .;

   /DISCARD/ :
   {
        *(.comment)
        *(.gnu*)
        *(.note*)
        *(.eh_frame*)
   }
}
__bss_size = (__bss_end - __bss_start)>>3;
