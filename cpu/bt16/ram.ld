MEMORY
{
	flash(x)            : ORIGIN = 0x1000000,   LENGTH = 8M      /*0x5000 -- 0x10000 16k*/
    ram(rw)             : ORIGIN = 0x00000,     LENGTH = 0xa000
}

SECTIONS
{
	/* L1 memory sections */

	. = ORIGIN(flash);
	.code ALIGN(4):
	{
		*startup.o(.text)

		__early_initcall_begin = .;
			*(.early.initcall)
		__early_initcall_end = .;
		__initcall_begin = .;
			*(.initcall)
		__initcall_end = .;
		__late_initcall_begin = .;
			*(.late.initcall)
		__late_initcall_end = .;

		_lmp_hci_handler_begin = .;
			*(.lmp_hci_handler)
		_lmp_hci_handler_end = .;

		_a2dp_media_codec_begin = .;
			*(.a2dp_media_codec)
		_a2dp_media_codec_end = .;

		*(.eh_frame)

		*(.ble)
		*(.bredr)

          *(.dll_api)
		*(.text .rodata*)
	} >flash

	_text_end = .;

	. = ORIGIN(ram);
	.data ALIGN(4):
	{
	  *(.data*)
      /* DLL func */
	} > ram

	.bss ALIGN(4) :
	{
	  *(.intr_stack)
	  *(.stack)

	  _bredr_base_addr = .;
	  *(.bredr_base)
	  *(.baseband)

	  *(.bss)
	} >ram



	data_addr = ADDR(.data);
	data_begin = _text_end;
	data_size = SIZEOF(.data);

	bss_begin = ADDR(.bss ) ;
	bss_size  = SIZEOF(.bss);
	bss_end   = ADDR(.bss )  + SIZEOF(.bss);

}
