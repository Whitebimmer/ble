obj_S = \
 
# objs = \
   # Baseband/common/$(CHIPID)/fpga_analog.o \

objs = \
   Baseband/common/$(CHIPID)/analog.o \

objs += \
   Baseband/common/$(CHIPID)/RF.o \
   Baseband/BLE/$(CHIPID)/RF_ble.o \
   Baseband/BLE/$(CHIPID)/analog_ble.o \
   Controller/BLE/ble_hci.o \
   Controller/BLE/ble_main.o \
   Controller/BLE/link_layer.o \
   Controller/common/$(CHIPID)/bt_power_driver.o \
   Controller/common/bt_power.o \
   Controller/common/bd_memory.o \
   Host/BLE/hci.o \
   Host/BLE/h4_transport.o \
   Host/BLE/l2cap_le.o \
   Host/BLE/ble_peripheral_test.o \
   Host/BLE/le_device_db_memory.o \
   Host/BLE/btstack_memory.o \
   Host/common/hci_cmds.o \
   Host/common/linked_list.o \
   Host/common/memory_pool.o \
   Host/common/utils.o \
   Protocol/BLE/att.o \
   Protocol/BLE/att_dispatch.o \
   Protocol/BLE/att_server.o \
   Protocol/BLE/sm.o \
   LL_TS_4_2_1_hci_test.o \
   hcitest_uart.o

INCLUDES += \
-IBaseband/include/ble/$(CHIPID)/ \
-IBaseband/include/common/$(CHIPID)/ \
-IController/include/ \
-IController/include/common/ \
-IController/include/common/$(CHIPID)/ \
-IHost/include/ \
-IHost/include/common/ \
-IProtocol/include/ \
-IProfile/include/ \
-I./
 
CC_ARG +=
GEN_LIB=n
include $(MAKE_RULE)
 
