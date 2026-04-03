/*
  ==============================================================================

    AudioWaveformSlicerEditor.cpp
    Created: 22 Dec 2025 04:26:23am
    Author:  boherm

  ==============================================================================
*/


#include "AudioWaveformSlicerEditor.h"
#include "../AudioCue.h"
#include "../AudioSlices.h"
#include "../AudioFile.h"
#include "../AudioWaveformSlicer.h"
#include "juce_gui_basics/juce_gui_basics.h"


AudioWaveform::AudioWaveform(AudioCue* audioCue) :
    thumbnailCache(5),
    thumbnail(512, formatManager, thumbnailCache)
{
    startTimer(50);
    this->audioCue = audioCue;
    // setWantsKeyboardFocus(true);
    formatManager.registerBasicFormats();
    thumbnail.addChangeListener(this);
    this->audioCue->slicesManager->addAsyncContainerListener(this);
    this->audioCue->filesManager->addAsyncContainerListener(this);

    zoomLevel = 1;
}

AudioWaveform::~AudioWaveform()
{
    stopTimer();
    this->audioCue->slicesManager->removeAsyncContainerListener(this);
    this->audioCue->filesManager->removeAsyncContainerListener(this);
}

void AudioWaveform::paint(Graphics& g)
{
    juce::Rectangle<float> thumbnailBounds(0, 0, getParentWidth(), getParentHeight());

    if (thumbnail.getNumChannels() == 0) {
        g.setColour(BG_COLOR);
        g.fillRect(thumbnailBounds);
        g.setColour(TEXT_COLOR);
        g.drawFittedText("No File Loaded", thumbnailBounds.toType<int>(), juce::Justification::centred, 1);
    } else {

        g.fillAll(BG_COLOR);

        if (thumbnail.getTotalLength() > 0.0)
        {
            visibleStartTime = 0.0f;
            visibleEndTime = thumbnail.getTotalLength();

            g.setColour (TEXT_COLOR.darker(0.3f));

            juce::Rectangle<int> thumbnailBounds2(0, 20, getParentWidth() * zoomLevel, getHeight() - 40);
            thumbnail.drawChannel(g, thumbnailBounds2.toType<int>(), visibleStartTime, visibleEndTime, 0, 1.0f);
            thumbnail.drawChannel(g, thumbnailBounds2.toType<int>(), visibleStartTime, visibleEndTime, 1, 1.0f);

            // Draw bounds with start / end
            const float xStart = timeToX(audioCue->slicesManager->startTime->doubleValue(), Rectangle<float>(0, 0, getParentWidth()*zoomLevel, getHeight()));
            const float xEnd = timeToX(audioCue->slicesManager->endTime->doubleValue(), Rectangle<float>(0, 0, getParentWidth()*zoomLevel, getHeight()));
            g.setColour(Colours::black.withAlpha(0.6f));
            g.fillRect(0.0f, 0.0f, jmax(0.0f, xStart), 150.0f);
            if (xEnd < getParentWidth()*zoomLevel)
                g.fillRect(xEnd, 0.0f, jmin((float)getParentWidth()*zoomLevel, (float)(getParentWidth()*zoomLevel) - xEnd), 150.0f);

            // Draw slices
            drawSlices(g);

            // overlay: time slices
            juce::Rectangle<int> thumbnailBounds3(0, 0, getParentWidth() * zoomLevel, getParentHeight());
            drawTimeSlices(g, thumbnailBounds3.toFloat());

            // if (selectedX > 0.0f) {
            //     g.setColour(Colours::red);
            //     g.drawLine(selectedX, 0, selectedX, 150, 1.0f);
            // }

            if (isMouseHovering) {
                g.setColour(HIGHLIGHT_COLOR);
                g.drawLine(mouseX, 0, mouseX, 150, 1.0f);
            }

            drawCurrentTime(g);

            // if (shiftSelection) {
            //     g.setColour(Colours::yellow.withAlpha(0.5f));
            //     float xBegin;
            //     float xEnd;

            //     if (mouseX < startSelectionX) {
            //         xBegin = startSelectionX;
            //         xEnd = mouseX;
            //     } else {
            //         xBegin = mouseX;
            //         xEnd = startSelectionX;
            //     }
            //     g.fillRect(xEnd, 0.0f, xBegin - xEnd, 150.0f);
            // }
        }
    }
}

void AudioWaveform::loadFile(File file)
{
    thumbnail.setSource(new juce::FileInputSource(file));
}

void AudioWaveform::drawSlices(Graphics& g)
{
    for (int i = 0 ; i < audioCue->slicesManager->items.size() ; i++)
    {
        AudioSlice* slice = audioCue->slicesManager->items.getUnchecked(i);
        const float xBegin = timeToX(slice->startTime->doubleValue(), Rectangle<float>(0, 0, getParentWidth()*zoomLevel, getHeight()));
        const float xEnd = timeToX(slice->endTime->doubleValue(), Rectangle<float>(0, 0, getParentWidth()*zoomLevel, getHeight()));

        // Wave area
        const Rectangle<float> area(xBegin, 0, xEnd - xBegin, 150);
        g.setColour(slice->itemColor->getColor());
        g.drawLine(xBegin, 0, xBegin, 150, 1.0f);
        g.drawLine(xEnd, 0, xEnd, 150, 1.0f);
        g.setColour(slice->itemColor->getColor().withAlpha(0.3f));
        g.fillRect(area);

        // Label area
        const Rectangle<float> areaText(xBegin, 130, xEnd - xBegin, 20);
        g.setColour(slice->itemColor->getColor().withAlpha(0.7f));
        g.fillRect(areaText);
        g.setColour(Colours::white.withAlpha(0.9f));
        g.setFont(juce::Font("Arial", 16.0f, juce::Font::plain));

        if (!slice->isEnabled())
            g.setColour(Colours::grey.withAlpha(0.7f));

        g.drawText(
            slice->niceName + " - " +
            (slice->loopSlice->boolValue() ? "looped" : slice->repetitions->stringValue() + " times"),
            areaText.toType<int>(),
            Justification::centred,
            true
        );
    }
}

void AudioWaveform::drawCurrentTime(Graphics& g)
{
    if (!audioCue->filesManager->haveOnePlaying())
        return;

    double time = audioCue->filesManager->getCurrentTime();

    const float x = timeToX(time, Rectangle<float>(0, 0, getParentWidth()*zoomLevel, getHeight()));

    g.setColour(Colours::green);
    g.drawLine(x, 0, x, 150, 2.0f);
}

void AudioWaveform::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &thumbnail)
        repaint();
}

void AudioWaveform::resized()
{
    setSize(getParentWidth() * zoomLevel, 150);
    repaint();
}

float AudioWaveform::timeToX(double t, Rectangle<float> area)
{
    const double span = visibleEndTime - visibleStartTime;
    if (span <= 0.0) return area.getX();

    const double norm = (t - visibleStartTime) / span;
    return (float) (area.getX() + norm * area.getWidth());
}

double AudioWaveform::chooseGridStepSeconds(double visibleSpanSeconds, int pixelWidth)
{
    const double targetLines = jmax (1.0, pixelWidth / 80.0);
    const double raw = visibleSpanSeconds / targetLines;

    const double baseSteps[] = { 0.1, 0.2, 0.5, 1, 2, 5, 10, 15, 30, 60, 120, 300, 600, 900, 1800, 3600 };

    for (double s : baseSteps)
        if (raw <= s) return s;

    return 7200.0; // 2h
}

String AudioWaveform::formatTime(double seconds)
{
    seconds = jmax (0.0, seconds);
    const int total = (int) std::floor (seconds + 1e-9);

    const int hh = total / 3600;
    const int mm = (total / 60) % 60;
    const int ss = total % 60;

    if (hh > 0)
        return String::formatted ("%d:%02d:%02d", hh, mm, ss);

    return String::formatted ("%d:%02d", mm, ss);
}

void AudioWaveform::drawTimeSlices(Graphics& g, Rectangle<float> area)
{
    const double span = visibleEndTime - visibleStartTime;
    if (span <= 0.0) return;

    const double step = chooseGridStepSeconds (span, (int) area.getWidth());

    // aligne sur un multiple de step
    double t0 = std::floor (visibleStartTime / step) * step;
    if (t0 < visibleStartTime) t0 += step;

    g.setFont (12.0f);

    for (double t = t0; t <= visibleEndTime + 1e-9; t += step)
    {
        const float x = timeToX (t, area);

        // ligne
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawLine (x, area.getY(), x, area.getBottom(), 1.0f);

        // label (en haut)
        const auto label = formatTime (t);
        g.setColour (juce::Colours::white.withAlpha (0.8f));

        const int textW = 60;
        const int textH = 16;

        juce::Rectangle<int> textBox ((int) x + 4, (int) area.getY() + 2, textW, textH);
        g.drawFittedText (label, textBox, juce::Justification::left, 1);
    }
}

// void AudioWaveform::mouseEnter(const MouseEvent& event)
// {
//     // isMouseHovering = true;
// }

// void AudioWaveform::mouseExit(const MouseEvent& event)
// {
//     isMouseHovering = false;
// }

void AudioWaveform::mouseMove(const MouseEvent& event)
{
    mouseX = event.position.x;

    if (!mouseDragging) {
        parameterResizing = nullptr;
        setMouseCursor(MouseCursor::CrosshairCursor);
        Rectangle<float> area(0, 0, getParentWidth()*zoomLevel, getHeight());

        for (int i = 0 ; i < audioCue->slicesManager->items.size() ; i++)
        {
            AudioSlice* slice = audioCue->slicesManager->items.getUnchecked(i);
            const float xBegin = timeToX(slice->startTime->doubleValue(), area);
            const float xEnd = timeToX(slice->endTime->doubleValue(), area);

            if (mouseX >= xBegin - 5.0f && mouseX <= xBegin + 5.0f) {
                parameterResizing = slice->startTime;
                setMouseCursor(MouseCursor::LeftRightResizeCursor);
            }

            if (mouseX >= xEnd - 5.0f && mouseX <= xEnd + 5.0f) {
                parameterResizing = slice->endTime;
                setMouseCursor(MouseCursor::LeftRightResizeCursor);
            }
        }

        // SlicesManager start/end
        if (parameterResizing == nullptr) {
            const float xBegin = timeToX(audioCue->slicesManager->startTime->doubleValue(), area);
            const float xEnd = timeToX(audioCue->slicesManager->endTime->doubleValue(), area);

            if (mouseX >= xBegin - 5.0f && mouseX <= xBegin + 5.0f) {
                parameterResizing = audioCue->slicesManager->startTime;
                setMouseCursor(MouseCursor::LeftRightResizeCursor);
            }

            if (mouseX >= xEnd - 5.0f && mouseX <= xEnd + 5.0f) {
                parameterResizing = audioCue->slicesManager->endTime;
                setMouseCursor(MouseCursor::LeftRightResizeCursor);
            }
        }
    }

    repaint();
}

void AudioWaveform::mouseDrag(const MouseEvent& event)
{
    // shiftSelection = true;

    if (parameterResizing != nullptr) {
        Rectangle<float> area(0, 0, getWidth(), getHeight());
        double timeAtMouse = visibleStartTime + (event.position.x / area.getWidth()) * (visibleEndTime - visibleStartTime);

        parameterResizing->setValue(timeAtMouse);
    }


    mouseMove(event);
}

void AudioWaveform::mouseUp(const MouseEvent& event)
{
    // shiftSelection = false;
    // startSelectionX = 0.0f;
    mouseDragging = false;
    repaint();
}

void AudioWaveform::mouseDown(const MouseEvent& event)
{
    // selectedX = event.position.x;
    // startSelectionX = event.position.x;
    mouseDragging = true;
    repaint();

    if (event.mods.isRightButtonDown()) {
        PopupMenu menu;
        menu.addItem(1, "Set start here");
        menu.addItem(2, "Set end here");

        menu.showMenuAsync(PopupMenu::Options(),
            [this, event](int result)
            {
                Rectangle<float> area(0, 0, getWidth(), getHeight());
                double timeAtMouse = visibleStartTime + (event.position.x / area.getWidth()) * (visibleEndTime - visibleStartTime);

                if (result == 1) {
                    audioCue->slicesManager->startTime->setValue(timeAtMouse);
                }

                if (result == 2) {
                    audioCue->slicesManager->endTime->setValue(timeAtMouse);
                }
            }
        );

    } else if (audioCue->filesManager->haveOnePlaying() && audioCue->isPreviewing) {
        Rectangle<float> area(0, 0, getWidth(), getHeight());
        double timeAtMouse = visibleStartTime + (event.position.x / area.getWidth()) * (visibleEndTime - visibleStartTime);
        audioCue->filesManager->setCurrentTime(jmax(timeAtMouse, audioCue->slicesManager->startTime->doubleValue()));
    }
}

void AudioWaveform::modifierKeysChanged(const ModifierKeys& modifiers)
{
    // if (modifiers.isShiftDown()) {
    //     shiftSelection = true;
    //     startSelectionX = mouseX;
    // }

    // if (!modifiers.isShiftDown()) {
    //     shiftSelection = false;
    //     startSelectionX = 0.0f;
    // }
}

void AudioWaveform::newMessage(const ContainerAsyncEvent& e)
{
    if (e.source == audioCue->filesManager
        && e.type == ContainerAsyncEvent::EventType::ControllableFeedbackUpdate
        && e.targetControllable->niceName == "Audio File")
    {
        File f = audioCue->filesManager->items.getFirst()->audioFile->getFile();
        if (f.existsAsFile())
            loadFile(f);
    }

    repaint();
}

void AudioWaveform::timerCallback()
{
    repaint();
}

// void AudioWaveform::mouseMagnify(const MouseEvent& event, float scaleFactor)
// {
//     zoomLevel = jmax(1, jmin(8, (int)(zoomLevel * abs(scaleFactor * 0.5f))));
//     Logger::writeToLog("Mouse magnify: " + String(scaleFactor));
//     Logger::writeToLog("zoomLevel: " + String(zoomLevel));
//     resized();
// }
//

// ------------------------------------------------------------------------------

AudioWaveformSlicerEditor::AudioWaveformSlicerEditor(Array<ControllableContainer*> containers, bool isRoot) :
    GenericControllableContainerEditor(containers, isRoot),
    viewport()
{
    audioWaveformContainer = dynamic_cast<AudioWaveformSlicer*>(container.get());
    auto* audioCue = dynamic_cast<AudioCue*>(audioWaveformContainer->audioCue);
    waveformC.reset(new AudioWaveform(audioCue));
    viewport.setViewedComponent(waveformC.get(), false);
    viewport.setScrollBarsShown(false, false, false, true);
    viewport.setScrollOnDragMode(Viewport::ScrollOnDragMode::never);
    addAndMakeVisible(viewport);

    zoomInUI = audioWaveformContainer->zoomInBtn->createButtonUI();
    addAndMakeVisible(zoomInUI);

    zoomOutUI = audioWaveformContainer->zoomOutBtn->createButtonUI();
    addAndMakeVisible(zoomOutUI);

    previewUI = audioCue->previewBtn->createButtonUI();
    previewUI->customBGColor = Colours::darkgreen;
    previewUI->useCustomBGColor = true;
    addAndMakeVisible(previewUI);

    refreshThumbnail();
    setSize(getWidth(), 144);
}

AudioWaveformSlicerEditor::~AudioWaveformSlicerEditor()
{
    delete zoomInUI;
    delete zoomOutUI;
    delete previewUI;
}

void AudioWaveformSlicerEditor::refreshThumbnail()
{
    File file = audioWaveformContainer->audioCue->filesManager->items.getFirst()->audioFile->getFile();

    if (file.existsAsFile())
    {
        waveformC->loadFile(file);
    }
}

void AudioWaveformSlicerEditor::resizedInternalHeader(Rectangle<int>& r)
{
    zoomOutUI->setBounds(r.removeFromRight(30).reduced(2));
    zoomInUI->setBounds(r.removeFromRight(30).reduced(2));

    r.removeFromRight(10);

    previewUI->setBounds(r.removeFromRight(50).reduced(2));
    GenericControllableContainerEditor::resizedInternalHeader(r);
}

void AudioWaveformSlicerEditor::resizedInternalContent(Rectangle<int>& r)
{
    viewport.setBounds(1, headerHeight, r.getWidth() + 5, 170);
    viewport.getViewedComponent()->resized();
    // r.translate(0, 144);
    r.translate(0, 160);
}

void AudioWaveformSlicerEditor::zoomIn()
{
    waveformC->zoomLevel = jmin(8, waveformC->zoomLevel + 1);
    waveformC->resized();
}

void AudioWaveformSlicerEditor::zoomOut()
{
    waveformC->zoomLevel = jmax(1, waveformC->zoomLevel - 1);
    waveformC->resized();
}
