/*
  ==============================================================================

    ShowInfos.h
    Created: 4 Nov 2025 10:50:23am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class Cuelist;
class Cue;

class ShowInfosUI : public ShapeShifterContent {
public:
    ShowInfosUI(const String &contentName);
    ~ShowInfosUI();

    static ShowInfosUI *create(const String &name) { return new ShowInfosUI(name); }
};

class ShowInfos :
    public ControllableContainer,
    public Component
{
public:
    juce_DeclareSingleton(ShowInfos, true);
    ShowInfos();
    ~ShowInfos() override;

    Cuelist* mainCuelist = nullptr;
    Cue* currentCue = nullptr;
    Cue* nextCue = nullptr;

    void paint (Graphics&) override;
    void resized() override;

    void parameterValueChanged(Parameter* p) override;

private:
    // Scrollable notes of the next cue: a child component drawn at the text's
    // full height, wrapped in a Viewport so only this area scrolls.
    class NotesView : public Component {
    public:
        void setText(const String& t);
        int getPreferredHeight(int width) const;
        void paint(Graphics& g) override;
    private:
        String text;
        AttributedString makeString() const;
    };

    static constexpr int lineHeight  = 26; // height of each cue line
    static constexpr int notesGap    = 5; // gap before the notes block
    static constexpr int notesIndent = 20; // left indent of the notes

    NotesView notesView;
    Viewport  notesViewport; // declared after notesView so it is destroyed first

    void updateNotes(); // push the next cue's notes into the view + relayout
    void layoutNotes(); // size the inner view for the current viewport width

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShowInfos)
};
