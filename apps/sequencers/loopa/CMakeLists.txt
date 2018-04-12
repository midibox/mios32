cmake_minimum_required(VERSION 2.8.4)
project(mbloopa)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_custom_target(mbloopa COMMAND cd /home/code/midibox/mios32/playground/hawkeye/mbloopa&&export MIOS32_PATH=/home/code/midibox/mios32/trunk&&export MIOS32_PATH=/home/code/midibox/mios32/trunk&&export MIOS32_BIN_PATH=$MIOS_PATH/bin&&export MIOS32_FAMILY=STM32F4xx&&export MIOS32_PROCESSOR=STM32F407VG&&export MIOS32_BOARD=MBHP_CORE_STM32F4&&export MIOS32_LCD=universal&&export MIOS32_GCC_PREFIX=arm-none-eabi&&export PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/sbin:/usr/local/bin:/usr/local/stm32f4/bin&&gmake -j1)
        #gmake -j4 -C ${mbloopa_SOURCE_DIR})
#CLION_EXE_DIR=${PROJECT_BINARY_DIR})