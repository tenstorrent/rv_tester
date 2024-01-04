OUTPUT_ARCH( "riscv" )
ENTRY( entry )
SECTIONS
{
    .whereto 0x258 :
    {
        *(.whereto)
    }   
    . = 0x800;
    .text :
    {
        *(.text)
    }
    _end = .;
}