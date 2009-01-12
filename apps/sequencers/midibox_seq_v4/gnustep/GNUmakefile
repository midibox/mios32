include $(GNUSTEP_MAKEFILES)/common.make

APP_NAME=midibox_seq_v4
VERSION=1.0

midibox_seq_v4_OBJC_FILES=\
	./build/CLCDView.m\
	./build/FreeRTOS_Wrapper.m\
	./build/MIOS32_BOARD_Wrapper.m \
	./build/MIOS32_COM_Wrapper.m \
	./build/MIOS32_DELAY_Wrapper.m \
	./build/MIOS32_ENC_Wrapper.m \
	./build/MIOS32_IRQ_Wrapper.m \
	./build/MIOS32_LCD_Wrapper.m \
	./build/MIOS32_MIDI_Wrapper.m \
	./build/MIOS32_SDCARD_Wrapper.m \
	./build/MIOS32_SRIO_Wrapper.m \
	./build/MIOS32_STOPWATCH_Wrapper.m \
	./build/MIOS32_SYS_Wrapper.m \
	./build/MIOS32_TIMER_Wrapper.m \
	./build/SEQButton.m \
	./build/SEQEncoder.m \
	./build/UI.m \
	./build/main.m 

midibox_seq_v4_C_FILES=\
	./build/mios32_din.c \
	./build/mios32_dout.c \
	./build/mios32_lcd.c \
	./build/mios32_midi.c \
	./build/mios32_uart_midi.c \
	./build/dfs_sdcard.c \
	./build/dosfs.c \
	./build/mid_parser.c \
	./build/seq_bpm.c \
	./build/seq_midi_out.c \
	./build/portmidi.c \
	./build/pmutil.c \
	./build/pmwin.c \
	./build/pmwinmm.c \
	./build/porttime.c \
	./build/ptwinmm.c \
	./build/app.c \
	./build/seq_cc.c \
	./build/seq_chord.c \
	./build/seq_core.c \
	./build/seq_file.c \
	./build/seq_file_b.c \
	./build/seq_file_m.c \
	./build/seq_layer.c \
	./build/seq_lcd.c \
	./build/seq_led.c \
	./build/seq_midi_in.c \
	./build/seq_midi_port.c \
	./build/seq_midply.c \
	./build/seq_mixer.c \
	./build/seq_par.c \
	./build/seq_pattern.c \
	./build/seq_random.c \
	./build/seq_scale.c \
	./build/seq_trg.c \
	./build/seq_ui.c \
	./build/seq_ui_bpm.c \
	./build/seq_ui_edit.c \
	./build/seq_ui_fx.c \
	./build/seq_ui_fx_echo.c \
	./build/seq_ui_menu.c \
	./build/seq_ui_mixer.c \
	./build/seq_ui_mute.c \
	./build/seq_ui_opt.c \
	./build/seq_ui_pattern.c \
	./build/seq_ui_save.c \
	./build/seq_ui_shortcut.c \
	./build/seq_ui_todo.c \
	./build/seq_ui_trgasg.c \
	./build/seq_ui_trkdir.c \
	./build/seq_ui_trkdiv.c \
	./build/seq_ui_trkevnt.c \
	./build/seq_ui_trklen.c \
	./build/seq_ui_trkmode.c \
	./build/seq_ui_trkrnd.c \
	./build/seq_ui_trktran.c \
	./build/seq_ui_util.c \
	./build/printf-stdarg.c 

midibox_seq_v4_RESOURCE_FILES=\
	Info.plist

midibox_seq_v4_LOCALIZED_RESOURCE_FILES=\
	InfoPlist.strings\
	MainMenu.gorm
	
midibox_seq_v4_MAIN_MODEL_FILE=\
	MainMenu.gorm

midibox_seq_v4_LANGUAGES=\
	English

midibox_seq_v4_INCLUDE_DIRS=\
	-I./.\
	-I./../core\
	-I./../macos\
	-I././../../../../include/mios32\
	-I./../../../../modules/dosfs\
	-I./../../../../modules/blm8x8\
	-I./../../../../mios32/common\
	-I./../../../../modules/sequencer\
	-I./../../../../modules/midifile\
	-I./../../../../drivers/gnustep \
	-I../../../../drivers/gnustep/portmidi/pm_common \
	-I../../../../drivers/gnustep/portmidi/porttime \
	-I../../../../drivers/gnustep/portmidi/pm_win

before-all::
	rm -rf ./build
	rm -rf ./obj
	mkdir ./build 
	mkdir ./obj 
	mkdir ./obj/build
	cp ../macos/*.m build 
	cp *.m build 
	cp ../core/*.c build 
	cp ../../../../mios32/common/*.c build 
	cp ../../../../modules/dosfs/*.c build 
	cp ../../../../modules/midifile/*.c build 
	cp ../../../../modules/sequencer/*.c build 
	cp ../../../../drivers/gnustep/portmidi/pm_common/*.c build 
	cp ../../../../drivers/gnustep/portmidi/pm_win/*.c build 
	cp ../../../../drivers/gnustep/portmidi/porttime/*.c build 
	
# Include windows midi dll (winmm.dll)
ADDITIONAL_NATIVE_LIBS = winmm



ADDITIONAL_CPPFLAGS+= -DGNUSTEP -DMIOS32_FAMILY_EMULATION -DMIOS32_FAMILY_STR=\"EMULATION\" -DMIOS32_BOARD_STR=\"WINDOWS\"

include $(GNUSTEP_MAKEFILES)/application.make