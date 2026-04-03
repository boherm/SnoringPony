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

    TargetParameter* outputTarget;
    FloatParameter* volume;
    BoolParameter* loop;
    BoolParameter* shuffle;

    std::unique_ptr<BaseManager<PlaylistFile>> filesManager;

    String getTypeString() const override { return "Playlist Cue"; }
    String getCueType() const override { return "Playlist"; }
    static PlaylistCue* create(var params) { return new PlaylistCue(params); }

    void loadJSONDataItemInternal(juce::var data) override;

    void playInternal() override;
    void stopInternal() override;
    void retriggerStop() override;
    void panicInternal() override;
    void fade(double targetGain, double duration) override;
    void fadeOut(double duration, bool stopAfterFade = true);
    void refreshGlobalDuration();
    void refreshAudioOutput();
    void refreshVolume();
    void generatePlaylistOrder();
    PlaylistFile* getNextFileToPlay();

    void parameterValueChanged(Parameter* p) override;
    void parameterControlModeChanged(Parameter* p) override;
    void newMessage(const ContainerAsyncEvent& e) override;
    void changeListenerCallback(ChangeBroadcaster* source) override;

    String autoDescriptionInternal() override;

    void createFromFiles(const StringArray& files);

private:
    void timerCallback() override;
};
