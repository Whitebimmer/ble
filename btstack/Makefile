obj_S = \
 
objs = \
   Baseband/common/$(CPU)/RF.o \
   Baseband/common/$(CPU)/analog.o \
   Baseband/BLE/$(CPU)/RF_ble.o \
   Baseband/BLE/$(CPU)/analog_ble.o \
   Controller/BLE/ble_hci.o \
   Controller/BLE/ble_main.o \
   Controller/BLE/link_layer.o \
   Controller/common/$(CPU)/bt_power_driver.o \
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

includes += \
-IBaseband/include/ble/$(CPU)/ \
-IBaseband/include/common/$(CPU)/ \
-IController/include/ \
-IController/include/common/ \
-IController/include/common/$(CPU)/ \
-IHost/include/ \
-IHost/include/common/ \
-IProtocol/include/ \
-IProfile/include/ \
-I./
 
DEFINS +=
CONFIG_LIB=n
include $(TOOLS)/rule.mk
 
