#ifndef WCSimPrimaryGeneratorMessenger_h
#define WCSimPrimaryGeneratorMessenger_h 1

class WCSimPrimaryGeneratorAction;
class G4UIdirectory;
class G4UIcmdWithAString;
class G4UIcmdWithAnInteger;
class G4UIcmdWithADouble;

#include "G4UImessenger.hh"
#include "globals.hh"

class WCSimPrimaryGeneratorMessenger: public G4UImessenger
{
 public:
  WCSimPrimaryGeneratorMessenger(WCSimPrimaryGeneratorAction* mpga);
  ~WCSimPrimaryGeneratorMessenger();

 public:
  void     SetNewValue(G4UIcommand* command, G4String newValues);
  G4String GetCurrentValue(G4UIcommand* command);

 private:
  WCSimPrimaryGeneratorAction* myAction;

 private: //commands
  G4UIdirectory*        mydetDirectory;
  G4UIcmdWithAString*   genCmd;
  G4UIcmdWithAString*   fileNameCmd;
  G4UIcmdWithAString*   primariesfileDirectoryCmd;
  G4UIcmdWithAString*   neutrinosfileDirectoryCmd;
  G4UIcmdWithAnInteger* primariesStartEventCmd;
  // IBD generator commands
  G4UIcmdWithAString*   ibdDatabaseCmd;
  G4UIcmdWithAString*   ibdModelCmd;
  // AmBe position command
  G4UIcmdWithAString*   ambePositionCmd;
  // Radon generator commands
  G4UIcmdWithAnInteger* radonScenarioCmd;
  G4UIcmdWithAnInteger* radonSymmetryCmd;
  G4UIcmdWithADouble*   radonWaterConcCmd;
  // HepMC3 reader commands
  G4UIcmdWithAString*   hepmc3FileCmd;

};

#endif


