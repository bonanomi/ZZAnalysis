/** \class dumpUserData
 *
 *  Dump all userFloat values attached to relevant collections of candidates.
 *
 *  $Date: 2013/06/06 15:40:38 $
 *  $Revision: 1.11 $
 *  \author N. Amapane - Torino
 */


#include <FWCore/Framework/interface/Frameworkfwd.h>
#include <FWCore/Framework/interface/EDAnalyzer.h>
#include <FWCore/Framework/interface/ESHandle.h>
#include <FWCore/Framework/interface/Event.h>
#include <FWCore/ParameterSet/interface/ParameterSet.h>
#include <DataFormats/Candidate/interface/Candidate.h>
#include <DataFormats/PatCandidates/interface/CompositeCandidate.h>
#include <DataFormats/PatCandidates/interface/Muon.h>
#include <DataFormats/PatCandidates/interface/Electron.h>
#include <DataFormats/PatCandidates/interface/Jet.h>
#include <ZZAnalysis/AnalysisStep/interface/PhotonFwd.h>
#include <DataFormats/Common/interface/TriggerResults.h>
#include <FWCore/Common/interface/TriggerNames.h>

#include <iostream>
#include <iterator>
#include <string>

using namespace std;
using namespace edm;
using namespace reco;


class dumpUserData: public edm::EDAnalyzer {
public:
  dumpUserData(const ParameterSet& pset);

  void analyze(const edm::Event & event, const edm::EventSetup& eventSetup);
 
  virtual void beginJob() {};
  virtual void endJob() {};  

  void dumpCandidates(const View<pat::CompositeCandidate>& cands);
  template<typename T> void dumpUserVal(const T& cand);

  edm::EDGetTokenT<pat::MuonCollection> muonToken;
  edm::EDGetTokenT<pat::ElectronCollection> electronToken;
  bool dumpJets;
  edm::EDGetTokenT<pat::JetCollection> jetToken;
  edm::EDGetTokenT<vector<reco::Vertex> > vtxToken;
  vector<string> collNames;
  vector<edm::EDGetTokenT<edm::View<pat::CompositeCandidate> > > candidateSrcTokens;
  bool listTriggers;
  edm::EDGetTokenT<edm::TriggerResults> triggerResultToken;

};


dumpUserData::dumpUserData(const ParameterSet& pset):
  muonToken(consumes<pat::MuonCollection>(pset.getParameter<InputTag>("muonSrc"))),
  electronToken(consumes<pat::ElectronCollection>(pset.getParameter<InputTag>("electronSrc"))),
  dumpJets(pset.existsAs<InputTag>("jetSrc")),
  jetToken( dumpJets ? consumes<pat::JetCollection>(pset.getParameter<InputTag>("jetSrc")) : edm::EDGetTokenT<pat::JetCollection>() ),
  listTriggers(pset.getUntrackedParameter<bool>("dumpTrigger",false))
{

  vtxToken = consumes<vector<reco::Vertex> >(edm::InputTag("offlinePrimaryVertices"));

  ParameterSet collps = pset.getParameter<ParameterSet>("candidateSrcs");
  collNames = collps.getParameterNamesForType<InputTag>();
  for( unsigned i=0; i<collNames.size(); ++i) {
    candidateSrcTokens.push_back(consumes<edm::View<pat::CompositeCandidate> >(collps.getParameter<InputTag>(collNames[i])));
  }

  triggerResultToken = consumes<edm::TriggerResults>(edm::InputTag("TriggerResults","","HLT"));
}


//Explicit instantiation of template function
// template void dumpUserData::dumpUserVal<const pat::Muon>(const pat::Muon& cand);
// template void dumpUserData::dumpUserVal<const pat::Electron>(const pat::Electron& cand);
// template void dumpUserData::dumpUserVal<const pat::CompositeCandidate>(const pat::CompositeCandidate& cand);


void dumpUserData::analyze(const Event & event, const EventSetup& eventSetup){

  int irun=event.id().run();
  long long int ievt=event.id().event(); 
  int ils =event.luminosityBlock();
  cout << "Dump for event " << irun << ":" << ils << ":" << ievt << endl; 

  Handle<pat::MuonCollection> muons;
  event.getByToken(muonToken, muons);

  Handle<pat::ElectronCollection> electrons;
  event.getByToken(electronToken, electrons);

  bool dumpVertices=false;
  if (dumpVertices) {  
    edm::Handle<vector<reco::Vertex> > vtxs;
    event.getByToken(vtxToken, vtxs);

    for( vector<reco::Vertex>::const_iterator vtx =vtxs->begin(); vtx != vtxs->end(); ++vtx ) {
      float rho = vtx->position().rho();
      float z = vtx->z();
      float isFake =vtx->isFake();
      float ndof = vtx->ndof();
    
      cout << "VTX: " << rho << " " << z << " " << isFake << " " << ndof<< " " 
	   << (!isFake && ndof > 4 && abs(z) <= 24 && rho <= 2) << endl;
    }
  }
  
  

  cout << " mu Candidates:    " << muons->size() << endl;
  cout << " e  Candidates:    " << electrons->size() << endl;

  cout << "Muons:" << endl;  
  for( pat::MuonCollection::const_iterator lep =muons->begin(); lep != muons->end(); ++lep ) {
    int i = distance(muons->begin(),lep);

    int genID=0;
    float genPT=0.;
    const reco::GenParticle * gp =lep->genLepton();
    if (gp) {
      genID=gp->pdgId();
      genPT=gp->pt();
    }

    cout << "#" << i << " mu"  << ((lep->charge()>0)?"+ ":"- ") << " pt= " << lep->pt() << " eta= " << lep->eta() << " phi= " << lep->phi() << " GLB= " << lep->isGlobalMuon() << " TK= " << lep->isTrackerMuon() << " matches= " << lep->numberOfMatches() << " BTT= " << lep->muonBestTrackType() << " t0_nDof: " << lep->time().nDof << " t0(ns): " << lep->time().timeAtIpInOut << " genID= " << genID <<  " genPT= " << genPT;

//	 << " BTPT: " <<  lep->muonBestTrack()->pt() << " " << lep->innerTrack()->pt() << " " <<  lep->innerTrack()->eta() << " " << lep->innerTrack()->phi();
    dumpUserVal(*lep);
    if (lep->hasUserData("FSRCandidates")){
      const PhotonPtrVector* fsrEle = lep->userData<PhotonPtrVector>("FSRCandidates");
      if (fsrEle->size()) {
	cout << " Photons: pT=";	
	for (PhotonPtrVector::const_iterator g = fsrEle->begin(); g!=fsrEle->end(); ++g) {
	  cout << " " << (*g)->pt();
	}
      }
    }
    cout << endl;
  }

  cout << "Electrons:" << endl;
  for( pat::ElectronCollection::const_iterator lep = electrons->begin(); lep != electrons->end(); ++lep ) {
    int i = distance(electrons->begin(),lep);

    int genID=0;
    float genPT=0.;
    const reco::GenParticle * gp =lep->genLepton();
    if (gp) {
      genID=gp->pdgId();
      genPT=gp->pt();
    }

    cout << "#" << i << " e"  << ((lep->charge()>0)?"+  ":"-  ") << " pt= " << lep->pt() << " eta= " << lep->eta() << " phi= " << lep->phi() << " genID= " << genID <<  " genPT= " << genPT;

    dumpUserVal(*lep);
    if (lep->hasUserData("FSRCandidates")){
      const PhotonPtrVector* fsrEle = lep->userData<PhotonPtrVector>("FSRCandidates");
      if (fsrEle->size()) {
	cout << " Photon pTs:"; // fsrEle->size() << endl;
	for (PhotonPtrVector::const_iterator g = fsrEle->begin(); g!=fsrEle->end(); ++g) {
	  cout << " (pt=" << (*g)->pt() ;//<< " isFromMu=" << (*g)->isFromMuon() << ")";
	}
      }
    }
    cout << endl;
  }

  
  unsigned int nColls = collNames.size();
  for(unsigned i=0; i<collNames.size(); ++i) {
    Handle<View<pat::CompositeCandidate> > coll;
    int j = nColls-i-1;
    event.getByToken(candidateSrcTokens[j],coll);
    cout << collNames[j] << ": " << coll->size() << endl;
    dumpCandidates(*coll);
  }


  if (dumpJets) {  
    Handle<pat::JetCollection> jets;
    event.getByToken(jetToken, jets);

    cout << "Jets (only for pT>30):" << endl;
    for( pat::JetCollection::const_iterator jet = jets->begin(); jet != jets->end(); ++jet ) {
      if(jet->pt()>30){
	int i = distance(jets->begin(),jet);
	cout << "#" << i << " pt=" << jet->pt() << " eta=" << jet->eta() << " phi=" << jet->phi() << " combinedInclusiveSecondaryVertexV2BJetTags=" << jet->bDiscriminator("combinedInclusiveSecondaryVertexV2BJetTags");
	dumpUserVal(*jet);
	cout << endl;
      }
    }
  }


  // Print passing triggers
  if (listTriggers) {
    Handle<TriggerResults> triggerResults;
    if (event.getByToken(triggerResultToken, triggerResults)) {
      edm::TriggerNames const* trigNames_;  
      trigNames_ = &event.triggerNames(*triggerResults);
      cout << "Trigger bits:" << endl;
      for (unsigned int i=0; i<triggerResults->size(); i++) {
	if (triggerResults->accept(i)) cout << "   " <<
	  trigNames_->triggerName(i) << endl;
      }
    }
  }
 

}

void dumpUserData::dumpCandidates(const View<pat::CompositeCandidate>& cands) {
  for( View<pat::CompositeCandidate>::const_iterator cand = cands.begin(); cand != cands.end(); ++ cand ) {
    int i = distance(cands.begin(),cand);
    cout << "#" << i << " mass: " << cand->mass() << " m0=" << cand->daughter(0)->mass() << " m1=" << cand->daughter(1)->mass()
	 << " pt0: " <<  cand->daughter(0)->pt() << " pt1: " <<  cand->daughter(1)->pt()
	 << " id1: " << cand->daughter(0)->pdgId() << " id2: " << cand->daughter(1)->pdgId();
    dumpUserVal(*cand);
    cout << endl;
  }
}

template<typename T> 
void dumpUserData::dumpUserVal(const T& cand) {
  const std::vector<std::string> & userLabels = cand.userFloatNames();
  //  copy(userLabels.begin(), userLabels.end(), ostream_iterator<string>(cout, " "));
  for (std::vector<std::string>::const_iterator name = userLabels.begin(); name!= userLabels.end(); ++name){
    cout << " " << *name << "=" << cand.userFloat(*name) << " ";
  }


   const std::vector<std::string> & userILabels = cand.userIntNames();
  //  copy(userLabels.begin(), userLabels.end(), ostream_iterator<string>(cout, " "));
  for (std::vector<std::string>::const_iterator name = userILabels.begin(); name!= userILabels.end(); ++name){
    cout << " " << *name << "=" << cand.userInt(*name) << " ";
  }

}


#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(dumpUserData);

