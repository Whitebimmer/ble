obj_S = \
   $(CHIPID)/startup.o \
 
objs = \
   $(CHIPID)/irq.o \
   $(CHIPID)/device.o \
   $(CHIPID)/clock.o \
   $(CHIPID)/board/main.o \
   $(CHIPID)/driver/clock.o \
   $(CHIPID)/driver/hwi.o \
   $(CHIPID)/driver/timer.o \
   $(CHIPID)/driver/uart.o \
   # $(CHIPID)/driver/spi_ctl.o \
 
INCLUDES += \
-I$(CHIPID) \
-I$(CHIPID)/board \
-I$(CHIPID)/driver \
-I$(CHIPID)/include \
-I../btstack/Baseband/include/common/$(CHIPID)/ \
-I../btstack/Controller/include/common/ \
 
 
CC_ARG +=
GEN_LIB=n
include $(MAKE_RULE)
 
