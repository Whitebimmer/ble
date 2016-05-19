MEMORY
{
	flash(x)            : ORIGIN = 0x1000000,   LENGTH = 8M      /*0x5000 -- 0x10000 16k*/
    ram(rw)             : ORIGIN = 0x00000,     LENGTH = 0x4000
    ble_ram(rw)         : ORIGIN = 0x4000,      LENGTH =  0x4000
}

SECTIONS
{
	/* L1 memory sections */

	. = ORIGIN(flash);
	.code ALIGN(4):
	{
		*startup.o(.text)
		*main.o(.rodata*)
		*lbuf.o(.text)
		*(.text .rodata*)
	} >flash

	_text_end = .;


	. = ORIGIN(ram);
	.data ALIGN(4):
	{
	  *(.data*)
	} > ram

	.bss ALIGN(4) :
	{
	  *(.intr_stack)
	  *(.stack)
	  *(.bss)
	} >ram


	. = ORIGIN(ble_ram);
	.ble ALIGN(4):
	{
        *(.ble_ram)
	} > ble_ram



	data_addr = ADDR(.data);
	data_begin = _text_end;
	data_size = SIZEOF(.data);

	bss_begin = ADDR(.bss ) ;
	bss_size  = SIZEOF(.bss);
	bss_end   = ADDR(.bss )  + SIZEOF(.bss);

}
