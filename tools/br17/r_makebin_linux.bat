%~d0

cd %~p0

pscp bingquan@192.168.8.229:jl_br17/tools/br16/flash.app sdram.app

 
isd_download.exe -f uboot.boot sdram.app -tonorflash -dev br16 -boot 0x2000 -div6 -wait 300

pause





