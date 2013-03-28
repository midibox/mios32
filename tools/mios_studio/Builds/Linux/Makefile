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
  CPPFLAGS := $(DEPFLAGS) -D "LINUX=1" -D "DEBUG=1" -D "_DEBUG=1" -D "JUCER_LINUX_MAKE_7346DA2A=1" -I /usr/include -I /usr/include/freetype2 -I ../../JuceLibraryCode
  CFLAGS += $(CPPFLAGS) $(TARGET_ARCH) -g -ggdb -O0
  CXXFLAGS += $(CFLAGS) 
  LDFLAGS += -L$(BINDIR) -L$(LIBDIR) -L/usr/X11R6/lib/ -lX11 -lXext -lXinerama -lasound -ldl -lfreetype -lpthread -lrt 
  LDDEPS :=
  RESFLAGS :=  -D "LINUX=1" -D "DEBUG=1" -D "_DEBUG=1" -D "JUCER_LINUX_MAKE_7346DA2A=1" -I /usr/include -I /usr/include/freetype2 -I ../../JuceLibraryCode
  TARGET := MIOS\ Studio
  BLDCMD = $(CXX) -o $(OUTDIR)/$(TARGET) $(OBJECTS) $(LDFLAGS) $(RESOURCES) $(TARGET_ARCH)
endif

ifeq ($(CONFIG),Release)
  BINDIR := build
  LIBDIR := build
  OBJDIR := build/intermediate/Release
  OUTDIR := build
  CPPFLAGS := $(DEPFLAGS) -D "LINUX=1" -D "NDEBUG=1" -D "JUCER_LINUX_MAKE_7346DA2A=1" -I /usr/include -I /usr/include/freetype2 -I ../../JuceLibraryCode
  CFLAGS += $(CPPFLAGS) $(TARGET_ARCH) -Os
  CXXFLAGS += $(CFLAGS) 
  LDFLAGS += -L$(BINDIR) -L$(LIBDIR) -L/usr/X11R6/lib/ -lX11 -lXext -lXinerama -lasound -ldl -lfreetype -lpthread -lrt 
  LDDEPS :=
  RESFLAGS :=  -D "LINUX=1" -D "NDEBUG=1" -D "JUCER_LINUX_MAKE_7346DA2A=1" -I /usr/include -I /usr/include/freetype2 -I ../../JuceLibraryCode
  TARGET := MIOS\ Studio
  BLDCMD = $(CXX) -o $(OUTDIR)/$(TARGET) $(OBJECTS) $(LDFLAGS) $(RESOURCES) $(TARGET_ARCH)
endif

OBJECTS := \
  $(OBJDIR)/ConfigTableComponents_f9ec0b18.o \
  $(OBJDIR)/HexTextEditor_ee5f672b.o \
  $(OBJDIR)/LogBox_4af6f1bb.o \
  $(OBJDIR)/MbCvTool_431c8434.o \
  $(OBJDIR)/MbhpMfTool_c231a562.o \
  $(OBJDIR)/MidiKeyboard_b542143c.o \
  $(OBJDIR)/MidiMonitor_8b0c5ef.o \
  $(OBJDIR)/Midio128Tool_635a24d5.o \
  $(OBJDIR)/MidiSlider_78ead436.o \
  $(OBJDIR)/MiosFileBrowser_87590e62.o \
  $(OBJDIR)/MiosStudio_25d6994a.o \
  $(OBJDIR)/MiosTerminal_d4edcb30.o \
  $(OBJDIR)/OscMonitor_7d14ddaf.o \
  $(OBJDIR)/OscTextEditor_19fee40f.o \
  $(OBJDIR)/OscTool_947459ad.o \
  $(OBJDIR)/SysexLibrarian_e2b740c.o \
  $(OBJDIR)/SysexTool_7ccd9ae.o \
  $(OBJDIR)/UploadWindow_c00d9ee5.o \
  $(OBJDIR)/HexFileLoader_8ddbb2b4.o \
  $(OBJDIR)/Main_36be1979.o \
  $(OBJDIR)/OscHelper_e41a9d17.o \
  $(OBJDIR)/SysexHelper_e1785bd8.o \
  $(OBJDIR)/SysexPatchDb_d23f64c6.o \
  $(OBJDIR)/UdpSocket_31e7cc9e.o \
  $(OBJDIR)/UploadHandler_13066b13.o \
  $(OBJDIR)/juce_audio_basics_5460bccb.o \
  $(OBJDIR)/juce_audio_devices_44d00ac9.o \
  $(OBJDIR)/juce_audio_formats_73515469.o \
  $(OBJDIR)/juce_audio_processors_d11d9b83.o \
  $(OBJDIR)/juce_audio_utils_5ff6f09.o \
  $(OBJDIR)/juce_core_5f5a3bad.o \
  $(OBJDIR)/juce_data_structures_7e517fa9.o \
  $(OBJDIR)/juce_events_88dd09a1.o \
  $(OBJDIR)/juce_graphics_29b23085.o \
  $(OBJDIR)/juce_gui_basics_a9efcd81.o \
  $(OBJDIR)/juce_gui_extra_a9396b49.o \

.PHONY: clean

$(OUTDIR)/$(TARGET): $(OBJECTS) $(LDDEPS) $(RESOURCES)
	@echo Linking MIOS Studio
	-@mkdir -p $(BINDIR)
	-@mkdir -p $(LIBDIR)
	-@mkdir -p $(OUTDIR)
	@$(BLDCMD)

clean:
	@echo Cleaning MIOS Studio
	-@rm -f $(OUTDIR)/$(TARGET)
	-@rm -rf $(OBJDIR)/*
	-@rm -rf $(OBJDIR)

strip:
	@echo Stripping MIOS Studio
	-@strip --strip-unneeded $(OUTDIR)/$(TARGET)

$(OBJDIR)/ConfigTableComponents_f9ec0b18.o: ../../src/gui/ConfigTableComponents.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling ConfigTableComponents.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/HexTextEditor_ee5f672b.o: ../../src/gui/HexTextEditor.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling HexTextEditor.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/LogBox_4af6f1bb.o: ../../src/gui/LogBox.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling LogBox.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbCvTool_431c8434.o: ../../src/gui/MbCvTool.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbCvTool.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MbhpMfTool_c231a562.o: ../../src/gui/MbhpMfTool.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MbhpMfTool.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MidiKeyboard_b542143c.o: ../../src/gui/MidiKeyboard.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MidiKeyboard.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MidiMonitor_8b0c5ef.o: ../../src/gui/MidiMonitor.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MidiMonitor.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/Midio128Tool_635a24d5.o: ../../src/gui/Midio128Tool.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling Midio128Tool.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MidiSlider_78ead436.o: ../../src/gui/MidiSlider.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MidiSlider.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MiosFileBrowser_87590e62.o: ../../src/gui/MiosFileBrowser.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MiosFileBrowser.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MiosStudio_25d6994a.o: ../../src/gui/MiosStudio.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MiosStudio.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/MiosTerminal_d4edcb30.o: ../../src/gui/MiosTerminal.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling MiosTerminal.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/OscMonitor_7d14ddaf.o: ../../src/gui/OscMonitor.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling OscMonitor.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/OscTextEditor_19fee40f.o: ../../src/gui/OscTextEditor.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling OscTextEditor.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/OscTool_947459ad.o: ../../src/gui/OscTool.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling OscTool.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/SysexLibrarian_e2b740c.o: ../../src/gui/SysexLibrarian.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling SysexLibrarian.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/SysexTool_7ccd9ae.o: ../../src/gui/SysexTool.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling SysexTool.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/UploadWindow_c00d9ee5.o: ../../src/gui/UploadWindow.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling UploadWindow.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/HexFileLoader_8ddbb2b4.o: ../../src/HexFileLoader.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling HexFileLoader.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/Main_36be1979.o: ../../src/Main.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling Main.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/OscHelper_e41a9d17.o: ../../src/OscHelper.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling OscHelper.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/SysexHelper_e1785bd8.o: ../../src/SysexHelper.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling SysexHelper.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/SysexPatchDb_d23f64c6.o: ../../src/SysexPatchDb.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling SysexPatchDb.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/UdpSocket_31e7cc9e.o: ../../src/UdpSocket.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling UdpSocket.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/UploadHandler_13066b13.o: ../../src/UploadHandler.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling UploadHandler.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_audio_basics_5460bccb.o: ../../../juce/modules/juce_audio_basics/juce_audio_basics.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_audio_basics.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_audio_devices_44d00ac9.o: ../../../juce/modules/juce_audio_devices/juce_audio_devices.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_audio_devices.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_audio_formats_73515469.o: ../../../juce/modules/juce_audio_formats/juce_audio_formats.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_audio_formats.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_audio_processors_d11d9b83.o: ../../../juce/modules/juce_audio_processors/juce_audio_processors.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_audio_processors.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_audio_utils_5ff6f09.o: ../../../juce/modules/juce_audio_utils/juce_audio_utils.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_audio_utils.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_core_5f5a3bad.o: ../../../juce/modules/juce_core/juce_core.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_core.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_data_structures_7e517fa9.o: ../../../juce/modules/juce_data_structures/juce_data_structures.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_data_structures.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_events_88dd09a1.o: ../../../juce/modules/juce_events/juce_events.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_events.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_graphics_29b23085.o: ../../../juce/modules/juce_graphics/juce_graphics.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_graphics.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_gui_basics_a9efcd81.o: ../../../juce/modules/juce_gui_basics/juce_gui_basics.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_gui_basics.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

$(OBJDIR)/juce_gui_extra_a9396b49.o: ../../../juce/modules/juce_gui_extra/juce_gui_extra.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling juce_gui_extra.cpp"
	@$(CXX) $(CXXFLAGS) -o "$@" -c "$<"

-include $(OBJECTS:%.o=%.d)
