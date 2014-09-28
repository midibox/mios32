# Automatically generated makefile, created by the Introjucer
# Don't edit this file! Your changes will be overwritten when you re-save the Introjucer project!

ifndef CONFIG
  CONFIG=Debug
endif

ifeq ($(TARGET_ARCH),)
  TARGET_ARCH := -march=native
endif

# (this disables dependency generation if multiple architectures are set)
DEPFLAGS := $(if $(word 2, $(TARGET_ARCH)), , -MMD)

ifeq ($(CONFIG),Debug)
  BINDIR := build
  LIBDIR := build
  OBJDIR := build/intermediate/Debug
  OUTDIR := build
  CPPFLAGS := $(DEPFLAGS) -D "LINUX=1" -D "DEBUG=1" -D "_DEBUG=1" -D "JUCER_LINUX_MAKE_7346DA2A=1" -I /usr/include -I /usr/include/freetype2 -I ../../JuceLibraryCode -I ../../Source -I ../../../../../../include/mios32 -I ../../../../../../modules/random -I ../../../../../../modules/notestack -I ../../../../../../modules/aout -I ../../../../../../modules/sid -I ../../../../../../modules/app_lcd/juce
  CFLAGS += $(CPPFLAGS) $(TARGET_ARCH) -g -ggdb -fPIC -O0
  CXXFLAGS += $(CFLAGS) 
  LDFLAGS += -L$(BINDIR) -L$(LIBDIR) -shared -L/usr/X11R6/lib/ -lGL -lX11 -lXext -lXinerama -lasound -ldl -lfreetype -lpthread -lrt 
  LDDEPS :=
  RESFLAGS :=  -D "LINUX=1" -D "DEBUG=1" -D "_DEBUG=1" -D "JUCER_LINUX_MAKE_7346DA2A=1" -I /usr/include -I /usr/include/freetype2 -I ../../JuceLibraryCode -I ../../Source -I ../../../../../../include/mios32 -I ../../../../../../modules/random -I ../../../../../../modules/notestack -I ../../../../../../modules/aout -I ../../../../../../modules/sid -I ../../../../../../modules/app_lcd/juce
  TARGET := MIDIboxSID
  BLDCMD = $(CXX) -o $(OUTDIR)/$(TARGET) $(OBJECTS) $(LDFLAGS) $(RESOURCES) $(TARGET_ARCH)
endif

ifeq ($(CONFIG),Release)
  BINDIR := build
  LIBDIR := build
  OBJDIR := build/intermediate/Release
  OUTDIR := build
  CPPFLAGS := $(DEPFLAGS) -D "LINUX=1" -D "NDEBUG=1" -D "JUCER_LINUX_MAKE_7346DA2A=1" -I /usr/include -I /usr/include/freetype2 -I ../../JuceLibraryCode -I ../../Source -I ../../../../../../include/mios32 -I ../../../../../../modules/random -I ../../../../../../modules/notestack -I ../../../../../../modules/aout -I ../../../../../../modules/sid -I ../../../../../../modules/app_lcd/juce
  CFLAGS += $(CPPFLAGS) $(TARGET_ARCH) -fPIC -Os
  CXXFLAGS += $(CFLAGS) 
  LDFLAGS += -L$(BINDIR) -L$(LIBDIR) -shared -L/usr/X11R6/lib/ -lGL -lX11 -lXext -lXinerama -lasound -ldl -lfreetype -lpthread -lrt 
  LDDEPS :=
  RESFLAGS :=  -D "LINUX=1" -D "NDEBUG=1" -D "JUCER_LINUX_MAKE_7346DA2A=1" -I /usr/include -I /usr/include/freetype2 -I ../../JuceLibraryCode -I ../../Source -I ../../../../../../include/mios32 -I ../../../../../../modules/random -I ../../../../../../modules/notestack -I ../../../../../../modules/aout -I ../../../../../../modules/sid -I ../../../../../../modules/app_lcd/juce
  TARGET := MIDIboxSID
  BLDCMD = $(CXX) -o $(OUTDIR)/$(TARGET) $(OBJECTS) $(LDFLAGS) $(RESOURCES) $(TARGET_ARCH)
endif

OBJECTS := \
  $(OBJDIR)/mios32_lcd_cdb61276.o \
  $(OBJDIR)/ControlGroup_24ed43c8.o \
  $(OBJDIR)/Knob_f533d61c.o \
  $(OBJDIR)/KnobBlue_ee96a216.o \
  $(OBJDIR)/KnobNegPos_7383bda0.o \
  $(OBJDIR)/KnobOrange_a48ea82a.o \
  $(OBJDIR)/KnobSmall_11f0e115.o \
  $(OBJDIR)/ControlGroupKnobs_97d36c8.o \
  $(OBJDIR)/MidiProcessing_51ccda9d.o \
  $(OBJDIR)/mios32_wrapper_code_a6e181fa.o \
  $(OBJDIR)/envelope_1a154f4a.o \
  $(OBJDIR)/extfilt_54aa28d6.o \
  $(OBJDIR)/filter_1a93a5a2.o \
  $(OBJDIR)/pot_87ea1b0d.o \
  $(OBJDIR)/resid_f61de0e7.o \
  $(OBJDIR)/version_6c8ca3aa.o \
  $(OBJDIR)/voice_953c5610.o \
  $(OBJDIR)/wave_cff57581.o \
  $(OBJDIR)/wave6581__ST_cff7fa8.o \
  $(OBJDIR)/wave6581_P_T_f40fe863.o \
  $(OBJDIR)/wave6581_PS__f36bce6c.o \
  $(OBJDIR)/wave6581_PST_f366ce57.o \
  $(OBJDIR)/wave8580__ST_cee7b84b.o \
  $(OBJDIR)/wave8580_P_T_b5f82106.o \
  $(OBJDIR)/wave8580_PS__b554070f.o \
  $(OBJDIR)/wave8580_PST_b54f06fa.o \
  $(OBJDIR)/app_18362435.o \
  $(OBJDIR)/MbSidArp_c9ca98b3.o \
  $(OBJDIR)/MbSidClock_60a63f22.o \
  $(OBJDIR)/MbSidDrum_b313c01c.o \
  $(OBJDIR)/MbSidEnv_96e4b041.o \
  $(OBJDIR)/MbSidEnvLead_b4a4ffbd.o \
  $(OBJDIR)/MbSidFilter_193ff6ae.o \
  $(OBJDIR)/MbSidLfo_fb26e589.o \
  $(OBJDIR)/MbSidMidiVoice_27d3ca5.o \
  $(OBJDIR)/MbSidMod_3ecdbbd6.o \
  $(OBJDIR)/MbSidRandomGen_21401ba1.o \
  $(OBJDIR)/MbSidSeq_6bd9dab3.o \
  $(OBJDIR)/MbSidSeqBassline_c5c29ea6.o \
  $(OBJDIR)/MbSidSeqDrum_f082fc79.o \
  $(OBJDIR)/MbSidVoice_5ba41246.o \
  $(OBJDIR)/MbSidVoiceDrum_fa71328c.o \
  $(OBJDIR)/MbSidVoiceQueue_d5041e35.o \
  $(OBJDIR)/MbSidWt_e1b96813.o \
  $(OBJDIR)/MbSidWtDrum_271cd9d9.o \
  $(OBJDIR)/MbSid_61df52d.o \
  $(OBJDIR)/MbSidAsid_262b039a.o \
  $(OBJDIR)/MbSidEnvironment_46b0cdb0.o \
  $(OBJDIR)/MbSidPatch_48ab8765.o \
  $(OBJDIR)/MbSidSe_3596b89f.o \
  $(OBJDIR)/MbSidSeBassline_d668d092.o \
  $(OBJDIR)/MbSidSeDrum_b3870465.o \
  $(OBJDIR)/MbSidSeLead_203fa91b.o \
  $(OBJDIR)/MbSidSeMulti_5e65efa4.o \
  $(OBJDIR)/MbSidSysEx_5b5ee37d.o \
  $(OBJDIR)/MbSidTables_2ac39b32.o \
  $(OBJDIR)/jsw_rand_294b894f.o \
  $(OBJDIR)/notestack_20d5562a.o \
  $(OBJDIR)/tasks_565cd9cf.o \
  $(OBJDIR)/aout_6cc4739c.o \
  $(OBJDIR)/sid_ec3c95a.o \
  $(OBJDIR)/app_lcd_f8140f8a.o \
  $(OBJDIR)/CLCDView_f4ff929e.o \
  $(OBJDIR)/PluginProcessor_a059e380.o \
  $(OBJDIR)/PluginEditor_94d4fb09.o \
  $(OBJDIR)/BinaryData_ce4232d4.o \
  $(OBJDIR)/juce_audio_basics_2442e4ea.o \
  $(OBJDIR)/juce_audio_devices_a4c8a728.o \
  $(OBJDIR)/juce_audio_formats_d349f0c8.o \
  $(OBJDIR)/juce_audio_processors_44a134a2.o \
  $(OBJDIR)/juce_audio_utils_f63b12e8.o \
  $(OBJDIR)/juce_core_aff681cc.o \
  $(OBJDIR)/juce_cryptography_25c7e826.o \
  $(OBJDIR)/juce_data_structures_bdd6d488.o \
  $(OBJDIR)/juce_events_79b2840.o \
  $(OBJDIR)/juce_graphics_c8f1e7a4.o \
  $(OBJDIR)/juce_gui_basics_a630dd20.o \
  $(OBJDIR)/juce_gui_extra_7767d6a8.o \
  $(OBJDIR)/juce_opengl_c7e3506c.o \
  $(OBJDIR)/juce_PluginUtilities_e2e19a34.o \
  $(OBJDIR)/juce_video_184321c8.o \
  $(OBJDIR)/juce_VST3_Wrapper_77e7c73b.o \
  $(OBJDIR)/juce_VST_Wrapper_bb62e93d.o \

.PHONY: clean

$(OUTDIR)/$(TARGET): $(OBJECTS) $(LDDEPS) $(RESOURCES)
	@echo Linking MIDIboxSID
	-@mkdir -p $(BINDIR)
	-@mkdir -p $(LIBDIR)
	-@mkdir -p $(OUTDIR)
	@$(BLDCMD)

clean:
	@echo Cleaning MIDIboxSID
	-@rm -f $(OUTDIR)/$(TARGET)
	-@rm -rf $(OBJDIR)/*
	-@rm -rf $(OBJDIR)

strip:
	@echo Stripping MIDIboxSID
	-@strip --strip-unneeded $(OUTDIR)/$(TARGET)

$(OBJDIR)/mios32_lcd_cdb61276.o: ../../../../../../mios32/common/mios32_lcd.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling mios32_lcd.c"
	@$(CC) $(CFLAGS) -o "$@" -c "$<"

$(OBJDIR)/ControlGroup_24ed43c8.o: ../../Source/gui/components/ControlGroup.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling ControlGroup.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/Knob_f533d61c.o: ../../Source/gui/components/Knob.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling Knob.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/KnobBlue_ee96a216.o: ../../Source/gui/components/KnobBlue.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling KnobBlue.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/KnobNegPos_7383bda0.o: ../../Source/gui/components/KnobNegPos.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling KnobNegPos.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/KnobOrange_a48ea82a.o: ../../Source/gui/components/KnobOrange.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling KnobOrange.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/KnobSmall_11f0e115.o: ../../Source/gui/components/KnobSmall.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling KnobSmall.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/ControlGroupKnobs_97d36c8.o: ../../Source/gui/ControlGroupKnobs.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling ControlGroupKnobs.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MidiProcessing_51ccda9d.o: ../../Source/MidiProcessing.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MidiProcessing.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/mios32_wrapper_code_a6e181fa.o: ../../Source/mios32_wrapper_code.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling mios32_wrapper_code.c"
	@$(CC) $(CFLAGS) -o "$@" -c "$<"

$(OBJDIR)/envelope_1a154f4a.o: ../../resid/envelope.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling envelope.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/extfilt_54aa28d6.o: ../../resid/extfilt.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling extfilt.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/filter_1a93a5a2.o: ../../resid/filter.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling filter.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/pot_87ea1b0d.o: ../../resid/pot.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling pot.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/resid_f61de0e7.o: ../../resid/resid.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling resid.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/version_6c8ca3aa.o: ../../resid/version.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling version.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/voice_953c5610.o: ../../resid/voice.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling voice.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/wave_cff57581.o: ../../resid/wave.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling wave.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/wave6581__ST_cff7fa8.o: ../../resid/wave6581__ST.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling wave6581__ST.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/wave6581_P_T_f40fe863.o: ../../resid/wave6581_P_T.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling wave6581_P_T.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/wave6581_PS__f36bce6c.o: ../../resid/wave6581_PS_.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling wave6581_PS_.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/wave6581_PST_f366ce57.o: ../../resid/wave6581_PST.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling wave6581_PST.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/wave8580__ST_cee7b84b.o: ../../resid/wave8580__ST.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling wave8580__ST.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/wave8580_P_T_b5f82106.o: ../../resid/wave8580_P_T.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling wave8580_P_T.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/wave8580_PS__b554070f.o: ../../resid/wave8580_PS_.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling wave8580_PS_.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/wave8580_PST_b54f06fa.o: ../../resid/wave8580_PST.cc
	-@mkdir -p $(OBJDIR)
	@echo "Compiling wave8580_PST.cc"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/app_18362435.o: ../../../core/app.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling app.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidArp_c9ca98b3.o: ../../../core/components/MbSidArp.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidArp.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidClock_60a63f22.o: ../../../core/components/MbSidClock.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidClock.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidDrum_b313c01c.o: ../../../core/components/MbSidDrum.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidDrum.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidEnv_96e4b041.o: ../../../core/components/MbSidEnv.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidEnv.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidEnvLead_b4a4ffbd.o: ../../../core/components/MbSidEnvLead.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidEnvLead.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidFilter_193ff6ae.o: ../../../core/components/MbSidFilter.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidFilter.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidLfo_fb26e589.o: ../../../core/components/MbSidLfo.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidLfo.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidMidiVoice_27d3ca5.o: ../../../core/components/MbSidMidiVoice.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidMidiVoice.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidMod_3ecdbbd6.o: ../../../core/components/MbSidMod.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidMod.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidRandomGen_21401ba1.o: ../../../core/components/MbSidRandomGen.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidRandomGen.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidSeq_6bd9dab3.o: ../../../core/components/MbSidSeq.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidSeq.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidSeqBassline_c5c29ea6.o: ../../../core/components/MbSidSeqBassline.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidSeqBassline.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidSeqDrum_f082fc79.o: ../../../core/components/MbSidSeqDrum.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidSeqDrum.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidVoice_5ba41246.o: ../../../core/components/MbSidVoice.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidVoice.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidVoiceDrum_fa71328c.o: ../../../core/components/MbSidVoiceDrum.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidVoiceDrum.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidVoiceQueue_d5041e35.o: ../../../core/components/MbSidVoiceQueue.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidVoiceQueue.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidWt_e1b96813.o: ../../../core/components/MbSidWt.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidWt.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidWtDrum_271cd9d9.o: ../../../core/components/MbSidWtDrum.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidWtDrum.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSid_61df52d.o: ../../../core/MbSid.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSid.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidAsid_262b039a.o: ../../../core/MbSidAsid.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidAsid.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidEnvironment_46b0cdb0.o: ../../../core/MbSidEnvironment.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidEnvironment.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidPatch_48ab8765.o: ../../../core/MbSidPatch.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidPatch.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidSe_3596b89f.o: ../../../core/MbSidSe.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidSe.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidSeBassline_d668d092.o: ../../../core/MbSidSeBassline.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidSeBassline.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidSeDrum_b3870465.o: ../../../core/MbSidSeDrum.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidSeDrum.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidSeLead_203fa91b.o: ../../../core/MbSidSeLead.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidSeLead.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidSeMulti_5e65efa4.o: ../../../core/MbSidSeMulti.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidSeMulti.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidSysEx_5b5ee37d.o: ../../../core/MbSidSysEx.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidSysEx.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbSidTables_2ac39b32.o: ../../../core/MbSidTables.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbSidTables.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/jsw_rand_294b894f.o: ../../../../../../modules/random/jsw_rand.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling jsw_rand.c"
	@$(CC) $(CFLAGS) -o "$@" -c "$<"

$(OBJDIR)/notestack_20d5562a.o: ../../../../../../modules/notestack/notestack.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling notestack.c"
	@$(CC) $(CFLAGS) -o "$@" -c "$<"

$(OBJDIR)/tasks_565cd9cf.o: ../../Source/tasks.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling tasks.c"
	@$(CC) $(CFLAGS) -o "$@" -c "$<"

$(OBJDIR)/aout_6cc4739c.o: ../../../../../../modules/aout/aout.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling aout.c"
	@$(CC) $(CFLAGS) -o "$@" -c "$<"

$(OBJDIR)/sid_ec3c95a.o: ../../../../../../modules/sid/sid.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling sid.c"
	@$(CC) $(CFLAGS) -o "$@" -c "$<"

$(OBJDIR)/app_lcd_f8140f8a.o: ../../../../../../modules/app_lcd/juce/app_lcd.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling app_lcd.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/CLCDView_f4ff929e.o: ../../../../../../modules/app_lcd/juce/CLCDView.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CLCDView.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/PluginProcessor_a059e380.o: ../../Source/PluginProcessor.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling PluginProcessor.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/PluginEditor_94d4fb09.o: ../../Source/PluginEditor.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling PluginEditor.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/BinaryData_ce4232d4.o: ../../JuceLibraryCode/BinaryData.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling BinaryData.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_audio_basics_2442e4ea.o: ../../JuceLibraryCode/modules/juce_audio_basics/juce_audio_basics.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_audio_basics.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_audio_devices_a4c8a728.o: ../../JuceLibraryCode/modules/juce_audio_devices/juce_audio_devices.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_audio_devices.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_audio_formats_d349f0c8.o: ../../JuceLibraryCode/modules/juce_audio_formats/juce_audio_formats.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_audio_formats.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_audio_processors_44a134a2.o: ../../JuceLibraryCode/modules/juce_audio_processors/juce_audio_processors.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_audio_processors.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_audio_utils_f63b12e8.o: ../../JuceLibraryCode/modules/juce_audio_utils/juce_audio_utils.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_audio_utils.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_core_aff681cc.o: ../../JuceLibraryCode/modules/juce_core/juce_core.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_core.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_cryptography_25c7e826.o: ../../JuceLibraryCode/modules/juce_cryptography/juce_cryptography.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_cryptography.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_data_structures_bdd6d488.o: ../../JuceLibraryCode/modules/juce_data_structures/juce_data_structures.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_data_structures.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_events_79b2840.o: ../../JuceLibraryCode/modules/juce_events/juce_events.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_events.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_graphics_c8f1e7a4.o: ../../JuceLibraryCode/modules/juce_graphics/juce_graphics.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_graphics.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_gui_basics_a630dd20.o: ../../JuceLibraryCode/modules/juce_gui_basics/juce_gui_basics.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_gui_basics.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_gui_extra_7767d6a8.o: ../../JuceLibraryCode/modules/juce_gui_extra/juce_gui_extra.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_gui_extra.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_opengl_c7e3506c.o: ../../JuceLibraryCode/modules/juce_opengl/juce_opengl.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_opengl.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_PluginUtilities_e2e19a34.o: ../../JuceLibraryCode/modules/juce_audio_plugin_client/utility/juce_PluginUtilities.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_PluginUtilities.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_video_184321c8.o: ../../JuceLibraryCode/modules/juce_video/juce_video.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_video.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_VST3_Wrapper_77e7c73b.o: ../../JuceLibraryCode/modules/juce_audio_plugin_client/VST3/juce_VST3_Wrapper.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_VST3_Wrapper.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_VST_Wrapper_bb62e93d.o: ../../JuceLibraryCode/modules/juce_audio_plugin_client/VST/juce_VST_Wrapper.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_VST_Wrapper.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

-include $(OBJECTS:%.o=%.d)
