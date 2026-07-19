/*
  ==============================================================================

    Interface.h
    Created: 19 Oct 2025 12:29:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class InterfaceUI;

class Interface :
    public BaseItem
{
public:
    Interface(String name = "Interface");
    virtual ~Interface();

    BoolParameter* logIncomingData;
    BoolParameter* logOutgoingData;

    // virtual void sendValuesForObject(Object* o) {}

    virtual ControllableContainer* getInterfaceParams() { return new ControllableContainer("Interface parameters"); }

    virtual InterfaceUI* createUI();

    static bool isMidiInterface(Interface* i);

    // --- Redundancy (Main/Backup) standby ---
    // Runtime-only (not saved, distinct from the user-facing `enabled`). While an instance is a
    // Backup in standby, its interfaces must stay silent (no output, no outbound connection) so
    // they don't double-drive the same devices as the Main. Driven by RedundancyManager via
    // InterfaceManager::setRedundancyStandby(). Concrete interfaces override onRedundancyStandbyChanged()
    // to stop/resume their transport. Base default is safe (a non-overriding interface stays quiet
    // only if its send paths consult isRedundancyStandby()).
    bool redundancyStandby = false;
    void setRedundancyStandby(bool standby);
    bool isRedundancyStandby() const { return redundancyStandby; }
    virtual void onRedundancyStandbyChanged() {}
};
