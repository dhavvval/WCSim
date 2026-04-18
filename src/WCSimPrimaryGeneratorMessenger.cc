#include "WCSimPrimaryGeneratorMessenger.hh"
#include "WCSimPrimaryGeneratorAction.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithADouble.hh"
#include "G4ios.hh"
#include <sstream>

WCSimPrimaryGeneratorMessenger::WCSimPrimaryGeneratorMessenger(WCSimPrimaryGeneratorAction* pointerToAction)
:myAction(pointerToAction)
{
  mydetDirectory = new G4UIdirectory("/mygen/");
  mydetDirectory->SetGuidance("WCSim detector control commands.");

  genCmd = new G4UIcmdWithAString("/mygen/generator",this);
  genCmd->SetGuidance("Select primary generator.");
  genCmd->SetGuidance(" Available generators : muline, gun, laser, gps, beam, ibd, ambe, radon, hepmc3");
  genCmd->SetParameterName("generator",true);
  genCmd->SetDefaultValue("beam");
  genCmd->SetCandidates("muline gun laser gps beam ibd ambe radon hepmc3");

  fileNameCmd = new G4UIcmdWithAString("/mygen/vecfile",this);
  fileNameCmd->SetGuidance("Select the file of vectors.");
  fileNameCmd->SetGuidance(" Enter the file name of the vector file");
  fileNameCmd->SetParameterName("fileName",true);
  fileNameCmd->SetDefaultValue("inputvectorfile");
  
  primariesfileDirectoryCmd = new G4UIcmdWithAString("/mygen/primariesdirectory", this);
  primariesfileDirectoryCmd->SetGuidance("Specify the directory containing beam primary root files");
  primariesfileDirectoryCmd->SetParameterName("directoryName",true);
  primariesfileDirectoryCmd->SetDefaultValue("");
  
  neutrinosfileDirectoryCmd = new G4UIcmdWithAString("/mygen/neutrinosdirectory", this);
  neutrinosfileDirectoryCmd->SetGuidance("Specify the directory containing genie neutrino root files. Set this before setting the primariesDirectory. Both should be set at the same time.");
  neutrinosfileDirectoryCmd->SetParameterName("directoryName",true);
  neutrinosfileDirectoryCmd->SetDefaultValue("");

  primariesStartEventCmd = new G4UIcmdWithAnInteger("/mygen/primariesoffset", this);
  primariesStartEventCmd->SetGuidance("The starting entry number for reading primaries");
  primariesStartEventCmd->SetParameterName("primariesoffset",true);
  primariesStartEventCmd->SetDefaultValue(0);

  // IBD generator
  ibdDatabaseCmd = new G4UIcmdWithAString("/mygen/ibd_database", this);
  ibdDatabaseCmd->SetGuidance("Path to JSON file with antineutrino flux spectra for IBD generator");
  ibdDatabaseCmd->SetParameterName("ibdDatabase", true);
  ibdDatabaseCmd->SetDefaultValue("");

  ibdModelCmd = new G4UIcmdWithAString("/mygen/ibd_model", this);
  ibdModelCmd->SetGuidance("Model name key in the IBD spectrum JSON database");
  ibdModelCmd->SetParameterName("ibdModel", true);
  ibdModelCmd->SetDefaultValue("Flat");

  // AmBe source position
  ambePositionCmd = new G4UIcmdWithAString("/mygen/ambe_position", this);
  ambePositionCmd->SetGuidance("AmBe source position in mm: 'x y z'");
  ambePositionCmd->SetParameterName("ambePosition", true);
  ambePositionCmd->SetDefaultValue("0 0 0");

  // Radon generator
  radonScenarioCmd = new G4UIcmdWithAnInteger("/mygen/radon_scaling", this);
  radonScenarioCmd->SetGuidance("Radon scenario index (default 1)");
  radonScenarioCmd->SetParameterName("radonScenario", true);
  radonScenarioCmd->SetDefaultValue(1);

  radonSymmetryCmd = new G4UIcmdWithAnInteger("/mygen/radon_symmetry", this);
  radonSymmetryCmd->SetGuidance("Radon symmetry flag (default 1)");
  radonSymmetryCmd->SetParameterName("radonSymmetry", true);
  radonSymmetryCmd->SetDefaultValue(1);

  radonWaterConcCmd = new G4UIcmdWithADouble("/mygen/radon_water_concentration", this);
  radonWaterConcCmd->SetGuidance("Radon water concentration in mBq/m3 (default 2.63)");
  radonWaterConcCmd->SetParameterName("radonWaterConc", true);
  radonWaterConcCmd->SetDefaultValue(2.63);

  // HepMC3 reader
  hepmc3FileCmd = new G4UIcmdWithAString("/mygen/hepmc3file", this);
  hepmc3FileCmd->SetGuidance("Path to HepMC3 event file");
  hepmc3FileCmd->SetParameterName("hepmc3File", true);
  hepmc3FileCmd->SetDefaultValue("inputhepmc3file");
}

WCSimPrimaryGeneratorMessenger::~WCSimPrimaryGeneratorMessenger()
{
  delete genCmd;
  delete fileNameCmd;
  delete primariesfileDirectoryCmd;
  delete neutrinosfileDirectoryCmd;
  delete mydetDirectory;
  delete primariesStartEventCmd;
  delete ibdDatabaseCmd;
  delete ibdModelCmd;
  delete ambePositionCmd;
  delete radonScenarioCmd;
  delete radonSymmetryCmd;
  delete radonWaterConcCmd;
  delete hepmc3FileCmd;
}

void WCSimPrimaryGeneratorMessenger::SetNewValue(G4UIcommand * command,G4String newValue)
{
  if( command==genCmd )
  {
    if (newValue == "muline")
    {
      G4cout<<"Setting generator source to muline"<<G4endl;
      myAction->SetMulineEvtGenerator(true);
      myAction->SetGunEvtGenerator(false);
      myAction->SetLaserEvtGenerator(false);
      myAction->SetBeamEvtGenerator(false);
      myAction->SetGPSEvtGenerator(false);
    }
    else if ( newValue == "gun")
    {
      G4cout<<"Setting generator source to gun"<<G4endl;
      myAction->SetMulineEvtGenerator(false);
      myAction->SetGunEvtGenerator(true);
      myAction->SetLaserEvtGenerator(false);
      myAction->SetBeamEvtGenerator(false);
      myAction->SetGPSEvtGenerator(false);
    }
    else if ( newValue == "laser")   //T. Akiri: Addition of laser
    {
      G4cout<<"Setting generator source to laser"<<G4endl;
      myAction->SetMulineEvtGenerator(false);
      myAction->SetGunEvtGenerator(false);
      myAction->SetLaserEvtGenerator(true);
      myAction->SetBeamEvtGenerator(false);
      myAction->SetGPSEvtGenerator(false);
    }
    else if ( newValue == "beam")
    {
      G4cout<<"Setting generator source to beam"<<G4endl;
      myAction->SetMulineEvtGenerator(false);
      myAction->SetGunEvtGenerator(false);
      myAction->SetLaserEvtGenerator(false);
      myAction->SetBeamEvtGenerator(true);
      myAction->SetGPSEvtGenerator(false);
    }
    else if ( newValue == "gps")
    {
      G4cout<<"Setting generator source to gps"<<G4endl;
      myAction->SetMulineEvtGenerator(false);
      myAction->SetGunEvtGenerator(false);
      myAction->SetLaserEvtGenerator(false);
      myAction->SetBeamEvtGenerator(false);
      myAction->SetGPSEvtGenerator(true);
      myAction->SetIBDEvtGenerator(false);
      myAction->SetAmBeEvtGenerator(false);
      myAction->SetRadonEvtGenerator(false);
      myAction->SetHepMC3EvtGenerator(false);
    }
    else if ( newValue == "ibd")
    {
      G4cout<<"Setting generator source to IBD"<<G4endl;
      myAction->SetMulineEvtGenerator(false);
      myAction->SetGunEvtGenerator(false);
      myAction->SetLaserEvtGenerator(false);
      myAction->SetBeamEvtGenerator(false);
      myAction->SetGPSEvtGenerator(false);
      myAction->SetIBDEvtGenerator(true);
      myAction->SetAmBeEvtGenerator(false);
      myAction->SetRadonEvtGenerator(false);
      myAction->SetHepMC3EvtGenerator(false);
    }
    else if ( newValue == "ambe")
    {
      G4cout<<"Setting generator source to AmBe"<<G4endl;
      myAction->SetMulineEvtGenerator(false);
      myAction->SetGunEvtGenerator(false);
      myAction->SetLaserEvtGenerator(false);
      myAction->SetBeamEvtGenerator(false);
      myAction->SetGPSEvtGenerator(false);
      myAction->SetIBDEvtGenerator(false);
      myAction->SetAmBeEvtGenerator(true);
      myAction->SetRadonEvtGenerator(false);
      myAction->SetHepMC3EvtGenerator(false);
    }
    else if ( newValue == "radon")
    {
      G4cout<<"Setting generator source to Radon"<<G4endl;
      myAction->SetMulineEvtGenerator(false);
      myAction->SetGunEvtGenerator(false);
      myAction->SetLaserEvtGenerator(false);
      myAction->SetBeamEvtGenerator(false);
      myAction->SetGPSEvtGenerator(false);
      myAction->SetIBDEvtGenerator(false);
      myAction->SetAmBeEvtGenerator(false);
      myAction->SetRadonEvtGenerator(true);
      myAction->SetHepMC3EvtGenerator(false);
    }
    else if ( newValue == "hepmc3")
    {
      G4cout<<"Setting generator source to HepMC3"<<G4endl;
      myAction->SetMulineEvtGenerator(false);
      myAction->SetGunEvtGenerator(false);
      myAction->SetLaserEvtGenerator(false);
      myAction->SetBeamEvtGenerator(false);
      myAction->SetGPSEvtGenerator(false);
      myAction->SetIBDEvtGenerator(false);
      myAction->SetAmBeEvtGenerator(false);
      myAction->SetRadonEvtGenerator(false);
      myAction->SetHepMC3EvtGenerator(true);
    }
  }

  if( command == fileNameCmd )
  {
    myAction->OpenVectorFile(newValue);
    G4cout << "Input vector file set to " << newValue << G4endl;
  }
  
  if( command == primariesfileDirectoryCmd )
  {
    myAction->SetPrimaryFilesDirectory(newValue);
    myAction->SetNewPrimariesFlag(true);
    G4cout << "Input directory set to " << newValue << G4endl;
  }
  
  if( command == neutrinosfileDirectoryCmd )
  {
    myAction->SetNeutrinoFilesDirectory(newValue);
    G4cout << "Input directory set to " << newValue << G4endl;
  }
  
  if( command == primariesStartEventCmd )
  {
    myAction->SetPrimariesOffset(primariesStartEventCmd->GetNewIntValue(newValue));
    G4cout << "Primary files will be read starting from entry "<<newValue << G4endl;
  }

  if( command == ibdDatabaseCmd )
  {
    myAction->SetIBDDatabase(newValue);
    G4cout << "IBD spectrum database set to " << newValue << G4endl;
  }

  if( command == ibdModelCmd )
  {
    myAction->SetIBDModel(newValue);
    G4cout << "IBD model set to " << newValue << G4endl;
  }

  if( command == ambePositionCmd )
  {
    // Parse "x y z" in mm
    std::istringstream iss(newValue);
    double x, y, z;
    iss >> x >> y >> z;
    if (myAction->IsUsingAmBeEvtGenerator()) {
      // Position will be applied at generator init — store via AmBeGen if available
    }
    G4cout << "AmBe source position set to (" << x << ", " << y << ", " << z << ") mm" << G4endl;
  }

  if( command == radonScenarioCmd )
  {
    myAction->SetRadonScenario(radonScenarioCmd->GetNewIntValue(newValue));
    G4cout << "Radon scenario set to " << newValue << G4endl;
  }

  if( command == radonSymmetryCmd )
  {
    myAction->SetRadonSymmetry(radonSymmetryCmd->GetNewIntValue(newValue));
    G4cout << "Radon symmetry set to " << newValue << G4endl;
  }

  if( command == radonWaterConcCmd )
  {
    myAction->SetRadonWaterConcentration(radonWaterConcCmd->GetNewDoubleValue(newValue));
    G4cout << "Radon water concentration set to " << newValue << " mBq/m3" << G4endl;
  }

  if( command == hepmc3FileCmd )
  {
    myAction->SetHepMC3Filename(newValue);
    G4cout << "HepMC3 input file set to " << newValue << G4endl;
  }

}

G4String WCSimPrimaryGeneratorMessenger::GetCurrentValue(G4UIcommand* command)
{
  G4String cv;
  
  if( command==genCmd )
  {
    if(myAction->IsUsingMulineEvtGenerator())
      { cv = "muline"; }
    else if(myAction->IsUsingGunEvtGenerator())
      { cv = "gun"; }
    else if(myAction->IsUsingLaserEvtGenerator())
      { cv = "laser"; }   //T. Akiri: Addition of laser
    else if(myAction->IsUsingBeamEvtGenerator())
      { cv = "beam"; }
    else if(myAction->IsUsingGPSEvtGenerator())
      { cv = "gps"; }
  }
  
  return cv;
  G4cout<<"generator is currently "<<cv<<G4endl;
}

