::cd br16
%~d0

cd %~p0
if exist ..\..\..\main.or32 (
   copy ..\..\..\main.or32 
   del ..\..\..\main.or32
)
del br16.bin br16.lst br16.map

llvm-objdump.exe -disassemble main.or32 > br16.lst
llvm-objcopy.exe -O binary -j .code  main.or32  br16.bin
llvm-objcopy.exe -O binary -j .data  main.or32  data.bin
llvm-objcopy.exe -O binary -j .data1  main.or32  data1.bin

llvm-objdump.exe -section-headers  main.or32

copy br16.bin/b + data.bin/b + data1.bin/b  sdram.app  
 
::pscp bingquan@192.168.8.229:jl_br17/dual_single_thread/code/tools/br16/sdram.app sdram.app
 
isd_download.exe -f uboot.boot sdram.app -tonorflash -dev br16 -boot 0x2000 -div6 -wait 300





