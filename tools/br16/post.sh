
cd tools/br16
${OBJDUMP} -disassemble main.or32 > br16.lst
${OBJCOPY} -O binary -j .code  main.or32  br16.bin
${OBJCOPY} -O binary -j .data  main.or32  data.bin
${OBJCOPY} -O binary -j .data1  main.or32  data1.bin

${OBJDUMP} -section-headers  main.or32

cat br16.bin data.bin data1.bin > flash.app  
