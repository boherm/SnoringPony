/*
  ==============================================================================

    AudioPlayer.cpp
    Created: 11 Nov 2025 06:22:20pm
    Author:  boherm

  ==============================================================================
*/

#include "AudioPlayer.h"
#include "PluginSlot.h"
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

void AudioPlayerMixer::fade(double targetGain, double duration)
{
    fadingGain.reset(sampleRate, duration);
    fadingGain.setTargetValue(targetGain);
    isFading = true;
}

void AudioPlayerMixer::fadeIn(double duration)
{
    fadingGain.reset(sampleRate, 0.0);
    fadingGain.setTargetValue(0.0f);
    fadingGain.reset(sampleRate, duration);
    fadingGain.setTargetValue(1.0f);
    isFading = true;
    fadeStopAfterComplete = false;
}

void AudioPlayerMixer::fadeOut(double duration, bool stopAfterFade)
{
    fadingGain.reset(sampleRate, duration);
    fadingGain.setTargetValue(0.0f);
    isFading = true;
    fadeStopAfterComplete = stopAfterFade;
}

void AudioPlayerMixer::resetFade()
{
    fadingGain.reset(sampleRate, 0.0);
    fadingGain.setTargetValue(1.0f);
    isFading = false;
    fadeStopAfterComplete = false;
}

void AudioPlayerMixer::prepareToPlay(int samplesPerBlockExpected, double sr)
{
    MixerAudioSource::prepareToPlay(samplesPerBlockExpected, sr);
    sampleRate = sr;
    blockSize = samplesPerBlockExpected;

    if (pluginChain != nullptr)
    {
        for (auto* slot : pluginChain->items)
            slot->prepareToPlay(sr, samplesPerBlockExpected, numChannels);
    }
}

void AudioPlayerMixer::getNextAudioBlock(const AudioSourceChannelInfo& info)
{
    MixerAudioSource::getNextAudioBlock(info);

    numChannels = info.buffer->getNumChannels();

    if (pluginChain != nullptr && !pluginChain->items.isEmpty())
    {
        int bufChannels = info.buffer->getNumChannels();
        int samples = info.numSamples;
        int start = info.startSample;

        // Find the max channel count any plugin needs
        int maxChannels = bufChannels;
        for (auto* slot : pluginChain->items)
        {
            if (slot->pluginInstance != nullptr && slot->enabled->boolValue())
            {
                maxChannels = jmax(maxChannels, slot->pluginInstance->getTotalNumInputChannels());
                maxChannels = jmax(maxChannels, slot->pluginInstance->getTotalNumOutputChannels());
            }
        }

        if (maxChannels > bufChannels)
        {
            // Need a wider buffer - copy our channels in, pad with silence
            AudioBuffer<float> pluginBuffer(maxChannels, samples);
            pluginBuffer.clear();
            for (int ch = 0; ch < bufChannels; ++ch)
                pluginBuffer.copyFrom(ch, 0, *info.buffer, ch, start, samples);

            MidiBuffer emptyMidi;
            for (auto* slot : pluginChain->items)
                slot->processBlock(pluginBuffer, emptyMidi);

            // Copy back to original buffer
            for (int ch = 0; ch < bufChannels; ++ch)
                info.buffer->copyFrom(ch, start, pluginBuffer, ch, 0, samples);
        }
        else
        {
            // Fast path: wrap existing buffer directly with offset pointers
            float* channelPtrs[32];
            for (int ch = 0; ch < bufChannels; ++ch)
                channelPtrs[ch] = info.buffer->getWritePointer(ch) + start;

            AudioBuffer<float> pluginBuffer(channelPtrs, bufChannels, samples);
            MidiBuffer emptyMidi;

            for (auto* slot : pluginChain->items)
                slot->processBlock(pluginBuffer, emptyMidi);
        }
    }

    auto* buffer = info.buffer;
    auto start   = info.startSample;
    auto num     = info.numSamples;

    auto numChannels = buffer->getNumChannels();

    for (int i = 0; i < num; ++i)
    {
        float panicGain = panicFadingGain.getNextValue();
        float fadeGain = fadingGain.getNextValue();
        float gain = panicGain * fadeGain;

        for (int ch = 0; ch < numChannels; ++ch)
            buffer->getWritePointer(ch, start)[i] *= gain;
    }

    if (isPanicking && !panicFadingGain.isSmoothing() && panicFadingGain.getCurrentValue() <= 0.0f)
    {
        fadeStopAfterComplete = false;
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

    if (!isPanicking && isFading && !fadingGain.isSmoothing())
    {
        isFading = false;

        if (fadeStopAfterComplete && fadingGain.getCurrentValue() <= 0.01f)
        {
            fadeStopAfterComplete = false;

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

void AudioPlayer::play(bool resetFade)
{
    if (currentFile.existsAsFile())
    {
        std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(currentFile));
        if (reader != nullptr)
        {
            if (readerSource != nullptr) {
                transport->stop();
                transport->setSource(nullptr);
                readerSource.reset();
            }
            readerSource.reset(new AudioFormatReaderSource(reader.release(), true));
            transport->setSource(readerSource.get(), 0, nullptr, readerSource->getAudioFormatReader()->sampleRate);
            transport->start();
            mixer->resetPanicFade();
            if (resetFade)
                mixer->resetFade();
        }
    }
}

void AudioPlayer::preview(AudioOutput* previewOutput)
{
    setOutput(previewOutput);
    play();
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

void AudioPlayer::fade(double targetGain, double duration)
{
    mixer->fade(targetGain, duration);
}

void AudioPlayer::fadeIn(double duration)
{
    mixer->fadeIn(duration);
    play(false);
}

void AudioPlayer::fadeOut(double duration, bool stopAfterFade)
{
    mixer->fadeOut(duration, stopAfterFade);
}

void AudioPlayerMixer::setPluginChain(PluginChainManager* chain)
{
    pluginChain = chain;

    if (pluginChain != nullptr)
    {
        for (auto* slot : pluginChain->items)
            slot->prepareToPlay(sampleRate, blockSize, numChannels);
    }
}
