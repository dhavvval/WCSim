#include "WCSimRunActionMessenger.hh"

#include "WCSimRunAction.hh"
#include "G4UIdirectory.hh"
#include "G4UIcommand.hh"
#include "G4UIparameter.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithABool.hh"

WCSimRunActionMessenger::WCSimRunActionMessenger(WCSimRunAction* WCSimRA)
:WCSimRun(WCSimRA)
{ 
  WCSimIODir = new G4UIdirectory("/WCSimIO/");
  WCSimIODir->SetGuidance("Commands to select I/O options");

  RootFile = new G4UIcmdWithAString("/WCSimIO/RootFile",this);
  RootFile->SetGuidance("Set the root file name");
  RootFile->SetGuidance("Enter the name of the output ROOT file");
  RootFile->SetParameterName("RootFileName",true);
  RootFile->SetDefaultValue("wcsim.root");

  SaveTracksOnDemand = new G4UIcmdWithABool("/WCSimIO/SaveTracksOnDemand",this);
  SaveTracksOnDemand->SetGuidance("Force-save tracks that produced detected photons");
  SaveTracksOnDemand->SetGuidance("If true, every track referenced as the direct parent of a");
  SaveTracksOnDemand->SetGuidance("detected photon is written out along with its ancestor chain,");
  SaveTracksOnDemand->SetGuidance("so per-hit DirectParentID always resolves to a real track.");
  SaveTracksOnDemand->SetGuidance("Set false to reproduce the historical save behaviour.");
  SaveTracksOnDemand->SetParameterName("SaveTracksOnDemand",true);
  SaveTracksOnDemand->SetDefaultValue(true);

}

WCSimRunActionMessenger::~WCSimRunActionMessenger()
{
  delete RootFile;
  delete SaveTracksOnDemand;
  delete WCSimIODir;
}

void WCSimRunActionMessenger::SetNewValue(G4UIcommand* command,G4String newValue)
{

  if ( command == RootFile)
    {
      WCSimRun->SetRootFileNameBase(newValue);
      G4cout << "Output ROOT file set to " << newValue << G4endl;
    }

  if ( command == SaveTracksOnDemand)
    {
      G4bool choice = SaveTracksOnDemand->GetNewBoolValue(newValue);
      WCSimRun->SetSaveTracksOnDemand(choice);
      G4cout << "Save-tracks-on-demand set to " << (choice ? "true" : "false") << G4endl;
    }

}
