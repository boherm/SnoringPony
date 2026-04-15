/*
  ==============================================================================

    DCAAssignmentDialog.cpp
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "DCAAssignmentDialog.h"
#include "../DCACue.h"
#include "../DCAAssignment.h"
#include "../../../Interface/mixer/MixerInterface.h"
#include "../../../Interface/mixer/MixerChannel.h"
#include "../../../Interface/mixer/Character.h"

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

    charactersLabel = std::make_unique<Label>("charactersLabel", "Characters:");
    charactersLabel->setColour(Label::textColourId, Colours::lightgrey);
    addAndMakeVisible(charactersLabel.get());

    listContent = std::make_unique<DarkPanel>();
    listViewport = std::make_unique<Viewport>();
    listViewport->setViewedComponent(listContent.get(), false);
    listViewport->setScrollBarsShown(true, false);
    addAndMakeVisible(listViewport.get());

    closeBtn = std::make_unique<TextButton>("Close");
    closeBtn->addListener(this);
    addAndMakeVisible(closeBtn.get());

    rebuildCharacterList();

    setSize(420, 520);
}

DCAAssignmentDialog::~DCAAssignmentDialog()
{
    if (cue != nullptr && assignment != nullptr && assignment->characters->items.isEmpty())
        cue->dcaAssignments->removeItem(assignment);
}

void DCAAssignmentDialog::rebuildCharacterList()
{
    charButtons.clear();
    characters.clear();

    MixerInterface* mixer = cue != nullptr ? cue->getMixer() : nullptr;
    if (mixer == nullptr) return;

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

        String label = "Ch " + String(row.channelNum) + "  —  " + row.character->characterName->stringValue();
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
    }

    const int rowH = 28;
    listContent->setSize(getWidth() - 20, jmax(1, charButtons.size()) * rowH);
    for (int i = 0; i < charButtons.size(); ++i)
        charButtons[i]->setBounds(4, i * rowH, listContent->getWidth() - 8, rowH - 2);
}

void DCAAssignmentDialog::buttonClicked(Button* b)
{
    if (b == closeBtn.get())
    {
        if (auto* w = findParentComponentOfClass<DialogWindow>())
            w->exitModalState(0);
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

void DCAAssignmentDialog::paint(Graphics& g)
{
    g.fillAll(BG_COLOR);
}

void DCAAssignmentDialog::resized()
{
    auto r = getLocalBounds().reduced(10);

    nameLabel->setBounds(r.removeFromTop(18));
    nameEditor->setBounds(r.removeFromTop(26));
    r.removeFromTop(10);

    charactersLabel->setBounds(r.removeFromTop(18));
    r.removeFromTop(4);

    auto bottom = r.removeFromBottom(32);
    closeBtn->setBounds(bottom.removeFromRight(80));

    r.removeFromBottom(6);
    listViewport->setBounds(r);

    const int rowH = 28;
    listContent->setSize(listViewport->getWidth() - listViewport->getScrollBarThickness(),
                         jmax(1, charButtons.size()) * rowH);
    for (int i = 0; i < charButtons.size(); ++i)
        charButtons[i]->setBounds(4, i * rowH, listContent->getWidth() - 8, rowH - 2);
}
