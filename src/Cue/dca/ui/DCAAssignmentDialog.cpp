/*
  ==============================================================================

    DCAAssignmentDialog.cpp
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "DCAAssignmentDialog.h"
#include "../CharacterRef.h"
#include "../DCACue.h"
#include "../DCAAssignment.h"
#include "../../../Interface/mixer/MixerInterface.h"
#include "../../../Interface/mixer/MixerChannel.h"
#include "../../../Interface/mixer/Character.h"
#include "../../../Interface/mixer/MixerFX.h"

using namespace juce;

namespace
{
    class DarkPanel : public Component
    {
    public:
        void paint(Graphics& g) override { g.fillAll(BG_COLOR.darker(.3f)); }
    };

    class HighlightToggleButton : public ToggleButton
    {
    public:
        HighlightToggleButton(const String& t) : ToggleButton(t) {}
        void paintButton(Graphics& g, bool over, bool down) override
        {
            auto bounds = getLocalBounds().toFloat().reduced(1.0f);
            if (getToggleState() && isEnabled())
            {
                g.setColour(Colours::limegreen.withAlpha(0.35f));
                g.fillRoundedRectangle(bounds, 3.0f);
            }
            else if (isEnabled())
            {
                g.setColour(BG_COLOR.brighter(.06f));
                g.fillRoundedRectangle(bounds, 3.0f);
            }
            ToggleButton::paintButton(g, over, down);
        }
    };
}

DCAAssignmentDialog::DCAAssignmentDialog(DCACue* cue, DCAAssignment* assignment) :
    cue(cue),
    assignment(assignment)
{
    nameLabel = std::make_unique<Label>("nameLabel", "Display name:");
    nameLabel->setColour(Label::textColourId, Colours::lightgrey);
    addAndMakeVisible(nameLabel.get());

    nameEditor = std::make_unique<TextEditor>();
    nameEditor->setText(assignment->displayName->stringValue(), dontSendNotification);
    nameEditor->setTextToShowWhenEmpty("(auto from selected characters)", Colours::grey);
    nameEditor->setColour(TextEditor::backgroundColourId, BG_COLOR.darker(.1f));
    nameEditor->setColour(TextEditor::textColourId, TEXT_COLOR);
    nameEditor->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
    nameEditor->setColour(TextEditor::focusedOutlineColourId, BG_COLOR.brighter(.3f));
    nameEditor->setColour(TextEditor::highlightColourId, BG_COLOR.brighter(.2f));
    nameEditor->setColour(TextEditor::highlightedTextColourId, TEXT_COLOR);
    nameEditor->setColour(CaretComponent::caretColourId, TEXT_COLOR);
    nameEditor->applyColourToAllText(TEXT_COLOR, true);
    nameEditor->addListener(this);
    addAndMakeVisible(nameEditor.get());

    globalFXLabel = std::make_unique<Label>("globalFXLabel", "Apply FX to all selected:");
    globalFXLabel->setColour(Label::textColourId, Colours::lightgrey);
    addAndMakeVisible(globalFXLabel.get());

    globalFXBtn = std::make_unique<TextButton>("-");
    globalFXBtn->setTooltip("Apply the chosen FX (or clear) to every selected character");
    globalFXBtn->setColour(TextButton::buttonColourId, BG_COLOR.brighter(.06f));
    globalFXBtn->setColour(TextButton::textColourOffId, TEXT_COLOR);
    globalFXBtn->addListener(this);
    addAndMakeVisible(globalFXBtn.get());

    charactersLabel = std::make_unique<Label>("charactersLabel", "Characters:");
    charactersLabel->setColour(Label::textColourId, Colours::lightgrey);
    addAndMakeVisible(charactersLabel.get());

    listContent = std::make_unique<DarkPanel>();
    listViewport = std::make_unique<Viewport>();
    listViewport->setViewedComponent(listContent.get(), false);
    listViewport->setScrollBarsShown(true, false);
    addAndMakeVisible(listViewport.get());

    forceFaderBtn = std::make_unique<ToggleButton>("Force fader");
    forceFaderBtn->setColour(ToggleButton::textColourId, TEXT_COLOR);
    forceFaderBtn->setToggleState(assignment->forceFader->boolValue(), dontSendNotification);
    forceFaderBtn->addListener(this);
    addAndMakeVisible(forceFaderBtn.get());

    faderSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::TextBoxLeft);
    faderSlider->setRange(-144.0, 10.0, 0.1);
    faderSlider->setSkewFactorFromMidPoint(-20.0);
    faderSlider->setValue(assignment->faderPosition->floatValue(), dontSendNotification);
    faderSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 60, 22);
    faderSlider->setColour(Slider::backgroundColourId, BG_COLOR.darker(.1f));
    faderSlider->setColour(Slider::trackColourId, Colours::limegreen.withAlpha(0.6f));
    faderSlider->setColour(Slider::thumbColourId, Colours::lightgrey);
    faderSlider->setColour(Slider::textBoxBackgroundColourId, BG_COLOR.darker(.1f));
    faderSlider->setColour(Slider::textBoxTextColourId, TEXT_COLOR);
    faderSlider->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    faderSlider->setTextValueSuffix(" dB");
    faderSlider->setEnabled(assignment->forceFader->boolValue());
    faderSlider->addListener(this);
    addAndMakeVisible(faderSlider.get());

    closeBtn = std::make_unique<TextButton>("Close");
    closeBtn->addListener(this);
    addAndMakeVisible(closeBtn.get());

    rebuildCharacterList();

    setSize(420, 570);
}

DCAAssignmentDialog::~DCAAssignmentDialog()
{
    if (cue != nullptr && assignment != nullptr && assignment->characters->items.isEmpty())
        cue->dcaAssignments->removeItem(assignment);
}

void DCAAssignmentDialog::rebuildCharacterList()
{
    charButtons.clear();
    fxButtons.clear();
    characters.clear();

    MixerInterface* mixer = cue != nullptr ? cue->getMixer() : nullptr;
    if (mixer == nullptr) return;

    // Compute shared FX across selected characters (nullptr = none / mixed)
    MixerFX* sharedFX = nullptr;
    bool hasAny = false, mixed = false;
    for (auto* r : assignment->characters->items)
    {
        MixerFX* fx = r->getFX();
        if (!hasAny) { sharedFX = fx; hasAny = true; }
        else if (sharedFX != fx) { mixed = true; }
    }
    if (globalFXBtn != nullptr)
    {
        String label = "-";
        if (mixed) label = "(mixed)";
        else if (sharedFX != nullptr) label = sharedFX->niceName;
        globalFXBtn->setButtonText(label);
        globalFXBtn->setEnabled(!assignment->characters->items.isEmpty());
    }

    struct Row { Character* character; int channelNum; };
    Array<Row> rows;
    for (auto* ch : mixer->channels->items)
        for (auto* c : ch->characters->items)
            rows.add({ c, ch->channelNumber->intValue() });

    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        if (a.channelNum != b.channelNum) return a.channelNum < b.channelNum;
        return a.character->characterName->stringValue() < b.character->characterName->stringValue();
    });

    for (const auto& row : rows)
    {
        characters.add(row.character);

        String label = "Ch " + String(row.channelNum) + "  -  " + row.character->characterName->stringValue();
        auto* btn = new HighlightToggleButton(label);
        btn->setColour(ToggleButton::textColourId, TEXT_COLOR);
        btn->setToggleState(assignment->hasCharacter(row.character), dontSendNotification);

        // Grey out if another character on the same channel is already in another DCA of this cue
        MixerChannel* rowChannel = row.character->getChannel();
        DCAAssignment* blockingDCA = nullptr;
        if (cue != nullptr && rowChannel != nullptr)
        {
            for (auto* a : cue->dcaAssignments->items)
            {
                if (a == assignment) continue;
                for (auto* r : a->characters->items)
                {
                    if (r->getChannel() == rowChannel) { blockingDCA = a; break; }
                }
                if (blockingDCA != nullptr) break;
            }
        }
        if (blockingDCA != nullptr)
        {
            btn->setEnabled(false);
            btn->setTooltip("Channel already in DCA " + String(blockingDCA->dcaNumber->intValue()));
        }
        else if (rowChannel != nullptr)
        {
            // Within this same DCA: if another character on the same channel is already
            // selected, grey out this row (the selected one itself remains interactable).
            for (auto* r : assignment->characters->items)
            {
                Character* existing = r->getCharacter();
                if (existing == nullptr || existing == row.character) continue;
                if (r->getChannel() == rowChannel)
                {
                    btn->setEnabled(false);
                    btn->setTooltip("Channel " + String(rowChannel->channelNumber->intValue())
                                    + " already represented in this DCA by " + existing->characterName->stringValue());
                    break;
                }
            }
        }

        btn->addListener(this);
        listContent->addAndMakeVisible(btn);
        charButtons.add(btn);

        // FX selector button (enabled only if the character is selected)
        auto* fxBtn = new TextButton();
        CharacterRef* existingRef = nullptr;
        for (auto* r : assignment->characters->items)
            if (r->getCharacter() == row.character) { existingRef = r; break; }

        MixerFX* currentFX = existingRef != nullptr ? existingRef->getFX() : nullptr;
        fxBtn->setButtonText(currentFX != nullptr ? currentFX->niceName : "-");
        fxBtn->setTooltip("Pick an FX for this character");
        fxBtn->setEnabled(existingRef != nullptr);
        fxBtn->setColour(TextButton::buttonColourId, BG_COLOR.brighter(.06f));
        fxBtn->setColour(TextButton::textColourOffId, TEXT_COLOR);
        fxBtn->addListener(this);
        listContent->addAndMakeVisible(fxBtn);
        fxButtons.add(fxBtn);
    }

    const int rowH = 28;
    const int fxWidth = 110;
    listContent->setSize(getWidth() - 20, jmax(1, charButtons.size()) * rowH);
    for (int i = 0; i < charButtons.size(); ++i)
    {
        int w = listContent->getWidth() - 8;
        charButtons[i]->setBounds(4, i * rowH, w - fxWidth - 6, rowH - 2);
        fxButtons[i]->setBounds(4 + w - fxWidth, i * rowH, fxWidth, rowH - 2);
    }
}

void DCAAssignmentDialog::buttonClicked(Button* b)
{
    if (b == closeBtn.get())
    {
        if (auto* w = findParentComponentOfClass<DialogWindow>())
            w->exitModalState(0);
        return;
    }

    if (b == globalFXBtn.get())
    {
        MixerInterface* mixer = cue != nullptr ? cue->getMixer() : nullptr;
        if (mixer == nullptr) return;

        PopupMenu m;
        m.addItem(1, "(none)");
        m.addSeparator();
        for (int i = 0; i < mixer->fxs->items.size(); ++i)
        {
            MixerFX* fx = mixer->fxs->items[i];
            m.addItem(10 + i, fx->niceName + " (bus " + fx->busNumber->stringValue() + ")");
        }

        Component::SafePointer<DCAAssignmentDialog> self(this);
        m.showMenuAsync(PopupMenu::Options().withTargetComponent(globalFXBtn.get()),
            [self, mixer](int result)
            {
                if (self == nullptr || result <= 0) return;
                MixerFX* picked = nullptr;
                if (result >= 10)
                {
                    int idx = result - 10;
                    if (idx >= 0 && idx < mixer->fxs->items.size())
                        picked = mixer->fxs->items[idx];
                }
                for (auto* r : self->assignment->characters->items)
                {
                    if (picked != nullptr) r->fxRef->setValueFromTarget(picked);
                    else r->fxRef->resetValue();
                }
                self->rebuildCharacterList();
                self->resized();
            });
        return;
    }

    // FX selector buttons
    int fxIdx = fxButtons.indexOf(dynamic_cast<TextButton*>(b));
    if (fxIdx >= 0 && fxIdx < characters.size())
    {
        Character* c = characters[fxIdx];
        CharacterRef* ref = nullptr;
        for (auto* r : assignment->characters->items)
            if (r->getCharacter() == c) { ref = r; break; }
        if (ref == nullptr) return;

        MixerInterface* mixer = cue != nullptr ? cue->getMixer() : nullptr;
        if (mixer == nullptr) return;

        PopupMenu m;
        m.addItem(1, "(none)");
        m.addSeparator();
        for (int i = 0; i < mixer->fxs->items.size(); ++i)
        {
            MixerFX* fx = mixer->fxs->items[i];
            bool ticked = (ref->getFX() == fx);
            m.addItem(10 + i, fx->niceName + " (bus " + fx->busNumber->stringValue() + ")", true, ticked);
        }

        Component::SafePointer<DCAAssignmentDialog> self(this);
        auto* fxBtn = fxButtons[fxIdx];
        m.showMenuAsync(PopupMenu::Options().withTargetComponent(fxBtn),
            [self, ref, mixer](int result)
            {
                if (self == nullptr || result <= 0) return;
                if (result == 1)
                {
                    ref->fxRef->resetValue();
                }
                else
                {
                    int idx = result - 10;
                    if (idx >= 0 && idx < mixer->fxs->items.size())
                        ref->fxRef->setValueFromTarget(mixer->fxs->items[idx]);
                }
                self->rebuildCharacterList();
                self->resized();
            });
        return;
    }

    if (b == forceFaderBtn.get())
    {
        bool state = forceFaderBtn->getToggleState();
        assignment->forceFader->setValue(state);
        faderSlider->setEnabled(state);
        return;
    }

    int idx = charButtons.indexOf(dynamic_cast<ToggleButton*>(b));
    if (idx < 0 || idx >= characters.size()) return;

    Character* c = characters[idx];
    bool state = charButtons[idx]->getToggleState();

    if (state) assignment->addCharacter(c);
    else       assignment->removeCharacter(c);

    rebuildCharacterList();
    resized();
}

void DCAAssignmentDialog::textEditorTextChanged(TextEditor& te)
{
    if (&te == nameEditor.get())
        assignment->displayName->setValue(nameEditor->getText());
}

void DCAAssignmentDialog::sliderValueChanged(Slider* slider)
{
    if (slider == faderSlider.get())
        assignment->faderPosition->setValue(slider->getValue());
}

void DCAAssignmentDialog::paint(Graphics& g)
{
    g.fillAll(BG_COLOR);
}

void DCAAssignmentDialog::resized()
{
    auto r = getLocalBounds().reduced(10);

    nameLabel->setBounds(r.removeFromTop(18));
    r.removeFromTop(4);
    nameEditor->setBounds(r.removeFromTop(26));
    r.removeFromTop(10);

    auto fxRow = r.removeFromTop(26);
    globalFXLabel->setBounds(fxRow.removeFromLeft(180));
    globalFXBtn->setBounds(fxRow.reduced(4, 0));
    r.removeFromTop(10);

    auto faderRow = r.removeFromTop(26);
    forceFaderBtn->setBounds(faderRow.removeFromLeft(130));
    faderSlider->setBounds(faderRow.reduced(4, 0));
    r.removeFromTop(10);

    charactersLabel->setBounds(r.removeFromTop(18));
    r.removeFromTop(4);

    auto bottom = r.removeFromBottom(32);
    closeBtn->setBounds(bottom.removeFromRight(80));

    r.removeFromBottom(6);
    listViewport->setBounds(r);

    const int rowH = 28;
    const int fxWidth = 110;
    listContent->setSize(listViewport->getWidth() - listViewport->getScrollBarThickness(),
                         jmax(1, charButtons.size()) * rowH);
    for (int i = 0; i < charButtons.size(); ++i)
    {
        int w = listContent->getWidth() - 8;
        charButtons[i]->setBounds(4, i * rowH, w - fxWidth - 6, rowH - 2);
        if (i < fxButtons.size())
            fxButtons[i]->setBounds(4 + w - fxWidth, i * rowH, fxWidth, rowH - 2);
    }
}
