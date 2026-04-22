# Automatically generated makefile, created by the Projucer
# Don't edit this file! Your changes will be overwritten when you re-save the Projucer project!

# build with "V=1" for verbose builds
ifeq ($(V), 1)
V_AT =
else
V_AT = @
endif

# (this disables dependency generation if multiple architectures are set)
DEPFLAGS := $(if $(word 2, $(TARGET_ARCH)), , -MMD)

ifndef PKG_CONFIG
  PKG_CONFIG=pkg-config
endif

ifndef STRIP
  STRIP=strip
endif

ifndef AR
  AR=ar
endif

ifndef CONFIG
  CONFIG=Debug
endif

JUCE_ARCH_LABEL := $(shell uname -m)

ifeq ($(CONFIG),Debug)
  JUCE_BINDIR := build
  JUCE_LIBDIR := build
  JUCE_OBJDIR := build/intermediate/Debug
  JUCE_OUTDIR := build

  ifeq ($(TARGET_ARCH),)
    TARGET_ARCH := -m64
  endif

  JUCE_CPPFLAGS := $(DEPFLAGS) "-DLINUX=1" "-DDEBUG=1" "-D_DEBUG=1" "-DVST_LOGGING=0" "-DLINK_PLATFORM_LINUX=1" "-DGDK_BACKEND=x11" "-DJUCER_LINUX_MAKE_6D53C8B4=1" "-DJUCE_APP_VERSION=1.0.0b4" "-DJUCE_APP_VERSION_HEX=0x10000" $(shell $(PKG_CONFIG) --cflags alsa freetype2 gl libcurl webkit2gtk-4.0 gtk+-x11-3.0) -pthread -I../../modules/juce/modules/juce_audio_processors/format_types/VST3_SDK -I../../JuceLibraryCode -I../../modules/juce/modules -I../../modules -I../../src -I../../external/asio -I../../external/asiodriver -I../../external/servus/include $(CPPFLAGS)
  JUCE_CPPFLAGS_APP :=  "-DJucePlugin_Build_VST=0" "-DJucePlugin_Build_VST3=0" "-DJucePlugin_Build_AU=0" "-DJucePlugin_Build_AUv3=0" "-DJucePlugin_Build_AAX=0" "-DJucePlugin_Build_Standalone=0" "-DJucePlugin_Build_Unity=0" "-DJucePlugin_Build_LV2=0"
  JUCE_TARGET_APP := SnoringPony

  JUCE_CFLAGS += $(JUCE_CPPFLAGS) $(TARGET_ARCH) -g -ggdb -O3 -Wno-multichar $(CFLAGS)
  JUCE_CXXFLAGS += $(JUCE_CFLAGS) -std=c++17 $(CXXFLAGS)
  JUCE_LDFLAGS += $(TARGET_ARCH) -L$(JUCE_BINDIR) -L$(JUCE_LIBDIR) -L../../external/servus/lib/linux -L/usr/lib/x86_64-linux-gnu/ -L../../external/wapi/linux -L../../modules/juce_simpleweb/libs/Linux/x86_64 $(shell $(PKG_CONFIG) --libs alsa freetype2 gl libcurl) -fvisibility=hidden -Wl,-rpath,"lib" -Wl,--as-needed -Wl,--export-dynamic -fvisibility=default -lrt -ldl -lpthread -lssl -lcrypto -lServus -lcurl -lwapi $(LDFLAGS)

  CLEANCMD = rm -rf $(JUCE_OUTDIR)/$(JUCE_TARGET_APP) $(JUCE_OBJDIR)
endif

ifeq ($(CONFIG),Release)
  JUCE_BINDIR := build
  JUCE_LIBDIR := build
  JUCE_OBJDIR := build/intermediate/Release
  JUCE_OUTDIR := build

  ifeq ($(TARGET_ARCH),)
    TARGET_ARCH := -m64
  endif

  JUCE_CPPFLAGS := $(DEPFLAGS) "-DLINUX=1" "-DNDEBUG=1" "-DVST_LOGGING=0" "-DLINK_PLATFORM_LINUX=1" "-DGDK_BACKEND=x11" "-DJUCER_LINUX_MAKE_6D53C8B4=1" "-DJUCE_APP_VERSION=1.0.0b4" "-DJUCE_APP_VERSION_HEX=0x10000" $(shell $(PKG_CONFIG) --cflags alsa freetype2 gl libcurl webkit2gtk-4.0 gtk+-x11-3.0) -pthread -I../../modules/juce/modules/juce_audio_processors/format_types/VST3_SDK -I../../JuceLibraryCode -I../../modules/juce/modules -I../../modules -I../../src -I../../external/asio -I../../external/asiodriver -I../../external/servus/include $(CPPFLAGS)
  JUCE_CPPFLAGS_APP :=  "-DJucePlugin_Build_VST=0" "-DJucePlugin_Build_VST3=0" "-DJucePlugin_Build_AU=0" "-DJucePlugin_Build_AUv3=0" "-DJucePlugin_Build_AAX=0" "-DJucePlugin_Build_Standalone=0" "-DJucePlugin_Build_Unity=0" "-DJucePlugin_Build_LV2=0"
  JUCE_TARGET_APP := SnoringPony

  JUCE_CFLAGS += $(JUCE_CPPFLAGS) $(TARGET_ARCH) -O3 -Wno-multichar $(CFLAGS)
  JUCE_CXXFLAGS += $(JUCE_CFLAGS) -std=c++17 $(CXXFLAGS)
  JUCE_LDFLAGS += $(TARGET_ARCH) -L$(JUCE_BINDIR) -L$(JUCE_LIBDIR) -L../../external/servus/lib/linux -L/usr/lib/x86_64-linux-gnu/ -L../../external/wapi/linux -L../../modules/juce_simpleweb/libs/Linux/x86_64 $(shell $(PKG_CONFIG) --libs alsa freetype2 gl libcurl) -fvisibility=hidden -Wl,-rpath,"lib" -Wl,--as-needed -Wl,--export-dynamic -fvisibility=default -lrt -ldl -lpthread -lssl -lcrypto -lServus -lcurl -lwapi $(LDFLAGS)

  CLEANCMD = rm -rf $(JUCE_OUTDIR)/$(JUCE_TARGET_APP) $(JUCE_OBJDIR)
endif

OBJECTS_APP := \
  $(JUCE_OBJDIR)/PluginSlotEditor_32f16d2a.o \
  $(JUCE_OBJDIR)/PluginScanner_94abfd7c.o \
  $(JUCE_OBJDIR)/PluginSlot_8351168a.o \
  $(JUCE_OBJDIR)/AudioPlayer_a840e188.o \
  $(JUCE_OBJDIR)/FadeCue_b47c1b80.o \
  $(JUCE_OBJDIR)/OSCCueMessageEditor_79a451.o \
  $(JUCE_OBJDIR)/OSCCue_6cd7e3c0.o \
  $(JUCE_OBJDIR)/OSCCueMessage_fc4f0bb1.o \
  $(JUCE_OBJDIR)/PlaylistCue_3b5dd40.o \
  $(JUCE_OBJDIR)/OBSCueRequestEditor_9d0649b9.o \
  $(JUCE_OBJDIR)/OBSCue_b6c639fe.o \
  $(JUCE_OBJDIR)/OBSCueRequest_90d4423b.o \
  $(JUCE_OBJDIR)/MTCContainerEditor_10bd4c82.o \
  $(JUCE_OBJDIR)/AudioWaveformSlicerEditor_302e651a.o \
  $(JUCE_OBJDIR)/AudioSlices_6406366c.o \
  $(JUCE_OBJDIR)/AudioWaveformSlicer_8710d808.o \
  $(JUCE_OBJDIR)/AudioCue_6b8f1412.o \
  $(JUCE_OBJDIR)/AudioFile_d20cdec7.o \
  $(JUCE_OBJDIR)/CueEditor_6ade7941.o \
  $(JUCE_OBJDIR)/DCAAssignmentDialog_62e17685.o \
  $(JUCE_OBJDIR)/DCAAssignmentEditor_58ab002a.o \
  $(JUCE_OBJDIR)/CharacterRef_390d4313.o \
  $(JUCE_OBJDIR)/DCAAssignment_a075f530.o \
  $(JUCE_OBJDIR)/DCACue_2d8b60fa.o \
  $(JUCE_OBJDIR)/GoCue_f384cdc0.o \
  $(JUCE_OBJDIR)/Cue_4c4af201.o \
  $(JUCE_OBJDIR)/CueManager_4c7e6ef6.o \
  $(JUCE_OBJDIR)/DCAMixingCuelistEditor_2970c9d0.o \
  $(JUCE_OBJDIR)/DCAMixingCuelist_50adf7d4.o \
  $(JUCE_OBJDIR)/CuelistEditor_b29c701.o \
  $(JUCE_OBJDIR)/CuelistManagerItemUI_efa5716a.o \
  $(JUCE_OBJDIR)/CuelistManagerUI_4123bb77.o \
  $(JUCE_OBJDIR)/Cuelist_15802d3d.o \
  $(JUCE_OBJDIR)/CuelistFactory_ee0f8317.o \
  $(JUCE_OBJDIR)/CuelistManager_ebc7583a.o \
  $(JUCE_OBJDIR)/CuesTableModel_7cc12950.o \
  $(JUCE_OBJDIR)/CuesTableUI_f48ef477.o \
  $(JUCE_OBJDIR)/DeckUI_fc73c092.o \
  $(JUCE_OBJDIR)/DeckViewHeaderUI_ff918184.o \
  $(JUCE_OBJDIR)/DeckViewUI_e4afbb17.o \
  $(JUCE_OBJDIR)/ReorderCuesWindow_464109e0.o \
  $(JUCE_OBJDIR)/GoAction_75b04dcb.o \
  $(JUCE_OBJDIR)/MappingAction_5677e781.o \
  $(JUCE_OBJDIR)/MappingActionFactory_2ba27453.o \
  $(JUCE_OBJDIR)/PanicAction_2c2e03ea.o \
  $(JUCE_OBJDIR)/SelectNextCueAction_16dbc2d7.o \
  $(JUCE_OBJDIR)/SelectPreviousCueAction_ba949353.o \
  $(JUCE_OBJDIR)/AudioInterfaceHardwareEditor_d10a2fce.o \
  $(JUCE_OBJDIR)/AudioOutputEditor_dae63958.o \
  $(JUCE_OBJDIR)/AudioInterface_a266d812.o \
  $(JUCE_OBJDIR)/AudioOutput_1c8db912.o \
  $(JUCE_OBJDIR)/MIDIInterface_77dce880.o \
  $(JUCE_OBJDIR)/MIDIDevice_de7e8239.o \
  $(JUCE_OBJDIR)/MIDIManager_52484d74.o \
  $(JUCE_OBJDIR)/MIDIMapping_98fde15.o \
  $(JUCE_OBJDIR)/MIDIFeedbackFactory_38648d8c.o \
  $(JUCE_OBJDIR)/MIDIFeedbackItem_e100d89b.o \
  $(JUCE_OBJDIR)/MIDIIsPanickingFeedback_c4b7d4f0.o \
  $(JUCE_OBJDIR)/MIDIIsPlayingFeedback_ae3f4490.o \
  $(JUCE_OBJDIR)/AHCQMixerSettings_c6532099.o \
  $(JUCE_OBJDIR)/AHCQProtocol_b5d245b5.o \
  $(JUCE_OBJDIR)/WingOscMixerSettings_bbefb8f.o \
  $(JUCE_OBJDIR)/WingOscProtocol_319ae8ff.o \
  $(JUCE_OBJDIR)/wapi_mingw_shim_56476567.o \
  $(JUCE_OBJDIR)/WingTcpMixerSettings_e9b7dc4b.o \
  $(JUCE_OBJDIR)/Character_4db51f37.o \
  $(JUCE_OBJDIR)/MixerChannel_ff00e9f6.o \
  $(JUCE_OBJDIR)/MixerFX_b07eb3e9.o \
  $(JUCE_OBJDIR)/MixerInterface_4b8bd7ac.o \
  $(JUCE_OBJDIR)/OBSCommandEditor_5d7a2d58.o \
  $(JUCE_OBJDIR)/OBSCommand_ff2f2710.o \
  $(JUCE_OBJDIR)/OBSInterface_4495a9fe.o \
  $(JUCE_OBJDIR)/OSCCommandArgumentsEditor_32cb47c2.o \
  $(JUCE_OBJDIR)/OSCCommandEditor_99163e38.o \
  $(JUCE_OBJDIR)/OSCInputEditor_1e7922b7.o \
  $(JUCE_OBJDIR)/OSCCommand_9cf21fd2.o \
  $(JUCE_OBJDIR)/OSCInterface_7d7d7a40.o \
  $(JUCE_OBJDIR)/OSCCurrentCueFeedback_52210c42.o \
  $(JUCE_OBJDIR)/OSCFeedbackFactory_9a40cf4c.o \
  $(JUCE_OBJDIR)/OSCFeedbackItem_b9a1eedb.o \
  $(JUCE_OBJDIR)/OSCIsPanickingFeedback_771d36b0.o \
  $(JUCE_OBJDIR)/OSCIsPlayingFeedback_a121650.o \
  $(JUCE_OBJDIR)/OSCMapping_ada9dfd5.o \
  $(JUCE_OBJDIR)/OSCNextCueFeedback_c1b9b76c.o \
  $(JUCE_OBJDIR)/InterfaceManagerUI_f9a324b7.o \
  $(JUCE_OBJDIR)/InterfaceUI_e393dec8.o \
  $(JUCE_OBJDIR)/FeedbackItem_3ca1f296.o \
  $(JUCE_OBJDIR)/Interface_9e38c80d.o \
  $(JUCE_OBJDIR)/InterfaceManager_8001f36a.o \
  $(JUCE_OBJDIR)/AudioSettings_1417810.o \
  $(JUCE_OBJDIR)/ColorPresets_884d5ac4.o \
  $(JUCE_OBJDIR)/DecksSettings_fe0d7724.o \
  $(JUCE_OBJDIR)/ShowProperties_a8318b43.o \
  $(JUCE_OBJDIR)/VolumePresets_e2087271.o \
  $(JUCE_OBJDIR)/Clock_bf28f053.o \
  $(JUCE_OBJDIR)/ShowControl_5e188ae5.o \
  $(JUCE_OBJDIR)/ShowInfos_e95c3bcd.o \
  $(JUCE_OBJDIR)/LookAndFeelTable_17cb8c95.o \
  $(JUCE_OBJDIR)/AboutWindow_db489c22.o \
  $(JUCE_OBJDIR)/SPAssetManager_e9ba871f.o \
  $(JUCE_OBJDIR)/Brain_7bbbce80.o \
  $(JUCE_OBJDIR)/Main_36be1979.o \
  $(JUCE_OBJDIR)/MainComponent_dd9a9ece.o \
  $(JUCE_OBJDIR)/MainComponentCommands_fa1e5076.o \
  $(JUCE_OBJDIR)/MainIncludes_af4e4ce4.o \
  $(JUCE_OBJDIR)/MainMenuBarComponent_c1978242.o \
  $(JUCE_OBJDIR)/PonyEngine_63977cec.o \
  $(JUCE_OBJDIR)/UserInputManager_a645b8ae.o \
  $(JUCE_OBJDIR)/BinaryData_ce4232d4.o \
  $(JUCE_OBJDIR)/include_juce_audio_basics_8a4e984a.o \
  $(JUCE_OBJDIR)/include_juce_audio_devices_63111d02.o \
  $(JUCE_OBJDIR)/include_juce_audio_formats_15f82001.o \
  $(JUCE_OBJDIR)/include_juce_audio_processors_10c03666.o \
  $(JUCE_OBJDIR)/include_juce_audio_processors_ara_2a4c6ef7.o \
  $(JUCE_OBJDIR)/include_juce_audio_processors_lv2_libs_12bdca08.o \
  $(JUCE_OBJDIR)/include_juce_audio_utils_9f9fb2d6.o \
  $(JUCE_OBJDIR)/include_juce_core_f26d17db.o \
  $(JUCE_OBJDIR)/include_juce_cryptography_8cb807a8.o \
  $(JUCE_OBJDIR)/include_juce_data_structures_7471b1e3.o \
  $(JUCE_OBJDIR)/include_juce_dsp_aeb2060f.o \
  $(JUCE_OBJDIR)/include_juce_events_fd7d695.o \
  $(JUCE_OBJDIR)/include_juce_graphics_f817e147.o \
  $(JUCE_OBJDIR)/include_juce_gui_basics_e3f79785.o \
  $(JUCE_OBJDIR)/include_juce_gui_extra_6dee1c1a.o \
  $(JUCE_OBJDIR)/include_juce_opengl_a8a032b.o \
  $(JUCE_OBJDIR)/include_juce_organicui_2c36794d.o \
  $(JUCE_OBJDIR)/include_juce_organicui2_5ab8c64f.o \
  $(JUCE_OBJDIR)/include_juce_organicui2b_ff2481bd.o \
  $(JUCE_OBJDIR)/include_juce_organicui3_5ac6ddd0.o \
  $(JUCE_OBJDIR)/include_juce_organicui4_5ad4f551.o \
  $(JUCE_OBJDIR)/include_juce_osc_f3df604d.o \
  $(JUCE_OBJDIR)/include_juce_simpleweb_acec7570.o \
  $(JUCE_OBJDIR)/include_juce_simpleweb2_f0c24e8c.o \

.PHONY: clean all strip

all : $(JUCE_OUTDIR)/$(JUCE_TARGET_APP)

$(JUCE_OUTDIR)/$(JUCE_TARGET_APP) : $(OBJECTS_APP) $(JUCE_OBJDIR)/execinfo.cmd $(RESOURCES)
	@command -v $(PKG_CONFIG) >/dev/null 2>&1 || { echo >&2 "pkg-config not installed. Please, install it."; exit 1; }
	@$(PKG_CONFIG) --print-errors alsa freetype2 gl libcurl
	@echo Linking "SnoringPony - App"
	-$(V_AT)mkdir -p $(JUCE_BINDIR)
	-$(V_AT)mkdir -p $(JUCE_LIBDIR)
	-$(V_AT)mkdir -p $(JUCE_OUTDIR)
	$(V_AT)$(CXX) -o $(JUCE_OUTDIR)/$(JUCE_TARGET_APP) $(OBJECTS_APP) $(JUCE_LDFLAGS) $(shell cat $(JUCE_OBJDIR)/execinfo.cmd) $(JUCE_LDFLAGS_APP) $(RESOURCES) $(TARGET_ARCH)

$(JUCE_OBJDIR)/PluginSlotEditor_32f16d2a.o: ../../src/Audio/ui/PluginSlotEditor.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling PluginSlotEditor.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/PluginScanner_94abfd7c.o: ../../src/Audio/PluginScanner.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling PluginScanner.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/PluginSlot_8351168a.o: ../../src/Audio/PluginSlot.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling PluginSlot.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/AudioPlayer_a840e188.o: ../../src/Audio/AudioPlayer.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling AudioPlayer.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/FadeCue_b47c1b80.o: ../../src/Cue/fade/FadeCue.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling FadeCue.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OSCCueMessageEditor_79a451.o: ../../src/Cue/osc/ui/OSCCueMessageEditor.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OSCCueMessageEditor.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OSCCue_6cd7e3c0.o: ../../src/Cue/osc/OSCCue.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OSCCue.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OSCCueMessage_fc4f0bb1.o: ../../src/Cue/osc/OSCCueMessage.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OSCCueMessage.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/PlaylistCue_3b5dd40.o: ../../src/Cue/playlist/PlaylistCue.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling PlaylistCue.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OBSCueRequestEditor_9d0649b9.o: ../../src/Cue/obs/ui/OBSCueRequestEditor.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OBSCueRequestEditor.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OBSCue_b6c639fe.o: ../../src/Cue/obs/OBSCue.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OBSCue.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OBSCueRequest_90d4423b.o: ../../src/Cue/obs/OBSCueRequest.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OBSCueRequest.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MTCContainerEditor_10bd4c82.o: ../../src/Cue/audio/ui/MTCContainerEditor.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MTCContainerEditor.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/AudioWaveformSlicerEditor_302e651a.o: ../../src/Cue/audio/ui/AudioWaveformSlicerEditor.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling AudioWaveformSlicerEditor.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/AudioSlices_6406366c.o: ../../src/Cue/audio/AudioSlices.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling AudioSlices.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/AudioWaveformSlicer_8710d808.o: ../../src/Cue/audio/AudioWaveformSlicer.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling AudioWaveformSlicer.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/AudioCue_6b8f1412.o: ../../src/Cue/audio/AudioCue.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling AudioCue.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/AudioFile_d20cdec7.o: ../../src/Cue/audio/AudioFile.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling AudioFile.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/CueEditor_6ade7941.o: ../../src/Cue/ui/CueEditor.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling CueEditor.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/DCAAssignmentDialog_62e17685.o: ../../src/Cue/dca/ui/DCAAssignmentDialog.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling DCAAssignmentDialog.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/DCAAssignmentEditor_58ab002a.o: ../../src/Cue/dca/ui/DCAAssignmentEditor.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling DCAAssignmentEditor.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/CharacterRef_390d4313.o: ../../src/Cue/dca/CharacterRef.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling CharacterRef.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/DCAAssignment_a075f530.o: ../../src/Cue/dca/DCAAssignment.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling DCAAssignment.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/DCACue_2d8b60fa.o: ../../src/Cue/dca/DCACue.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling DCACue.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/GoCue_f384cdc0.o: ../../src/Cue/go/GoCue.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling GoCue.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/Cue_4c4af201.o: ../../src/Cue/Cue.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling Cue.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/CueManager_4c7e6ef6.o: ../../src/Cue/CueManager.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling CueManager.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/DCAMixingCuelistEditor_2970c9d0.o: ../../src/Cuelist/dca/ui/DCAMixingCuelistEditor.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling DCAMixingCuelistEditor.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/DCAMixingCuelist_50adf7d4.o: ../../src/Cuelist/dca/DCAMixingCuelist.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling DCAMixingCuelist.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/CuelistEditor_b29c701.o: ../../src/Cuelist/ui/CuelistEditor.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling CuelistEditor.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/CuelistManagerItemUI_efa5716a.o: ../../src/Cuelist/ui/CuelistManagerItemUI.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling CuelistManagerItemUI.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/CuelistManagerUI_4123bb77.o: ../../src/Cuelist/ui/CuelistManagerUI.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling CuelistManagerUI.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/Cuelist_15802d3d.o: ../../src/Cuelist/Cuelist.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling Cuelist.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/CuelistFactory_ee0f8317.o: ../../src/Cuelist/CuelistFactory.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling CuelistFactory.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/CuelistManager_ebc7583a.o: ../../src/Cuelist/CuelistManager.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling CuelistManager.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/CuesTableModel_7cc12950.o: ../../src/Deck/ui/CuesTableModel.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling CuesTableModel.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/CuesTableUI_f48ef477.o: ../../src/Deck/ui/CuesTableUI.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling CuesTableUI.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/DeckUI_fc73c092.o: ../../src/Deck/ui/DeckUI.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling DeckUI.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/DeckViewHeaderUI_ff918184.o: ../../src/Deck/ui/DeckViewHeaderUI.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling DeckViewHeaderUI.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/DeckViewUI_e4afbb17.o: ../../src/Deck/ui/DeckViewUI.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling DeckViewUI.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/ReorderCuesWindow_464109e0.o: ../../src/Deck/ui/ReorderCuesWindow.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling ReorderCuesWindow.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/GoAction_75b04dcb.o: ../../src/Interface/action/GoAction.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling GoAction.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MappingAction_5677e781.o: ../../src/Interface/action/MappingAction.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MappingAction.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MappingActionFactory_2ba27453.o: ../../src/Interface/action/MappingActionFactory.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MappingActionFactory.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/PanicAction_2c2e03ea.o: ../../src/Interface/action/PanicAction.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling PanicAction.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/SelectNextCueAction_16dbc2d7.o: ../../src/Interface/action/SelectNextCueAction.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling SelectNextCueAction.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/SelectPreviousCueAction_ba949353.o: ../../src/Interface/action/SelectPreviousCueAction.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling SelectPreviousCueAction.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/AudioInterfaceHardwareEditor_d10a2fce.o: ../../src/Interface/audio/ui/AudioInterfaceHardwareEditor.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling AudioInterfaceHardwareEditor.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/AudioOutputEditor_dae63958.o: ../../src/Interface/audio/ui/AudioOutputEditor.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling AudioOutputEditor.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/AudioInterface_a266d812.o: ../../src/Interface/audio/AudioInterface.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling AudioInterface.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/AudioOutput_1c8db912.o: ../../src/Interface/audio/AudioOutput.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling AudioOutput.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MIDIInterface_77dce880.o: ../../src/Interface/midi/MIDIInterface.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MIDIInterface.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MIDIDevice_de7e8239.o: ../../src/Interface/midi/MIDIDevice.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MIDIDevice.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MIDIManager_52484d74.o: ../../src/Interface/midi/MIDIManager.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MIDIManager.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MIDIMapping_98fde15.o: ../../src/Interface/midi/MIDIMapping.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MIDIMapping.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MIDIFeedbackFactory_38648d8c.o: ../../src/Interface/midi/MIDIFeedbackFactory.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MIDIFeedbackFactory.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MIDIFeedbackItem_e100d89b.o: ../../src/Interface/midi/MIDIFeedbackItem.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MIDIFeedbackItem.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MIDIIsPanickingFeedback_c4b7d4f0.o: ../../src/Interface/midi/MIDIIsPanickingFeedback.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MIDIIsPanickingFeedback.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MIDIIsPlayingFeedback_ae3f4490.o: ../../src/Interface/midi/MIDIIsPlayingFeedback.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MIDIIsPlayingFeedback.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/AHCQMixerSettings_c6532099.o: ../../src/Interface/mixer/ahcq/AHCQMixerSettings.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling AHCQMixerSettings.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/AHCQProtocol_b5d245b5.o: ../../src/Interface/mixer/ahcq/AHCQProtocol.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling AHCQProtocol.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/WingOscMixerSettings_bbefb8f.o: ../../src/Interface/mixer/wingosc/WingOscMixerSettings.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling WingOscMixerSettings.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/WingOscProtocol_319ae8ff.o: ../../src/Interface/mixer/wingosc/WingOscProtocol.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling WingOscProtocol.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/wapi_mingw_shim_56476567.o: ../../src/Interface/mixer/wingtcp/wapi_mingw_shim.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling wapi_mingw_shim.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/WingTcpMixerSettings_e9b7dc4b.o: ../../src/Interface/mixer/wingtcp/WingTcpMixerSettings.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling WingTcpMixerSettings.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/Character_4db51f37.o: ../../src/Interface/mixer/Character.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling Character.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MixerChannel_ff00e9f6.o: ../../src/Interface/mixer/MixerChannel.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MixerChannel.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MixerFX_b07eb3e9.o: ../../src/Interface/mixer/MixerFX.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MixerFX.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MixerInterface_4b8bd7ac.o: ../../src/Interface/mixer/MixerInterface.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MixerInterface.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OBSCommandEditor_5d7a2d58.o: ../../src/Interface/obs/ui/OBSCommandEditor.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OBSCommandEditor.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OBSCommand_ff2f2710.o: ../../src/Interface/obs/OBSCommand.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OBSCommand.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OBSInterface_4495a9fe.o: ../../src/Interface/obs/OBSInterface.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OBSInterface.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OSCCommandArgumentsEditor_32cb47c2.o: ../../src/Interface/osc/ui/OSCCommandArgumentsEditor.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OSCCommandArgumentsEditor.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OSCCommandEditor_99163e38.o: ../../src/Interface/osc/ui/OSCCommandEditor.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OSCCommandEditor.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OSCInputEditor_1e7922b7.o: ../../src/Interface/osc/ui/OSCInputEditor.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OSCInputEditor.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OSCCommand_9cf21fd2.o: ../../src/Interface/osc/OSCCommand.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OSCCommand.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OSCInterface_7d7d7a40.o: ../../src/Interface/osc/OSCInterface.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OSCInterface.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OSCCurrentCueFeedback_52210c42.o: ../../src/Interface/osc/OSCCurrentCueFeedback.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OSCCurrentCueFeedback.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OSCFeedbackFactory_9a40cf4c.o: ../../src/Interface/osc/OSCFeedbackFactory.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OSCFeedbackFactory.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OSCFeedbackItem_b9a1eedb.o: ../../src/Interface/osc/OSCFeedbackItem.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OSCFeedbackItem.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OSCIsPanickingFeedback_771d36b0.o: ../../src/Interface/osc/OSCIsPanickingFeedback.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OSCIsPanickingFeedback.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OSCIsPlayingFeedback_a121650.o: ../../src/Interface/osc/OSCIsPlayingFeedback.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OSCIsPlayingFeedback.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OSCMapping_ada9dfd5.o: ../../src/Interface/osc/OSCMapping.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OSCMapping.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/OSCNextCueFeedback_c1b9b76c.o: ../../src/Interface/osc/OSCNextCueFeedback.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling OSCNextCueFeedback.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/InterfaceManagerUI_f9a324b7.o: ../../src/Interface/ui/InterfaceManagerUI.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling InterfaceManagerUI.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/InterfaceUI_e393dec8.o: ../../src/Interface/ui/InterfaceUI.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling InterfaceUI.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/FeedbackItem_3ca1f296.o: ../../src/Interface/feedback/FeedbackItem.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling FeedbackItem.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/Interface_9e38c80d.o: ../../src/Interface/Interface.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling Interface.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/InterfaceManager_8001f36a.o: ../../src/Interface/InterfaceManager.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling InterfaceManager.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/AudioSettings_1417810.o: ../../src/ProjectSettings/AudioSettings.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling AudioSettings.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/ColorPresets_884d5ac4.o: ../../src/ProjectSettings/ColorPresets.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling ColorPresets.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/DecksSettings_fe0d7724.o: ../../src/ProjectSettings/DecksSettings.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling DecksSettings.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/ShowProperties_a8318b43.o: ../../src/ProjectSettings/ShowProperties.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling ShowProperties.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/VolumePresets_e2087271.o: ../../src/ProjectSettings/VolumePresets.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling VolumePresets.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/Clock_bf28f053.o: ../../src/ui/panels/Clock.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling Clock.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/ShowControl_5e188ae5.o: ../../src/ui/panels/ShowControl.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling ShowControl.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/ShowInfos_e95c3bcd.o: ../../src/ui/panels/ShowInfos.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling ShowInfos.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/LookAndFeelTable_17cb8c95.o: ../../src/ui/LookAndFeelTable.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling LookAndFeelTable.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/AboutWindow_db489c22.o: ../../src/ui/AboutWindow.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling AboutWindow.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/SPAssetManager_e9ba871f.o: ../../src/ui/SPAssetManager.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling SPAssetManager.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/Brain_7bbbce80.o: ../../src/Brain.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling Brain.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/Main_36be1979.o: ../../src/Main.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling Main.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MainComponent_dd9a9ece.o: ../../src/MainComponent.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MainComponent.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MainComponentCommands_fa1e5076.o: ../../src/MainComponentCommands.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MainComponentCommands.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MainIncludes_af4e4ce4.o: ../../src/MainIncludes.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MainIncludes.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MainMenuBarComponent_c1978242.o: ../../src/MainMenuBarComponent.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling MainMenuBarComponent.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/PonyEngine_63977cec.o: ../../src/PonyEngine.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling PonyEngine.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/UserInputManager_a645b8ae.o: ../../src/UserInputManager.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling UserInputManager.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/BinaryData_ce4232d4.o: ../../JuceLibraryCode/BinaryData.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling BinaryData.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_audio_basics_8a4e984a.o: ../../JuceLibraryCode/include_juce_audio_basics.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_audio_basics.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_audio_devices_63111d02.o: ../../JuceLibraryCode/include_juce_audio_devices.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_audio_devices.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_audio_formats_15f82001.o: ../../JuceLibraryCode/include_juce_audio_formats.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_audio_formats.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_audio_processors_10c03666.o: ../../JuceLibraryCode/include_juce_audio_processors.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_audio_processors.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_audio_processors_ara_2a4c6ef7.o: ../../JuceLibraryCode/include_juce_audio_processors_ara.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_audio_processors_ara.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_audio_processors_lv2_libs_12bdca08.o: ../../JuceLibraryCode/include_juce_audio_processors_lv2_libs.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_audio_processors_lv2_libs.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_audio_utils_9f9fb2d6.o: ../../JuceLibraryCode/include_juce_audio_utils.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_audio_utils.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_core_f26d17db.o: ../../JuceLibraryCode/include_juce_core.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_core.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_cryptography_8cb807a8.o: ../../JuceLibraryCode/include_juce_cryptography.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_cryptography.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_data_structures_7471b1e3.o: ../../JuceLibraryCode/include_juce_data_structures.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_data_structures.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_dsp_aeb2060f.o: ../../JuceLibraryCode/include_juce_dsp.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_dsp.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_events_fd7d695.o: ../../JuceLibraryCode/include_juce_events.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_events.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_graphics_f817e147.o: ../../JuceLibraryCode/include_juce_graphics.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_graphics.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_gui_basics_e3f79785.o: ../../JuceLibraryCode/include_juce_gui_basics.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_gui_basics.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_gui_extra_6dee1c1a.o: ../../JuceLibraryCode/include_juce_gui_extra.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_gui_extra.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_opengl_a8a032b.o: ../../JuceLibraryCode/include_juce_opengl.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_opengl.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_organicui_2c36794d.o: ../../JuceLibraryCode/include_juce_organicui.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_organicui.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_organicui2_5ab8c64f.o: ../../JuceLibraryCode/include_juce_organicui2.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_organicui2.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_organicui2b_ff2481bd.o: ../../JuceLibraryCode/include_juce_organicui2b.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_organicui2b.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_organicui3_5ac6ddd0.o: ../../JuceLibraryCode/include_juce_organicui3.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_organicui3.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_organicui4_5ad4f551.o: ../../JuceLibraryCode/include_juce_organicui4.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_organicui4.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_osc_f3df604d.o: ../../JuceLibraryCode/include_juce_osc.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_osc.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_simpleweb_acec7570.o: ../../JuceLibraryCode/include_juce_simpleweb.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_simpleweb.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/include_juce_simpleweb2_f0c24e8c.o: ../../JuceLibraryCode/include_juce_simpleweb2.cpp
	-$(V_AT)mkdir -p $(@D)
	@echo "Compiling include_juce_simpleweb2.cpp"
	$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_APP) $(JUCE_CFLAGS_APP) -o "$@" -c "$<"

$(JUCE_OBJDIR)/execinfo.cmd:
	-$(V_AT)mkdir -p $(@D)
	-@if [ -z "$(V_AT)" ]; then echo "Checking if we need to link libexecinfo"; fi
	$(V_AT)printf "int main() { return 0; }" | $(CXX) -x c++ -o $(@D)/execinfo.x -lexecinfo - >/dev/null 2>&1 && printf -- "-lexecinfo" > "$@" || touch "$@"

$(JUCE_OBJDIR)/cxxfs.cmd:
	-$(V_AT)mkdir -p $(@D)
	-@if [ -z "$(V_AT)" ]; then echo "Checking if we need to link stdc++fs"; fi
	$(V_AT)printf "int main() { return 0; }" | $(CXX) -x c++ -o $(@D)/cxxfs.x -lstdc++fs - >/dev/null 2>&1 && printf -- "-lstdc++fs" > "$@" || touch "$@"

clean:
	@echo Cleaning SnoringPony
	$(V_AT)$(CLEANCMD)

strip:
	@echo Stripping SnoringPony
	-$(V_AT)$(STRIP) --strip-unneeded $(JUCE_OUTDIR)/$(JUCE_TARGET_APP)

-include $(OBJECTS_APP:%.o=%.d)
