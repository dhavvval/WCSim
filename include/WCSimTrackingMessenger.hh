#ifndef WCSimTrackingMessenger_h
#define WCSimTrackingMessenger_h 1

#include <cstdlib>
#include <string>
#include "G4UImessenger.hh"
#include "G4UIcmdWithAString.hh"

// Exposes the neutron-descendant cut profile as a Geant4 macro command:
//   /WCSim/tracking/neutronCutProfile conservative|balanced|aggressive
//
// Implementation note: the selected profile is cached on first call to
// GetNeutronCutValues() in WCSimTrackingAction.cc (static local), so this
// messenger must be used in PreInit/Idle states (before tracking starts).
// The command is hidden from normal state transitions via AvailableForStates.
class WCSimTrackingMessenger : public G4UImessenger {
 public:
  WCSimTrackingMessenger() {
    fCmd = new G4UIcmdWithAString("/WCSim/tracking/neutronCutProfile", this);
    fCmd->SetGuidance("Neutron-descendant cut profile: conservative|balanced|aggressive");
    fCmd->SetCandidates("conservative balanced aggressive");
    fCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
  }
  ~WCSimTrackingMessenger() override { delete fCmd; }

  void SetNewValue(G4UIcommand* cmd, G4String val) override {
    if (cmd != fCmd) return;
    int i = (val == "conservative") ? 0 : (val == "aggressive") ? 2 : 1;
    // Stash via env var so GetNeutronCutValues() picks it up on its first call.
    setenv("WCSIM_NEUTRON_CUT_PROFILE", std::to_string(i).c_str(), 1);
  }

 private:
  G4UIcmdWithAString* fCmd;
};

#endif
