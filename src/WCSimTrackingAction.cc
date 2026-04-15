#include "WCSimTrackingAction.hh"
#include "WCSimTrajectory.hh"
#include "G4ParticleTypes.hh"
#include "G4TrackingManager.hh"
#include "G4Track.hh"
#include "G4ios.hh"
#include "G4VProcess.hh"
#include "G4UserEventAction.hh"
#include "G4EventManager.hh"
#include "WCSimTrackInformation.hh"
#include "WCSimEventInformation.hh"
#include "WCSimTrackingMessenger.hh"
#include "G4TransportationManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4Event.hh"
#include <algorithm>
#include <cstdlib>

namespace {

struct NeutronCutValues {
  G4double gammaMinKinE;
  G4double electronMinKinE;
  G4double protonMinKinE;
  G4double ionMinKinE;
  G4double maxGlobalTime;
  bool     requireNeutronLikeProcess;
};

// Index 0=conservative, 1=balanced, 2=aggressive.
// Select with env var WCSIM_NEUTRON_CUT_PROFILE or /WCSim/tracking/neutronCutProfile macro command.
static const NeutronCutValues kProfiles[3] = {
  // gamma      e+/-       proton     ion        t_max                req.proc
  { 0.10*MeV,  0.05*MeV,  5.0*MeV,   0.20*MeV,  500.0*microsecond,   false }, // conservative
  { 0.30*MeV,  0.15*MeV,  10.0*MeV,  0.50*MeV,  200.0*microsecond,   false }, // balanced
  { 0.50*MeV,  0.30*MeV,  20.0*MeV,  1.00*MeV,  100.0*microsecond,   true  }, // aggressive
};
static const char* kProfileNames[3] = { "CONSERVATIVE", "BALANCED", "AGGRESSIVE" };

const NeutronCutValues& GetNeutronCutValues() {
  static const int idx = [](){
    const char* e = std::getenv("WCSIM_NEUTRON_CUT_PROFILE");
    int i = e ? std::atoi(e) : 1;
    if (i < 0 || i > 2) i = 1;
    G4cout << "WCSim neutron-descendant cut profile: " << kProfileNames[i]
           << " (WCSIM_NEUTRON_CUT_PROFILE=" << i << ")\n"
           << "  gamma>=" << kProfiles[i].gammaMinKinE/MeV << " MeV, "
           << "e+/->=" << kProfiles[i].electronMinKinE/MeV << " MeV, "
           << "p>=" << kProfiles[i].protonMinKinE/MeV << " MeV, "
           << "ion>=" << kProfiles[i].ionMinKinE/MeV << " MeV, "
           << "t<=" << kProfiles[i].maxGlobalTime/microsecond << " us, "
           << "reqProc=" << kProfiles[i].requireNeutronLikeProcess << G4endl;
    return i;
  }();
  return kProfiles[idx];
}

// Per-cut stats, printed at program exit.
// To move this to end-of-run, call gStats.Print() from WCSimRunAction::EndOfRunAction.
struct NeutronCutStats {
  long seen = 0, saved = 0, rejTime = 0, rejEnergy = 0, rejProcess = 0;
  ~NeutronCutStats() {
    G4cout << "\n=== Neutron-ancestor cut stats ===\n"
           << "  seen:           " << seen       << "\n"
           << "  saved:          " << saved      << "\n"
           << "  rej by time:    " << rejTime    << "\n"
           << "  rej by energy:  " << rejEnergy  << "\n"
           << "  rej by process: " << rejProcess << "\n"
           << "==================================" << G4endl;
  }
};
static NeutronCutStats gStats;

// Exact-name set of processes relevant to neutron physics in water.
// Tune this list after running once with a diagnostic print in PostUserTrackingAction
// (see commented block there) to enumerate which process names your physics list produces.
bool IsNeutronRelevantProcess(const G4VProcess* creatorProcess) {
  if (!creatorProcess) return false;
  static const std::set<std::string> kRelevant = {
    "nCapture", "neutronInelastic", "hadElastic",
    "protonInelastic", "compt", "phot", "conv", "eBrem",
  };
  return kRelevant.count(creatorProcess->GetProcessName()) > 0;
}

bool PassesNeutronAncestorCuts(const G4Track* aTrack,
                               const G4VProcess* creatorProcess,
                               G4int thispdg) {
  ++gStats.seen;
  const NeutronCutValues& cuts = GetNeutronCutValues();
  const G4double ke = aTrack->GetKineticEnergy();

  if (cuts.maxGlobalTime > 0.0 && aTrack->GetGlobalTime() > cuts.maxGlobalTime) {
    ++gStats.rejTime;
    return false;
  }

  if (cuts.requireNeutronLikeProcess && creatorProcess &&
      !IsNeutronRelevantProcess(creatorProcess) &&
      !((thispdg == 22 || std::abs(thispdg) == 11) && ke > 2.0 * MeV)) {
    ++gStats.rejProcess;
    return false;
  }

  const G4int absPdg = std::abs(thispdg);
  bool pass = true;
  if      (thispdg == 22)       pass = ke >= cuts.gammaMinKinE;
  else if (absPdg  == 11)       pass = ke >= cuts.electronMinKinE;
  else if (thispdg == 2212)     pass = ke >= cuts.protonMinKinE;
  else if (absPdg  > 1000000000) pass = (thispdg == 1000010020) || (ke >= cuts.ionMinKinE);
  // else: other hadrons/muons kept by default (pass stays true)

  if (pass) ++gStats.saved; else ++gStats.rejEnergy;
  return pass;
}

}  // namespace

WCSimTrackingAction::WCSimTrackingAction(){
  ProcessList.insert("Decay") ;
  ProcessList.insert("nCapture");
  ProcessList.insert("MuonMinusCaptureAtRest");
  ProcessList.insert("muMinusCaptureAtRest");  // which syntax is correct?
  //ProcessList.insert("conv");
  ParticleList.insert(0);    // geantino
  ParticleList.insert(111);  // pi0
  ParticleList.insert(211);  // pion+
  ParticleList.insert(-211); // pion-
  ParticleList.insert(321);  // kaon+
  ParticleList.insert(-321); // kaon-
  ParticleList.insert(311);  // kaon0
  ParticleList.insert(-311); // kaon0 bar
  ParticleList.insert(12);   // nu_e
  ParticleList.insert(-12);  // nubar_e
  ParticleList.insert(13);   // mu-
  ParticleList.insert(-13);  // mu+
  ParticleList.insert(14);   // nu_mu
  ParticleList.insert(-14);  // nubar_mu
  ParticleList.insert(2112); // neutron
  ParticleList.insert(2212); // proton
//  ParticleList.insert(11);   // e-    // do not save electrons unless they are from Decay process (mu decay)
//  ParticleList.insert(-11);  // e+
//  Don't put gammas there or there'll be too many -  we can add an energy cut later

  fMessenger = new WCSimTrackingMessenger();
}

WCSimTrackingAction::~WCSimTrackingAction(){ delete fMessenger; }

void WCSimTrackingAction::PreUserTrackingAction(const G4Track* aTrack){
  G4float percentageOfCherenkovPhotonsToDraw = 0.0;
  
  if (aTrack->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition()
       || G4UniformRand() < percentageOfCherenkovPhotonsToDraw){
      WCSimTrajectory* thisTrajectory = new WCSimTrajectory(aTrack);
      fpTrackingManager->SetTrajectory(thisTrajectory);
      fpTrackingManager->SetStoreTrajectory(true);
  } else {
      fpTrackingManager->SetStoreTrajectory(false);
  }
 
  if (aTrack->GetDefinition()->GetParticleName() == "opticalphoton"){
    G4Event* event = G4EventManager::GetEventManager()->GetNonconstCurrentEvent();
    WCSimEventInformation* evInfo = dynamic_cast<WCSimEventInformation*> (event->GetUserInformation());
    WCSimTrackInformation* trackInfo= dynamic_cast<WCSimTrackInformation*> (aTrack->GetUserInformation());
    std::string creatorProcessName="";
    const G4VProcess *creatorProcess = aTrack->GetCreatorProcess();

    if (creatorProcess) creatorProcessName = creatorProcess->GetProcessName();
    if (creatorProcessName == "Cerenkov") {
	evInfo->numCherenPhoton++;
        G4float photonWavelength = (2.0*M_PI*197.3)/(aTrack->GetTotalEnergy()/CLHEP::eV);
        //evInfo->hCher.Fill(photonWavelength);	//Comment out in case you want to create histograms of the wavelength distribution
        if (photonWavelength >= 200 && photonWavelength < 790) {
          evInfo->numCherenPhotonWCSim++;
          //evInfo->hCherWCSim.Fill(photonWavelength); //Comment out in case you want to create histograms of the wavelength distribution
          G4ThreeVector photonPosition = aTrack->GetVertexPosition();
          //evInfo->hCherXZ.Fill(photonPosition.z()/1000.,photonPosition.x()/1000.);
          //evInfo->hCherYZ.Fill(photonPosition.z()/1000.,photonPosition.y()/1000.);
        }
    }
  }
 
  /*
  // implemented to allow photon tracks to be drawn during photon debugging, 
  // but interferes with saving of primary information.
  WCSimTrackInformation* anInfo = new WCSimTrackInformation();
  G4Track* theTrack = (G4Track*)aTrack;
  anInfo->WillBeSaved(false);
  theTrack->SetUserInformation(anInfo);
  */
}

void WCSimTrackingAction::PostUserTrackingAction(const G4Track* aTrack){
  
  // retrieve UserTrackInfo
  // This is used to keep track of parentage - PrimaryParentId, stored in Hits,
  // and ParentPdg, used by EndOfEventAction when recording Tracks -
  // and to mark Trajectories to be saved by EndOfEventAction
  WCSimTrackInformation* anInfo;
  if (aTrack->GetUserInformation()){
    anInfo = (WCSimTrackInformation*)(aTrack->GetUserInformation());
  } else {
    anInfo = new WCSimTrackInformation();
  }
  
  // get particle type and creator process
  const G4VProcess* creatorProcess = aTrack->GetCreatorProcess();
  G4int thispdg;
  if(aTrack->GetDefinition()==G4OpticalPhoton::OpticalPhotonDefinition()) thispdg=100;
  else thispdg = aTrack->GetDefinition()->GetPDGEncoding();

  /* // --- DIAGNOSTIC: enumerate neutron-ancestor creator processes ---
  // Uncomment to collect the set of process names your physics list produces on
  // neutron-descendant tracks, then feed them into IsNeutronRelevantProcess().
  if (anInfo->GetHasNeutronAncestor() && creatorProcess) {
    static std::set<std::string> seenProc;
    const std::string& pn = creatorProcess->GetProcessName();
    if (seenProc.insert(pn).second) {
      G4cout << "[nDesc process seen] " << pn << "  (pdg=" << thispdg << ")" << G4endl;
    }
  }
  */
  
  // check if it's of interest
  // *  is it a primary ?
  // *  is the process in the set ?
  // *  is the particle in the set ?
  // *  is it a gamma with energy > threshold?
  /*if( ( aTrack->GetParentID()==0 ) ||
      ( (creatorProcess!=0) && ProcessList.count(creatorProcess->GetProcessName()) ) ||
      ( ParticleList.count(thispdg) ) ||
      ( thispdg==22 && aTrack->GetTotalEnergy()>50.0*MeV ) ||     // 50 MeV? 1MeV? what threshold?
      ( thispdg==22 && anInfo->GetParentPdg()==111 )              // gamma from a Pi0 decay
    ){*/		//-->this is currently the default
    const bool neutronAncestorTrack = anInfo->GetHasNeutronAncestor();
    const bool saveNeutronAncestorTrack =
      neutronAncestorTrack && PassesNeutronAncestorCuts(aTrack, creatorProcess, thispdg);

    if( aTrack->GetParentID()==0 || 
      ((creatorProcess!=0) && ProcessList.count(creatorProcess->GetProcessName())) ||
      (ParticleList.count(aTrack->GetDefinition()->GetPDGEncoding())) || 
      (aTrack->GetDefinition()->GetPDGEncoding()==22 && aTrack->GetTotalEnergy() > 1.0*MeV) ||
      ((creatorProcess!=0) && creatorProcess->GetProcessName() == "muMinusCaptureAtRest" && aTrack->GetTotalEnergy() > 1.0*MeV)||
      ( thispdg==22 && anInfo->GetParentPdg()==111) || saveNeutronAncestorTrack ){	//---> try this out to get lower energetic gammas
    anInfo->WillBeSaved(true);
  } else {
    anInfo->WillBeSaved(false);
  }
  
  // for primary particles, set the ParentID to the track's own ID
  if(aTrack->GetParentID()==0 && aTrack->GetDefinition()!=G4OpticalPhoton::OpticalPhotonDefinition()){
    anInfo->SetPrimaryParentID(aTrack->GetTrackID());
  }
  
  // bypass const-ness to update track information
  G4Track* theTrack = (G4Track*)aTrack;
  theTrack->SetUserInformation(anInfo);
  
  // pass parentage information to children
  G4TrackVector* secondaries = fpTrackingManager->GimmeSecondaries();
  if(secondaries){
      for(size_t i=0;i<secondaries->size();i++){
        WCSimTrackInformation* infoSec = new WCSimTrackInformation(anInfo);
        infoSec->WillBeSaved(false);
        infoSec->SetParentPdg(thispdg);
        infoSec->SetHasNeutronAncestor(anInfo->GetHasNeutronAncestor() || aTrack->GetDefinition()->GetPDGEncoding() == 2112); //
        infoSec->SetPrimaryParentID(anInfo->GetPrimaryParentID()); // pass down primary parent ID, Do I need to set DirectParentID too? (DJA)
        (*secondaries)[i]->SetUserInformation(infoSec);
      }
  }
  
  // Pass the information to the Trajectory, used by EndOfEventAction
  if ( aTrack->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition()){
    G4ThreeVector currentPosition      = aTrack->GetPosition();
    G4VPhysicalVolume* currentVolume   = aTrack->GetVolume();
    G4double currentTime               = aTrack->GetGlobalTime();
    G4ThreeVector currentMomentum      = aTrack->GetMomentum();
    
    WCSimTrajectory *currentTrajectory = (WCSimTrajectory*)fpTrackingManager->GimmeTrajectory();
    currentTrajectory->SetStoppingPoint(currentPosition);
    currentTrajectory->SetStoppingVolume(currentVolume);
    currentTrajectory->SetStoppingTime(currentTime);
    currentTrajectory->SetStoppingMomentum(currentMomentum);
    currentTrajectory->SetParentPdg(anInfo->GetParentPdg());
    currentTrajectory->SetPrimaryParentID(anInfo->GetPrimaryParentID());
    currentTrajectory->SetSaveFlag(anInfo->isSaved());
    
    // DEBUG: Print parent IDs for first few tracks
    static int trackDebugCount = 0;
    if(trackDebugCount < 10 && anInfo->isSaved()) {
      G4cout << "DEBUG WCSimTrackingAction: Track #" << trackDebugCount
             << " TrackID=" << aTrack->GetTrackID()
             << " PrimaryParentID=" << anInfo->GetPrimaryParentID()
             << " DirectParentID=" << aTrack->GetParentID()
             << " PDG of this track=" << aTrack->GetDefinition()->GetPDGEncoding() << G4endl;
      trackDebugCount++;
    }
  }
  
  // report every 100000'th track, just to see progress
  static int line=0;
  if(line%100000==0){ //100000
    G4cout<<"  PostUserTrackingAction call number: "<<line
          <<", "<<aTrack->GetDefinition()->GetParticleName();
    if(creatorProcess) G4cout<<" from "<<creatorProcess->GetProcessName();
    else G4cout<<"primary";
    G4cout<<" in "<<aTrack->GetVolume()->GetName()<<G4endl; 
  }
  line++;
  
  /*
  if( (aTrack->GetParentID()==0) // primary particle
      && (abs(thispdg)==13) ){ // is a muon
      G4ThreeVector endpos = aTrack->GetPosition();
      WCSimTrajectory* trj = (WCSimTrajectory*)fpTrackingManager->GimmeTrajectory();
      G4TrajectoryPoint* startpnt = (G4TrajectoryPoint*)trj->GetPoint(0);
      G4ThreeVector startpos  = startpnt->GetPosition();
      G4cout<<"Primary muon started at ("<<startpos.x()<<", "<<startpos.y()<<", "<<startpos.z()<<")"
            <<"and ended at ("<<endpos.x()<<", "<<endpos.y()<<", "<<endpos.z()<<")"<<G4endl;
  }
  */
}





