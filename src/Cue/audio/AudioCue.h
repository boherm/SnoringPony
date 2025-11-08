/*
  ==============================================================================

    AudioCue.h
    Created: 19 Oct 2025 11:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Cue.h"

// #include <memory>
// #include <unordered_map>
// #include <vector>

class AudioManager;
class AudioOutput;

class AudioCue :
    public Cue,
    // private ChangeListener,
    public ContainerAsyncListener
{
public:
    AudioCue(var params = var());
    virtual ~AudioCue();

    AudioManager* audioManager;
    AudioFormatManager formatManager;

    String getTypeString() const override { return "Audio"; }
    String getCueType() const override { return "Audio"; }
    static AudioCue* create(var params) { return new AudioCue(params); }

    void newMessage(const ContainerAsyncEvent& e) override;

    void play() override;

// private:
//     struct OutputPlaybackContext
//     {
//         AudioOutput* output = nullptr;
//         std::unique_ptr<MixerAudioSource> mixer;
//         std::unique_ptr<AudioSourcePlayer> player;
//         int activeTransports = 0;
//     };

//     struct PlaybackInstance
//     {
//         OutputPlaybackContext* context = nullptr;
//         std::unique_ptr<AudioFormatReaderSource> readerSource;
//         std::unique_ptr<AudioTransportSource> transport;
//     };

//     std::unordered_map<AudioOutput*, std::unique_ptr<OutputPlaybackContext>> outputContexts;
//     std::vector<std::unique_ptr<PlaybackInstance>> activePlaybacks;

//     OutputPlaybackContext* getOrCreateOutputContext(AudioOutput*);
//     void releaseOutputContext(AudioOutput*);
//     void stopAllPlaybacks();

    // void changeListenerCallback(ChangeBroadcaster* source) override;
};
