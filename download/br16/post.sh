cd tools/br16

${OBJDUMP} -disassemble main.or32 > br16.lst
${OBJCOPY} -O binary -j .code  main.or32  br16.bin
${OBJCOPY} -O binary -j .data  main.or32  data.bin
${OBJCOPY} -O binary -j .data1  main.or32  data1.bin

${OBJDUMP} -section-headers  main.or32

# cat br17.bin data.bin data1.bin > flash.app  
cat br16.bin data.bin data1.bin > sdram.app  

rm br16.bin data.bin data1.bin main.or32

host-client -project ble_br16 -f uboot.boot sdram.app -tonorflash -dev br16 -boot 0x2000 -div6 -wait 300

rm sdram.app
