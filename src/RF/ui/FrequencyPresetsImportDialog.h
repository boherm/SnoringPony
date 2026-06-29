/*
  ==============================================================================

    FrequencyPresetsImportDialog.h
    Created: 29 Jun 2026
    Author:  boherm

    Modal dialog to bulk-import frequency presets into a profile: paste one
    frequency per line in the textarea and they are added as Presets. Each line
    holds a frequency in MHz (kHz/Hz units are also understood) with an optional
    label, e.g. "Vocal 1  470.125" or "606,300 MHz".

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../FrequencyPreset.h"

class FrequencyPresetsImportDialog :
    public juce::Component,
    public juce::Button::Listener
{
public:
    FrequencyPresetsImportDialog(BaseManager<FrequencyPreset>* presets);
    ~FrequencyPresetsImportDialog() override;

    // Launches the dialog as a standalone window over the app.
    static void show(BaseManager<FrequencyPreset>* presets);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void buttonClicked(juce::Button* b) override;

private:
    BaseManager<FrequencyPreset>* presets = nullptr;

    std::unique_ptr<juce::TextEditor> textArea;
    std::unique_ptr<juce::ToggleButton> replaceBtn;
    std::unique_ptr<juce::Label> resultLabel;
    std::unique_ptr<juce::TextButton> importBtn;
    std::unique_ptr<juce::TextButton> cancelBtn;

    // Parses one line into a frequency (MHz) and an optional preset name.
    static bool parseLine(const juce::String& rawLine, double& outMHz, juce::String& outName);

    void doImport();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyPresetsImportDialog)
};
