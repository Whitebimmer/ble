#!/bin/sh

# ----------- ctags
# export exclude_dir

# if [ $1 = "br17" ]
# then 
    # exclude_dir=$(find -name br16)
# fi

# if [ $1 = "br16" ]
# then 
    # exclude_dir=$(find -name br17)
# fi

# echo ${exclude_dir}


# ctags -R --fields=+lS --languages=+Asm --exclude ${exclude_dir}
# ctags -R --fields=+lS --languages=+Asm 

# ----------- cscope
DIR=.
export TARGET=br17
export EXCEPT=br16
export EXCEPT1=bt16
# echo ${DIR}
# echo ${TARGET}

/cygdrive/d/cygwin64/bin/echo reloading ${TARGET} project ...

/cygdrive/d/cygwin64/bin/find ${DIR} -path "${DIR}/code/btstack/Baseband/BLE/*" ! -path "${DIR}/code/btstack/Baseband/BLE/${TARGET}*" -prune -o \
    -path "${DIR}/code/btstack/Baseband/common/*" ! -path "${DIR}/code/btstack/Baseband/common/${TARGET}*" -prune -o \
    -path "${DIR}/code/btstack/Baseband/include/ble/*" ! -path "${DIR}/code/btstack/Baseband/include/ble/${TARGET}*" -prune -o \
    -path "${DIR}/code/btstack/Baseband/include/common/ble/*" ! -path "${DIR}/code/btstack/Baseband/include/ble/common/${TARGET}*" -prune -o \
    -path "${DIR}/code/btstack/Controller/common/*" ! -path "${DIR}/code/btstack/Controller/common/${TARGET}*" -prune -o \
    -path "${DIR}/code/btstack/Controller/include/common/*" ! -path "${DIR}/code/btstack/Controller/include/common/${TARGET}*" -prune -o \
    -path "${DIR}/code/cpu/*" ! -path "${DIR}/code/cpu/${TARGET}*" -prune -o \
    -path "${DIR}/tools" -prune -o \
    -name "*.[Sch]" -print > cscope.files


# find ${DIR} ! -path "*br17*" ! -path "${DIR}/target" ! -path "${DIR}/libs/ac461x_uboot_lib" \
    # -name "*.[Sch]" -print > cscope.filess

# find ${PROJECT} -name "*.[Sch]" -print > cscope.files

# export PROJECT=$(find -path "${DIR}/apps/cpu/br17*" -prune -a -type d -print)
# export PROJECT=$(find -path "${DIR}/target" -prune -o -type d -print)

# echo ${PROJECT} > project_path.txt
# find ${PROJECT} -name "*.[Sch]" -print >> project_path.txt

# ----------- lookup file

d:/cygwin64/bin/bash sync.sh
