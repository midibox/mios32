cmake_minimum_required(VERSION 2.8.4)
project(loopa)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(/usr/include/c++/v1 /usr/lib/clang/6.0.1/include /usr/include)
add_custom_target(loopa ALL COMMAND cd /home/code/mios32/trunk/apps/sequencers/LoopA&&export MIOS32_PATH=/home/code/mios32/trunk&&export MIOS32_BIN_PATH=$MIOS_PATH/bin&&export MIOS32_FAMILY=STM32F4xx&&export MIOS32_PROCESSOR=STM32F407VG&&export MIOS32_BOARD=MBHP_CORE_STM32F4&&export MIOS32_LCD=universal&&export MIOS32_GCC_PREFIX=arm-none-eabi&&export BUILDENV_HAWKEYE=1&&export PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/sbin:/usr/local/bin:/usr/local/stm32f4/bin&&gmake -DBUILDENV_HAWKEYE -j1&&cp project.hex /home/xfer&&echo done)

add_executable(dummyForClionHeaders
	convPNM.C
   screen.c
	app.c        
	app_lcd.c    
	loopa.c
        ui.c
	hardware.c   
	screen.c     
	terminal.c
	voxelspace.c
)

target_include_directories(dummyForClionHeaders PUBLIC 
	/usr/include/c++/v1 
	/usr/lib/clang/6.0.1/include 
	/usr/include
	/home/code/mios32/trunk/FreeRTOS/Source/include
	/usr/home/code/mios32/trunk/include/mios32
	/usr/home/code/mios32/trunk/modules/midi_router
	/usr/home/code/mios32/trunk/modules/file
	/home/code/mios32/trunk/modules/sequencer
)

