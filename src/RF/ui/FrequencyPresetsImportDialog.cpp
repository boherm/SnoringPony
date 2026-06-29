/*
  ==============================================================================

    FrequencyPresetsImportDialog.cpp
    Created: 29 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "FrequencyPresetsImportDialog.h"

using namespace juce;

namespace
{
    // TextEditor whose "empty" placeholder supports multiple lines (JUCE's built-in
    // setTextToShowWhenEmpty only renders a single line).
    class PlaceholderTextEditor : public TextEditor
    {
    public:
        StringArray placeholderLines;
        Colour placeholderColour = Colours::grey;

        void paintOverChildren(Graphics& g) override
        {
            TextEditor::paintOverChildren(g);

            if (!getText().isEmpty() || placeholderLines.isEmpty())
                return;

            g.setColour(placeholderColour);
            g.setFont(getFont());

            const float lineH = getFont().getHeight() * 1.35f;
            auto area = getLocalBounds().reduced(6, 4);
            float y = (float)area.getY();

            for (const auto& line : placeholderLines)
            {
                g.drawText(line, area.getX(), (int)y, area.getWidth(), (int)lineH,
                           Justification::topLeft, true);
                y += lineH;
            }
        }
    };
}

FrequencyPresetsImportDialog::FrequencyPresetsImportDialog(BaseManager<FrequencyPreset>* presets) :
    presets(presets)
{
    auto* ed = new PlaceholderTextEditor();
    ed->placeholderColour = Colours::grey;
    ed->placeholderLines.addLines(
        "One frequency per line, in MHz (kHz / Hz also understood).\n"
        "An optional label may precede the value, e.g. \"Vocal 1  470.125\".\n"
        "\n"
        "470.125\n"
        "471.300\n"
        "606,300 MHz");
    textArea.reset(ed);
    textArea->setMultiLine(true, true);
    textArea->setReturnKeyStartsNewLine(true);
    textArea->setScrollbarsShown(true);
    textArea->setColour(TextEditor::backgroundColourId, BG_COLOR.darker(.1f));
    textArea->setColour(TextEditor::textColourId, TEXT_COLOR);
    textArea->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
    textArea->setColour(TextEditor::focusedOutlineColourId, BG_COLOR.brighter(.3f));
    textArea->setColour(TextEditor::highlightColourId, BG_COLOR.brighter(.2f));
    textArea->setColour(TextEditor::highlightedTextColourId, TEXT_COLOR);
    textArea->setColour(CaretComponent::caretColourId, TEXT_COLOR);
    textArea->applyColourToAllText(TEXT_COLOR, true);
    addAndMakeVisible(textArea.get());

    replaceBtn = std::make_unique<ToggleButton>("Replace existing presets");
    replaceBtn->setColour(ToggleButton::textColourId, TEXT_COLOR);
    addAndMakeVisible(replaceBtn.get());

    resultLabel = std::make_unique<Label>("result", "");
    resultLabel->setColour(Label::textColourId, Colours::orange);
    addAndMakeVisible(resultLabel.get());

    importBtn = std::make_unique<TextButton>("Import");
    importBtn->addListener(this);
    addAndMakeVisible(importBtn.get());

    cancelBtn = std::make_unique<TextButton>("Cancel");
    cancelBtn->addListener(this);
    addAndMakeVisible(cancelBtn.get());

    setSize(560, 640);
}

FrequencyPresetsImportDialog::~FrequencyPresetsImportDialog()
{
}

void FrequencyPresetsImportDialog::show(BaseManager<FrequencyPreset>* presets)
{
    if (presets == nullptr) return;

    DialogWindow::LaunchOptions dw;
    dw.content.setOwned(new FrequencyPresetsImportDialog(presets));
    dw.dialogTitle = "Import frequency presets";
    dw.dialogBackgroundColour = BG_COLOR;
    dw.escapeKeyTriggersCloseButton = true;
    dw.useNativeTitleBar = false;
    dw.resizable = false;
    dw.launchAsync();
}

bool FrequencyPresetsImportDialog::parseLine(const String& rawLine, double& outMHz, String& outName)
{
    const String line = rawLine.trim();
    if (line.isEmpty()) return false;

    // Accept comma as a decimal separator (same length, so indices stay valid).
    const String norm = line.replaceCharacter(',', '.');

    // Collect every numeric token (digits and a single dot) with its position.
    struct Token { int start, end; bool hasDot; double value; };
    Array<Token> tokens;
    int i = 0;
    while (i < norm.length())
    {
        const juce_wchar c = norm[i];
        if ((c >= '0' && c <= '9') || c == '.')
        {
            int s = i;
            bool dot = false;
            while (i < norm.length())
            {
                const juce_wchar cc = norm[i];
                if (cc == '.') dot = true;
                else if (cc < '0' || cc > '9') break;
                ++i;
            }
            tokens.add({ s, i, dot, norm.substring(s, i).getDoubleValue() });
        }
        else ++i;
    }
    if (tokens.isEmpty()) return false;

    // The frequency is the most "frequency-looking" token: prefer one with a
    // decimal point (labels like "Vocal 1" carry a bare integer), then the
    // largest value. This keeps "Vocal 1  470.125" from importing 1 MHz.
    int best = 0;
    for (int t = 1; t < tokens.size(); ++t)
    {
        const auto& a = tokens.getReference(t);
        const auto& b = tokens.getReference(best);
        if ((a.hasDot && !b.hasDot) || (a.hasDot == b.hasDot && a.value > b.value))
            best = t;
    }

    const Token tok = tokens.getReference(best);
    double value = tok.value;
    if (value <= 0.0) return false;

    // Unit handling: explicit suffix wins, otherwise guess from magnitude
    // (mic lists are sometimes exported in kHz/Hz without a unit).
    const String lower = line.toLowerCase();
    if (lower.contains("khz"))      value /= 1000.0;
    else if (lower.contains("mhz")) { /* already MHz */ }
    else if (lower.contains("hz"))  value /= 1.0e6;
    else while (value > 6000.0)     value /= 1000.0;

    if (value < 1.0 || value > 6000.0) return false;
    outMHz = value;

    // Name: the rest of the line once the frequency token is removed, with any
    // unit word and leading/trailing separators stripped.
    String name = (line.substring(0, tok.start) + line.substring(tok.end)).trim();
    name = name.replace("MHz", "", true).replace("kHz", "", true).replace("Hz", "", true);
    name = name.trim().trimCharactersAtStart(":;,-").trimCharactersAtEnd(":;,-").trim();
    outName = name;
    return true;
}

void FrequencyPresetsImportDialog::doImport()
{
    if (presets == nullptr) return;

    StringArray lines;
    lines.addLines(textArea->getText());

    Array<FrequencyPreset*> toAdd;
    int skipped = 0;

    for (const auto& l : lines)
    {
        if (l.trim().isEmpty()) continue;

        double mhz = 0.0;
        String name;
        if (!parseLine(l, mhz, name))
        {
            ++skipped;
            continue;
        }

        auto* p = new FrequencyPreset();
        // No label on the line -> let the manager assign a unique "Preset N" name.
        if (name.isNotEmpty())
            p->setNiceName(name);
        else
            p->setNiceName("Preset 1");
        p->freqMHz->setValue(mhz);
        toAdd.add(p);
    }

    if (toAdd.isEmpty())
    {
        resultLabel->setText(skipped > 0 ? "No valid frequency found." : "Nothing to import.",
                             dontSendNotification);
        return;
    }

    if (replaceBtn->getToggleState())
        presets->clear();

    presets->addItems(toAdd);

    if (auto* w = findParentComponentOfClass<DialogWindow>())
        w->exitModalState(0);
}

void FrequencyPresetsImportDialog::buttonClicked(Button* b)
{
    if (b == importBtn.get())
    {
        doImport();
    }
    else if (b == cancelBtn.get())
    {
        if (auto* w = findParentComponentOfClass<DialogWindow>())
            w->exitModalState(0);
    }
}

void FrequencyPresetsImportDialog::paint(Graphics& g)
{
    g.fillAll(BG_COLOR);
}

void FrequencyPresetsImportDialog::resized()
{
    auto r = getLocalBounds().reduced(10);

    auto bottom = r.removeFromBottom(32);
    cancelBtn->setBounds(bottom.removeFromRight(80));
    bottom.removeFromRight(6);
    importBtn->setBounds(bottom.removeFromRight(80));

    r.removeFromBottom(6);
    replaceBtn->setBounds(r.removeFromBottom(24));
    resultLabel->setBounds(r.removeFromBottom(20));
    r.removeFromBottom(6);

    textArea->setBounds(r);
}
