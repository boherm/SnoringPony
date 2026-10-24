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

void AudioPlayerMixer::fadeIn(double duration, float targetGain)
{
    fadingGain.reset(sampleRate, 0.0);
    fadingGain.setTargetValue(0.0f);
    fadingGain.reset(sampleRate, duration);
    fadingGain.setTargetValue(targetGain);
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

void SkipRegionsAudioSource::setInput(PositionableAudioSource* source, double sourceSampleRate)
{
    input = source;
    inputSampleRate = sourceSampleRate > 0.0 ? sourceSampleRate : 44100.0;
    crossfadeRemaining = 0;

    const SpinLock::ScopedLockType lock(regionsLock);
    regionsChanged = true; // sample positions depend on the source rate
}

void SkipRegionsAudioSource::setSkipRegions(const Array<Range<double>>& newRegions)
{
    Array<Range<double>> sorted;
    for (auto& r : newRegions)
        if (r.getLength() > 0.0)
            sorted.add(r);

    std::sort(sorted.begin(), sorted.end(), [](const Range<double>& a, const Range<double>& b) { return a.getStart() < b.getStart(); });

    // merge overlapping/adjacent regions so a jump never lands inside another one
    Array<Range<double>> merged;
    for (auto& r : sorted)
    {
        if (!merged.isEmpty() && r.getStart() <= merged.getLast().getEnd())
            merged.getReference(merged.size() - 1) = merged.getLast().getUnionWith(r);
        else
            merged.add(r);
    }

    const SpinLock::ScopedLockType lock(regionsLock);
    pendingRegions.swapWith(merged);
    regionsChanged = true;
}

void SkipRegionsAudioSource::pullRegions()
{
    const SpinLock::ScopedTryLockType lock(regionsLock);
    if (!lock.isLocked() || !regionsChanged)
        return;

    numRegions = 0;
    for (auto& r : pendingRegions)
    {
        if (numRegions >= maxRegions)
            break;
        regions[numRegions++] = Range<int64>((int64)(r.getStart() * inputSampleRate), (int64)(r.getEnd() * inputSampleRate));
    }
    crossfadeLength = jmax(1, (int)(crossfadeSeconds * inputSampleRate));
    regionsChanged = false;
}

void SkipRegionsAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    tailBuffer.setSize(8, jmax(samplesPerBlockExpected, 1024));
    if (input != nullptr)
        input->prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void SkipRegionsAudioSource::releaseResources()
{
    if (input != nullptr)
        input->releaseResources();
}

void SkipRegionsAudioSource::readFromInput(AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    AudioSourceChannelInfo sub(&buffer, startSample, numSamples);
    input->getNextAudioBlock(sub);
}

void SkipRegionsAudioSource::getNextAudioBlock(const AudioSourceChannelInfo& info)
{
    if (input == nullptr)
    {
        info.clearActiveBufferRegion();
        return;
    }

    pullRegions();

    if (numRegions == 0)
    {
        crossfadeRemaining = 0;
        input->getNextAudioBlock(info);
        return;
    }

    int offset = 0;
    while (offset < info.numSamples)
    {
        int64 pos = input->getNextReadPosition();

        // landed inside a region (seek / start position): jump to its end, no crossfade
        for (int r = 0; r < numRegions; r++)
        {
            if (regions[r].contains(pos))
            {
                pos = regions[r].getEnd();
                input->setNextReadPosition(pos);
                crossfadeRemaining = 0;
            }
        }

        // next region starting ahead of us limits this chunk
        int regionIndex = -1;
        int64 chunk = info.numSamples - offset;
        for (int r = 0; r < numRegions; r++)
        {
            if (regions[r].getStart() > pos && regions[r].getStart() - pos <= chunk)
            {
                chunk = regions[r].getStart() - pos;
                regionIndex = r;
                break; // regions are sorted
            }
        }
        if (crossfadeRemaining > 0)
            chunk = jmin<int64>(chunk, crossfadeRemaining, tailBuffer.getNumSamples());

        const int n = (int)chunk;
        readFromInput(*info.buffer, info.startSample + offset, n);

        if (crossfadeRemaining > 0)
        {
            // keep reading the cut part, fading it out while the new part fades in
            const int numChannels = jmin(info.buffer->getNumChannels(), 32);
            tailBuffer.setSize(jmax(numChannels, tailBuffer.getNumChannels()), tailBuffer.getNumSamples(), true, false, true);

            input->setNextReadPosition(crossfadeTailPos);
            readFromInput(tailBuffer, 0, n);
            input->setNextReadPosition(pos + n);

            const int done = crossfadeTotal - crossfadeRemaining;
            for (int i = 0; i < n; i++)
            {
                const float t = (float)(done + i + 1) / (float)crossfadeTotal;
                const float gIn = std::sin(t * MathConstants<float>::halfPi);
                const float gOut = std::cos(t * MathConstants<float>::halfPi);
                for (int ch = 0; ch < numChannels; ch++)
                {
                    float* out = info.buffer->getWritePointer(ch, info.startSample + offset);
                    out[i] = out[i] * gIn + tailBuffer.getSample(ch, i) * gOut;
                }
            }

            crossfadeTailPos += n;
            crossfadeRemaining -= n;
        }

        offset += n;

        if (regionIndex >= 0 && pos + n == regions[regionIndex].getStart())
        {
            // reached a region: splice to its end and crossfade with the cut part
            crossfadeTailPos = regions[regionIndex].getStart();
            crossfadeRemaining = (int)jmin<int64>(crossfadeLength, regions[regionIndex].getLength());
            crossfadeTotal = jmax(1, crossfadeRemaining);
            input->setNextReadPosition(regions[regionIndex].getEnd());
        }
    }
}

void SkipRegionsAudioSource::setNextReadPosition(int64 newPosition)
{
    crossfadeRemaining = 0;
    if (input != nullptr)
        input->setNextReadPosition(newPosition);
}

int64 SkipRegionsAudioSource::getNextReadPosition() const
{
    return input != nullptr ? input->getNextReadPosition() : 0;
}

int64 SkipRegionsAudioSource::getTotalLength() const
{
    return input != nullptr ? input->getTotalLength() : 0;
}

bool SkipRegionsAudioSource::isLooping() const
{
    return input != nullptr && input->isLooping();
}

void SkipRegionsAudioSource::setLooping(bool shouldLoop)
{
    if (input != nullptr)
        input->setLooping(shouldLoop);
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
                skipSource.setInput(nullptr, 0.0);
                readerSource.reset();
            }
            readerSource.reset(new AudioFormatReaderSource(reader.release(), true));
            const double fileSampleRate = readerSource->getAudioFormatReader()->sampleRate;
            skipSource.setInput(readerSource.get(), fileSampleRate);
            transport->setSource(&skipSource, 0, nullptr, fileSampleRate);
            transport->start();
            mixer->resetPanicFade();
            if (resetFade)
                mixer->resetFade();
        }
    }
}

void AudioPlayer::setSkipRegions(const Array<Range<double>>& regions)
{
    skipSource.setSkipRegions(regions);
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

void AudioPlayer::fadeIn(double duration, float targetGain)
{
    mixer->fadeIn(duration, targetGain);
    play(false);
}

void AudioPlayer::fadeOut(double duration, bool stopAfterFade)
{
    mixer->fadeOut(duration, stopAfterFade);
}

double AudioPlayer::getFadeMultiplier() const
{
    return mixer->fadingGain.getCurrentValue() * mixer->panicFadingGain.getCurrentValue();
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
