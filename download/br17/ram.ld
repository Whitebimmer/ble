MEMORY
{
	flash(x)            : ORIGIN = 0x1000000,   LENGTH = 8M      /*0x5000 -- 0x10000 16k*/
    ram(rw)             : ORIGIN = 0x300000,     LENGTH = 64k
    ram2(rw)            : ORIGIN = 0x310000,     LENGTH = 40k
    ram1                : ORIGIN = 0x340000,    LENGTH = 24k-16

    zebra_ram           : ORIGIN = 0xf0000,    LENGTH = 12k-256
}

SECTIONS
{
	/* L1 memory sections */

	. = ORIGIN(flash);
	.code ALIGN(4):
	{
		*startup.o(.text)
        /* Align 4 otherwise will fill 0 not the same addr with the segament*/ 
		. = (. +3) & ~ 3;
		_early_initcall_begin = .;
		   *(.early.initcall)
		_early_initcall_end = .;
		. = (. +3) & ~ 3;
		_initcall_begin = .;
		   *(.initcall)
		_initcall_end = .;
		. = (. +3) & ~ 3;
		_late_initcall_begin = .;
		   *(.late.initcall)
		_late_initcall_end = .;

		. = (. +3) & ~ 3;
		_lmp_hci_handler_begin = .;
			*(.lmp_hci_handler)
		_lmp_hci_handler_end = .;

		. = (. +3) & ~ 3;
		_a2dp_media_codec_begin = .;
			*(.a2dp_media_codec)
		_a2dp_media_codec_end = .;

		*(.eh_frame)

		*(.ble)
		*(.bredr)
		*(.bt_power)
		*(.poweroff_flash)

		*(.zebra)

		*(.text .rodata*)
	} >flash

	_text_end = .;

	. = ORIGIN(ram);
	.data ALIGN(4):
	{
	  *(.data*)
      /* DLL func */
	  *(.dll_api)
      *(.zebra_data)
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

    . = ORIGIN(ram2);
    .db :SUBALIGN(4)
    {
        *(.db_memory)
    } > ram2

    . = ORIGIN(ram1);
    .data1 :SUBALIGN(4)
    {
		_ble_ram1_begin = .;
        *(.btmem_highly_available)
		_ble_ram1_end = .;

		_bredr_ram1_begin = .;
        *(.reg_buf)
        *(.ram1_data)
		_bredr_ram1_begin = .;

        poweroff_text_begin = . ;
        *(.poweroff_text)
        poweroff_text_end = . ;
    } > ram1

    . = ORIGIN(zebra_ram);
    .data3 :SUBALIGN(4)
    {
        *(.zb_core_ram)
    } > zebra_ram 

	data_addr = ADDR(.data);
    /*after run startup data_size will become 0*/
	data_size = SIZEOF(.data);
    /*can not use "data_size", must rename*/
	data1_size = SIZEOF(.data);
	data_begin = _text_end;

	bss_begin = ADDR(.bss ) ;
	bss_size  = SIZEOF(.bss);
	bss_end   = ADDR(.bss )  + SIZEOF(.bss);

    ram1_addr  = 0x340000;
    ram1_begin = _text_end + SIZEOF(.data) ;
    ram1_size  = 0x4000-32;
}
