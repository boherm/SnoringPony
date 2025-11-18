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

AudioPlayer::AudioPlayer()
{
    formatManager.registerBasicFormats();
    player = new AudioSourcePlayer();
    transport = new AudioTransportSource();
    player->setSource(transport);
}

AudioPlayer::~AudioPlayer()
{
    stopAndClean();
    player->setSource(nullptr);
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
        }
    }
}

void AudioPlayer::stop()
{
    transport->stop();
    transport->setPosition(0.0);
}

void AudioPlayer::stopAndClean()
{
    stop();
    transport->setSource(nullptr);
    if (output != nullptr)
        output->am.removeAudioCallback(player);
}


// void AudioPlayer::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
// {
//     Logger::writeToLog("AudioPlayer::prepareToPlay");
// }

// void AudioPlayer::releaseResources()
// {
//     Logger::writeToLog("AudioPlayer::releaseResources");
// }

// void AudioPlayer::getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill)
// {
//     Logger::writeToLog("AudioPlayer::getNextAudioBlock");
// }
