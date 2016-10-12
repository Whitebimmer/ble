obj_S = \
 
# objs = \
   # Baseband/common/$(CHIPID)/fpga_analog.o \

objs = \
   btstack/Baseband/common/$(CHIPID)/analog.o \
   cpu/$(CHIPID)/startup.o \

objs += \
   btstack/Baseband/common/$(CHIPID)/RF.o \
   btstack/Baseband/BLE/$(CHIPID)/RF_ble.o \
   btstack/Baseband/BLE/$(CHIPID)/analog_ble.o \
   btstack/Controller/BLE/ble_hci.o \
   btstack/Controller/BLE/ble_main.o \
   btstack/Controller/BLE/link_layer.o \
   btstack/Controller/common/$(CHIPID)/bt_power_driver.o \
   btstack/Controller/common/bt_power.o \
   btstack/Controller/common/bd_memory.o \
   btstack/Host/BLE/hci.o \
   btstack/Host/BLE/h4_transport.o \
   btstack/Host/BLE/l2cap_le.o \
   btstack/Host/BLE/ble_peripheral_test.o \
   btstack/Host/BLE/le_device_db_memory.o \
   btstack/Host/BLE/btstack_memory.o \
   btstack/Host/common/hci_cmds.o \
   btstack/Host/common/linked_list.o \
   btstack/Host/common/memory_pool.o \
   btstack/Host/common/utils.o \
   btstack/Protocol/BLE/att.o \
   btstack/Protocol/BLE/att_dispatch.o \
   btstack/Protocol/BLE/att_server.o \
   btstack/Protocol/BLE/sm.o \
   btstack/LL_TS_4_2_1_hci_test.o \
   btstack/hcitest_uart.o \
   cpu/$(CHIPID)/irq.o \
   cpu/$(CHIPID)/device.o \
   cpu/$(CHIPID)/clock.o \
   cpu/$(CHIPID)/board/main.o \
   cpu/$(CHIPID)/driver/clock.o \
   cpu/$(CHIPID)/driver/hwi.o \
   cpu/$(CHIPID)/driver/timer.o \
   cpu/$(CHIPID)/driver/uart.o \
   driver/platform.o \
   driver/interrupt.o \
   driver/init.o \
   lib/delay.o \
   lib/lbuf.o \
   lib/run_loop.o \
   lib/vsprintf.o \
   lib/stdio/fcvt.o \
   lib/stdio/printf-stdarg.o \
   os/thread.o \
   os/sys_timer.o \

INCLUDES += \
-Iinclude \
-Icpu/$(CHIPID) \
-Icpu/$(CHIPID)/board \
-Icpu/$(CHIPID)/driver \
-Icpu/$(CHIPID)/include \
-Ilib/stdio \
-Ios/ucos \
-Ibtstack/ \
-Ibtstack/Baseband/include/ble/$(CHIPID)/ \
-Ibtstack/Baseband/include/common/$(CHIPID)/ \
-Ibtstack/Controller/include/ \
-Ibtstack/Controller/include/common/ \
-Ibtstack/Controller/include/common/$(CHIPID)/ \
-Ibtstack/Host/include/ \
-Ibtstack/Host/include/common/ \
-Ibtstack/Protocol/include/ \
-Ibtstack/Profile/include/ \
 
CC_ARG = \
-DNOFLOAT \
-DEMBEDDED \
-DBR17 \
-DFPGA \

GEN_LIB=n
include $(MAKE_RULE)
 
