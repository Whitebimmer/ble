# -----------------Usage --------------------------

# 1.编译工程:		make;
# 2.清理工程:		make clean;
# --------------basic setting-----------------------
# ifeq ($(LANG),)
ifeq ($(shell uname),Linux)
export HOST_OS = linux
else
export HOST_OS = windows
endif
#

#配置下载CPU架构(bfin/pi32/pi32_lto)
export ARCH = pi32_lto

#配置下载目标平台(br16/br17)
export CHIPID = br17

#配置项目文件指向(br16/br17)
export PLATFORM = br17

#配置是否FPGA 验证平台(FPGA/CHIP)
export TYPE=CHIP

#配置蓝牙层模式(CONTROLLER_MODE/HOST_MODE/FULL_MODE)
export LE_MODE=FULL_MODE

ifeq ($(HOST_OS), linux) 
export SLASH=/
endif

ifeq ($(HOST_OS), windows) 
export SLASH=\\
endif

#配置是否显示编译信息(@/ )
export V=@


-include tools$(SLASH)compiler$(SLASH)Makefile.$(ARCH)


