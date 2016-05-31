obj_S = \
   $(CPU)/startup.o \
 
objs = \
   $(CPU)/irq.o \
   $(CPU)/device.o \
   $(CPU)/clock.o \
   $(CPU)/board/main.o \
   $(CPU)/driver/clock.o \
   $(CPU)/driver/hwi.o \
   $(CPU)/driver/timer.o \
   $(CPU)/driver/uart.o \
   # $(CPU)/driver/spi_ctl.o \
 
includes += \
-I$(CPU) \
-I$(CPU)/board \
-I$(CPU)/driver \
-I$(CPU)/include \
-I../btstack/Baseband/include/common/$(CPU)/ \
-I../btstack/Controller/include/common/ \
 
 
DEFINS +=
CONFIG_LIB=n
-include $(TOOLS)/rule.mk
 
