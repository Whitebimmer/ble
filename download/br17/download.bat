::cd %1
::
@echo off

@echo ***************************************************************
@echo               BLE BR17
@echo ***************************************************************
@echo %data%

%~d0

cd %~p0

if exist %1.bin del %1.bin
if exist %1.lst del %1.lst

REM  del %1.bin %1.lst %1.map

REM  set OBJDUMP=C:\JL\pi32\bin\llvm-objdump
REM  set OBJCOPY=C:\JL\pi32\bin\llvm-objcopy

%OBJDUMP% -disassemble %1.or32 > %1.lst
%OBJCOPY% -O binary -j .code  %1.or32  %1.bin
%OBJCOPY% -O binary -j .data  %1.or32  data.bin
%OBJCOPY% -O binary -j .data1  %1.or32  data1.bin

%OBJDUMP% -section-headers  %1.or32

copy %1.bin/b + data.bin/b + data1.bin/b  sdram.app  

del %1.bin data.bin data1.bin
 
 
isd_download.exe -f uboot.boot sdram.app -tonorflash -dev br17 -boot 0x2000 -div6 -wait 300





