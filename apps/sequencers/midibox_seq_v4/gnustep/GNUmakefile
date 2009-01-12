include $(GNUSTEP_MAKEFILES)/common.make

APP_NAME=midibox_seq_v4
VERSION=1.0

midibox_seq_v4_OBJC_FILES=\
	../macos/CLCDView.m\
	../macos/FreeRTOS_Wrapper.m\
	../macos/MIOS32_BOARD_Wrapper.m \
	../macos/MIOS32_COM_Wrapper.m \
	../macos/MIOS32_DELAY_Wrapper.m \
	../macos/MIOS32_ENC_Wrapper.m \
	../macos/MIOS32_IRQ_Wrapper.m \
	../macos/MIOS32_LCD_Wrapper.m \
	MIOS32_MIDI_Wrapper.m \
	../macos/MIOS32_SDCARD_Wrapper.m \
	../macos/MIOS32_SRIO_Wrapper.m \
	../macos/MIOS32_STOPWATCH_Wrapper.m \
	../macos/MIOS32_SYS_Wrapper.m \
	../macos/MIOS32_TIMER_Wrapper.m \
	../macos/SEQButton.m \
	../macos/SEQEncoder.m \
	UI.m \
	../macos/main.m 

midibox_seq_v4_C_FILES=\
	../../../../mios32/common/mios32_din.c \
	../../../../mios32/common/mios32_dout.c \
	../../../../mios32/common/mios32_lcd.c \
	../../../../mios32/common/mios32_midi.c \
	../../../../mios32/common/mios32_uart_midi.c \
	../../../../modules/dosfs/dfs_sdcard.c \
	../../../../modules/dosfs/dosfs.c \
	../../../../modules/midifile/mid_parser.c \
	../../../../modules/sequencer/seq_bpm.c \
	../../../../modules/sequencer/seq_midi_out.c \
	../../../../drivers/gnustep/portmidi/pm_common/portmidi.c \
	../../../../drivers/gnustep/portmidi/pm_common/pmutil.c \
	../../../../drivers/gnustep/portmidi/pm_win/pmwin.c \
	../../../../drivers/gnustep/portmidi/pm_win/pmwinmm.c \
	../../../../drivers/gnustep/portmidi/porttime/porttime.c \
	../../../../drivers/gnustep/portmidi/porttime/ptwinmm.c \
	../core/app.c \
	../core/seq_cc.c \
	../core/seq_chord.c \
	../core/seq_core.c \
	../core/seq_file.c \
	../core/seq_file_b.c \
	../core/seq_file_m.c \
	../core/seq_layer.c \
	../core/seq_lcd.c \
	../core/seq_led.c \
	../core/seq_midi_in.c \
	../core/seq_midi_port.c \
	../core/seq_midply.c \
	../core/seq_mixer.c \
	../core/seq_par.c \
	../core/seq_pattern.c \
	../core/seq_random.c \
	../core/seq_scale.c \
	../core/seq_trg.c \
	../core/seq_ui.c \
	../core/seq_ui_bpm.c \
	../core/seq_ui_edit.c \
	../core/seq_ui_fx.c \
	../core/seq_ui_fx_echo.c \
	../core/seq_ui_menu.c \
	../core/seq_ui_mixer.c \
	../core/seq_ui_mute.c \
	../core/seq_ui_opt.c \
	../core/seq_ui_pattern.c \
	../core/seq_ui_save.c \
	../core/seq_ui_shortcut.c \
	../core/seq_ui_todo.c \
	../core/seq_ui_trgasg.c \
	../core/seq_ui_trkdir.c \
	../core/seq_ui_trkdiv.c \
	../core/seq_ui_trkevnt.c \
	../core/seq_ui_trklen.c \
	../core/seq_ui_trkmode.c \
	../core/seq_ui_trkrnd.c \
	../core/seq_ui_trktran.c \
	../core/seq_ui_util.c \
	../../../../mios32/common/printf-stdarg.c 

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

#before-all::
#	mkdir -p ./obj

ADDITIONAL_NATIVE_LIBS = winmm
#ADDITIONAL_LDFLAGS = --add-stdcall-alias -l winmm.dll



ADDITIONAL_CPPFLAGS+= -DGNUSTEP -DMIOS32_FAMILY_EMULATION -DMIOS32_FAMILY_STR=\"EMULATION\" -DMIOS32_BOARD_STR=\"WINDOWS\"

include $(GNUSTEP_MAKEFILES)/application.make