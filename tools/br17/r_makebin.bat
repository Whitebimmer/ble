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

::del jl_bc51.bin
::rename jl_isd.bin jl_bc51.bin
::bfumake.exe -fi jl_bc51.bin -ld 0x0000 -rd 0x0000 -fo jl_bc51.bfu

::IF EXIST no_isd_file del jl_bc51.bin
::del no_isd_file

@rem format vm        //����VM 68K����
@rem format cfg       //����BT CFG 4K����
@rem format 0x3f0-2  //��ʾ�ӵ� 0x3f0 �� sector ��ʼ�������� 2 �� sector(��һ������Ϊ16���ƻ�10���ƶ��ɣ��ڶ�������������10����)



