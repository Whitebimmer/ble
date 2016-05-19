
::c:\JL\dv10\bin\dv10-elf-objcopy -O binary -j .code -j .data main.or32 rom.bin


c:/JL/dv10/bin/dv10-elf-objcopy -O binary   -j .code main.or32 sdram.txt
c:/JL/dv10/bin/dv10-elf-objcopy -O binary   -j .data  main.or32 sdram.dat

copy sdram.txt/b + sdram.dat/b rom.bin

::dv10-elf-objcopy.exe -O binary -j .code  ..\main.or32 sdram.app

copy rom.bin sdram.app

sh60_isd_dwonload.exe -f uboot.boot sdram.app -tonorflash -dev bt15 -div3
