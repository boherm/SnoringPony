/*
  ==============================================================================

    OSCInputEditor.cpp
    Created: 9 Dec 2025 11:07:20pm
    Author:  boherm

  ==============================================================================
*/

#include "OSCInputEditor.h"

OSCInputEditor::OSCInputEditor(Array<ControllableContainer*> cc, bool isRoot) :
    EnablingControllableContainerEditor(Inspectable::getArrayAs<ControllableContainer, EnablingControllableContainer>(cc), isRoot)
{
    StringArray ips = NetworkHelpers::getLocalIPs();
    ipLabel.setText("IPs : " + ips.joinIntoString(","), dontSendNotification);
    ipLabel.setColour(ipLabel.textColourId, TEXTNAME_COLOR);
    ipLabel.setFont(headerHeight - 6);
    addAndMakeVisible(&ipLabel);
}

OSCInputEditor::~OSCInputEditor()
{
}

void OSCInputEditor::resizedInternalHeader(Rectangle<int>& r)
{
    ipLabel.setBounds(r.removeFromRight(jmin(ipLabel.getFont().getStringWidth(ipLabel.getText()), getWidth() - 100)));
    r.removeFromRight(2);
    EnablingControllableContainerEditor::resizedInternalHeader(r);
}
