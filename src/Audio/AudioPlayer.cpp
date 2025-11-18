/*
  ==============================================================================

    AudioPlayer.cpp
    Created: 11 Nov 2025 06:22:20pm
    Author:  boherm

  ==============================================================================
*/

#include "AudioPlayer.h"
#include "../Interface/audio/AudioInterface.h"
#include "../Interface/audio/AudioOutput.h"

void AudioPlayerMixer::panicFade()
{
    if (isPanicking) {
        transportSource->stop();
        isPanicking = false;
        return;
    }

    panicFadingGain.reset(sampleRate, 5.0);
    panicFadingGain.setTargetValue(0.0f);
    isPanicking = true;
}

void AudioPlayerMixer::resetPanicFade()
{
    panicFadingGain.reset(sampleRate, 0.0);
    panicFadingGain.setTargetValue(1.0f);
    isPanicking = false;
}

void AudioPlayerMixer::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    MixerAudioSource::prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void AudioPlayerMixer::getNextAudioBlock(const AudioSourceChannelInfo& info)
{
    MixerAudioSource::getNextAudioBlock(info);

    auto* buffer = info.buffer;
    auto start   = info.startSample;
    auto num     = info.numSamples;

    for (int ch = 0; ch < buffer->getNumChannels(); ++ch)
    {
        float* data = buffer->getWritePointer (ch, start);
        for (int i = 0; i < num; ++i) {
            data[i] *= panicFadingGain.getNextValue();
        }
    }

    if (isPanicking && !panicFadingGain.isSmoothing() && panicFadingGain.getCurrentValue() <= 0.0f)
    {
        fadeStopPending = true;
        isPanicking = false;

        if (transportSource != nullptr)
        {
            AudioTransportSource* transportToStop = transportSource;
            MessageManager::callAsync([transportToStop]()
            {
                if (transportToStop != nullptr)
                {
                    transportToStop->stop();
                    transportToStop->setPosition(0.0);
                }
            });
        }
    }

}

//==============================================================================

AudioPlayer::AudioPlayer()
{
    formatManager.registerBasicFormats();
    player = new AudioSourcePlayer();
    mixer = new AudioPlayerMixer();
    transport = new AudioTransportSource();
    mixer->addInputSource(transport, false);
    mixer->setSource(transport);
    player->setSource(mixer);
}

AudioPlayer::~AudioPlayer()
{
    stopAndClean();
    player->setSource(nullptr);
    mixer->removeAllInputs();
    delete mixer;
    delete readerSource;
    delete transport;
    delete player;
}

bool AudioPlayer::setFile(const File& audioFile)
{
    if (!audioFile.existsAsFile())
        return false;

    currentFile = audioFile;
    duration = 0;

    std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
    if (reader == nullptr)
        return false;

    duration = reader->lengthInSamples / reader->sampleRate;

    return true;
}

bool AudioPlayer::setOutput(AudioOutput* output)
{
    if (this->output != nullptr)
        this->output->am.removeAudioCallback(player);

    if (output == nullptr)
        return false;

    this->output = output;
    this->output->am.addAudioCallback(player);

    return true;
}

void AudioPlayer::play()
{
    if (currentFile.existsAsFile())
    {
        std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(currentFile));
        if (reader != nullptr)
        {
            if (readerSource != nullptr) {
                transport->stop();
                transport->setSource(nullptr);
                delete readerSource;
            }
            readerSource = new AudioFormatReaderSource(reader.release(), true);
            transport->setSource(readerSource, 0, nullptr, readerSource->getAudioFormatReader()->sampleRate);
            transport->setPosition(0.0);
            transport->start();
            mixer->resetPanicFade();
        }
    }
}

void AudioPlayer::stop()
{
    transport->stop();
    transport->setPosition(0.0);
}

void AudioPlayer::panic()
{
    mixer->panicFade();
}

void AudioPlayer::stopAndClean()
{
    stop();
    transport->setSource(nullptr);
    if (output != nullptr)
        output->am.removeAudioCallback(player);
}
