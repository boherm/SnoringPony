/*
  ==============================================================================

    MTCEncoder.h
    Created: 20 Apr 2026
    Author:  boherm

    Utility class for encoding MIDI Time Code (MTC) messages.
    Supports Full Frame (SysEx) and Quarter Frame messages.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class MTCEncoder
{
public:
    enum FrameRate
    {
        FPS_24 = 0,
        FPS_25 = 1,
        FPS_30_DROP = 2,
        FPS_30 = 3
    };

    struct SMPTETime
    {
        int hours = 0;
        int minutes = 0;
        int seconds = 0;
        int frames = 0;
    };

    static int getFramesPerSecond(FrameRate rate)
    {
        switch (rate)
        {
            case FPS_24:      return 24;
            case FPS_25:      return 25;
            case FPS_30_DROP: return 30;
            case FPS_30:      return 30;
            default:          return 25;
        }
    }

    static SMPTETime secondsToSMPTE(double timeInSeconds, FrameRate rate)
    {
        SMPTETime smpte;
        if (timeInSeconds < 0.0) timeInSeconds = 0.0;

        int fps = getFramesPerSecond(rate);
        int totalFrames = (int)(timeInSeconds * fps);

        smpte.frames  = totalFrames % fps;
        int totalSecs  = totalFrames / fps;
        smpte.seconds  = totalSecs % 60;
        int totalMins  = totalSecs / 60;
        smpte.minutes  = totalMins % 60;
        smpte.hours    = totalMins / 60;

        if (smpte.hours > 23) smpte.hours = 23;

        return smpte;
    }

    /** Full Frame SysEx: F0 7F 7F 01 01 hr mn sc fr F7
        hr encodes frame rate in bits 5-6. */
    static juce::MidiMessage createFullFrameMessage(double timeInSeconds, FrameRate rate)
    {
        SMPTETime t = secondsToSMPTE(timeInSeconds, rate);

        juce::uint8 rateBits = ((juce::uint8)rate & 0x03) << 5;
        juce::uint8 hr = rateBits | ((juce::uint8)t.hours & 0x1F);

        juce::uint8 data[] = {
            0xF0, 0x7F, 0x7F, 0x01, 0x01,
            hr,
            (juce::uint8)(t.minutes & 0x3F),
            (juce::uint8)(t.seconds & 0x3F),
            (juce::uint8)(t.frames & 0x1F),
            0xF7
        };

        return juce::MidiMessage(data, 10);
    }

    /** Quarter Frame message: status 0xF1, data = (piece << 4) | nibble.
        piece 0: frame  low nibble
        piece 1: frame  high nibble
        piece 2: seconds low nibble
        piece 3: seconds high nibble
        piece 4: minutes low nibble
        piece 5: minutes high nibble
        piece 6: hours   low nibble
        piece 7: hours   high nibble + rate bits */
    static juce::MidiMessage createQuarterFrameMessage(int piece, double timeInSeconds, FrameRate rate)
    {
        SMPTETime t = secondsToSMPTE(timeInSeconds, rate);

        juce::uint8 nibble = 0;

        switch (piece)
        {
            case 0: nibble = (juce::uint8)(t.frames & 0x0F);  break;
            case 1: nibble = (juce::uint8)((t.frames >> 4) & 0x01); break;
            case 2: nibble = (juce::uint8)(t.seconds & 0x0F); break;
            case 3: nibble = (juce::uint8)((t.seconds >> 4) & 0x03); break;
            case 4: nibble = (juce::uint8)(t.minutes & 0x0F); break;
            case 5: nibble = (juce::uint8)((t.minutes >> 4) & 0x03); break;
            case 6: nibble = (juce::uint8)(t.hours & 0x0F);   break;
            case 7: nibble = (juce::uint8)(((t.hours >> 4) & 0x01) | (((juce::uint8)rate & 0x03) << 1)); break;
            default: break;
        }

        juce::uint8 dataByte = (juce::uint8)((piece << 4) | (nibble & 0x0F));
        return juce::MidiMessage::quarterFrame(piece, nibble);
    }

    /** Time interval between quarter-frame messages in seconds.
        There are 4 quarter-frames per frame. */
    static double getQuarterFrameInterval(FrameRate rate)
    {
        return 1.0 / (getFramesPerSecond(rate) * 4.0);
    }
};
