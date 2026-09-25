/*
  ==============================================================================

    PlaylistCue.h
    Created: 3 Dec 2025 06:30:22pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Cue.h"
#include "../../Audio/AudioPlayer.h"

class PluginChainManager;

class PlaylistFile :
    public BaseItem,
    public ChangeListener
{
public:
    PlaylistFile(String name = "File 1");
    virtual ~PlaylistFile();

    std::unique_ptr<AudioPlayer> player;
    FileParameter* audioFile;
    FloatParameter* duration;
    BoolParameter* isPlaying;

    String getTypeString() const override { return "Playlist File"; }
    static PlaylistFile* create(var params) { return new PlaylistFile(params.getProperty("name", "Playlist File")); };

    void parameterValueChanged(Parameter* p) override;
    void changeListenerCallback(ChangeBroadcaster* source) override;
};

// -----------------------------------------------------

class PlaylistCue :
    public Cue,
    public ContainerAsyncListener,
    public Timer,
    public ChangeListener
{
public:
    PlaylistCue(var params = var());
    virtual ~PlaylistCue();

    bool askedToStop = false;

    int currentOrderIndex = -1;
    Array<int> playlistOrder = Array<int>();

    // Runtime playback state (not saved). currentFile is the track that owns the cue's
    // timeline: during a crossfade the outgoing track still sounds but no longer drives
    // anything. elapsedBeforeCurrentFile is the playlist time consumed before it started,
    // so currentTime stays monotonic even while two tracks overlap.
    PlaylistFile* currentFile = nullptr;
    double elapsedBeforeCurrentFile = 0.0;
    bool crossfadeStarted = false;

    TargetParameter* outputTarget;
    FloatParameter* volume;
    BoolParameter* loop;
    BoolParameter* shuffle;

    EnablingControllableContainer* crossfadeCC;
    FloatParameter* crossfadeDuration;

    std::unique_ptr<BaseManager<PlaylistFile>> filesManager;
    PluginChainManager* pluginChainManager;

    String getTypeString() const override { return "Playlist Cue"; }
    String getCueType() const override { return "Playlist"; }
    juce::StringArray getMultiEditHiddenControllableNames() const override;
    juce::StringArray getMultiEditUnsetProxyNames() const override;
    static PlaylistCue* create(var params) { return new PlaylistCue(params); }

    void loadJSONDataItemInternal(juce::var data) override;

    void updateWarnings();
    bool canBePlayed() override;

    void playInternal() override;
    void stopInternal() override;
    void retriggerStop() override;
    void fadeAndStop(double fadeTime) override;
    void panicInternal() override;
    void fade(double targetGain, double duration) override;
    void fadeOut(double duration, bool stopAfterFade = true);

    // Volume fader (Active Cues panel).
    bool  hasVolumeFader() override { return true; }
    float getFaderVolume() override;
    float getMaxFaderVolume() override { return 1.5f; }
    void  setFaderVolume(float v) override;
    float getOutputLevel() override;
    void refreshGlobalDuration();
    void refreshDuration() override { refreshGlobalDuration(); }
    void refreshAudioOutput();
    void refreshVolume();
    void generatePlaylistOrder();
    PlaylistFile* getNextFileToPlay(int* nextOrderIndex = nullptr);
    PlaylistFile* peekNextFileToPlay(int* nextOrderIndex = nullptr) const;

    // Overlap duration between two consecutive tracks, 0 when the Crossfade section is off.
    double getCrossfadeTime() const;
    void startCrossfadeToNext();

    void parameterValueChanged(Parameter* p) override;
    void parameterControlModeChanged(Parameter* p) override;
    void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;
    void newMessage(const ContainerAsyncEvent& e) override;
    void changeListenerCallback(ChangeBroadcaster* source) override;

    String autoDescriptionInternal() override;

    void createFromFiles(const StringArray& files);

private:
    void timerCallback() override;
};
