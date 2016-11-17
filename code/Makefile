
CC_ARG = \
-DNOFLOAT \
-DEMBEDDED \
-DBR17 \
 
# -DFPGA \

INCLUDES = \
-Iinclude \
-Icpu/$(CHIPID) \
-Icpu/$(CHIPID)/board \
-Icpu/$(CHIPID)/driver \
-Icpu/$(CHIPID)/include \
-Ios/ucos \
-Ibtstack/ \
-Ibtstack/Baseband/include/ble/$(CHIPID)/ \
-Ibtstack/Baseband/include/common/ \
-Ibtstack/Baseband/include/common/$(CHIPID)/ \
-Ibtstack/Controller/include/ \
-Ibtstack/Controller/include/common/ \
-Ibtstack/Controller/include/common/$(CHIPID)/ \
-Ibtstack/Host/include/ \
-Ibtstack/Host/include/common/ \
-Ibtstack/Protocol/include/ \
-Ibtstack/Profile/include/ \
-Iuser/include/ \
-Iuser/include/common \

LIBS = \
cpu/$(CHIPID)/driver/br16_ccm_aes.a \
 
# -Ilib/stdio \

# objs = \
   # Baseband/common/$(CHIPID)/fpga_analog.o \

objs = \
   btstack/Baseband/common/$(CHIPID)/analog.o \

objs += \
   lib/delay.o \
   lib/lbuf.o \
   lib/run_loop.o \
   lib/vsprintf.o \
   lib/stdio/fcvt.o \
   lib/stdio/printf-stdarg.o \
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
   os/thread.o \
   os/sys_timer.o \
   user/ble/ble_peripheral_test.o \
   btstack/Baseband/common/$(CHIPID)/RF.o \
   btstack/Baseband/BLE/$(CHIPID)/RF_ble.o \
   btstack/Baseband/BLE/$(CHIPID)/analog_ble.o \
   btstack/Baseband/common/$(CHIPID)/bt_power_driver.o \
   btstack/Baseband/common/$(CHIPID)/bt_power.o \
   btstack/Baseband/common/$(CHIPID)/bd_memory.o \
   btstack/Controller/BLE/ble_hci.o \
   btstack/Controller/BLE/ble_main.o \
   btstack/Controller/BLE/link_layer.o \
   btstack/Host/BLE/le_device_db_memory.o \
   btstack/Host/common/btstack_memory.o \
   btstack/Host/common/hci.o \
   btstack/Host/common/h4_transport.o \
   btstack/Host/common/l2cap.o \
   btstack/Host/common/l2cap_signaling.o \
   btstack/Host/common/hci_cmds.o \
   btstack/Host/common/linked_list.o \
   btstack/Host/common/memory_pool.o \
   btstack/Host/common/utils.o \
   btstack/Protocol/BLE/att.o \
   btstack/Protocol/BLE/att_dispatch.o \
   btstack/Protocol/BLE/att_server.o \
   btstack/Protocol/BLE/sm.o \
   #btstack/LL_TS_4_2_1_hci_test.o \
   btstack/hcitest_uart.o \

obj_S = \
   cpu/$(CHIPID)/startup.o \


GEN_LIB=n
-include $(MAKE_RULE)
 
