// -*- C++ -*-
//
// Package:    CMGTools/WTau3Mu
// Class:      L1MuonRecoPropagator
// 
/**\class L1MuonRecoPropagator L1MuonRecoPropagator.cc CMGTools/WTau3Mu/src/L1MuonRecoPropagator.cc

 Description: for each PAT muon extrapolates its coordinates to the second muon station, 
              so that the offline-L1 matching is done on an equal footing.
              Saves a map: each offline muon is associated to the extrapolated 4 vector. 
              
              Very much inspired to 
              https://github.com/cms-l1-dpg/Legacy-L1Ntuples/blob/6b1d8fce0bd2058d4309af71b913e608fced4b17/src/L1MuonRecoTreeProducer.cc

*/
//
// Original Author:  Riccardo Manzoni
//
//

#include <memory>

// framework
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Utilities/interface/InputTag.h"

// Muons & Tracks Data Formats
#include "DataFormats/GeometrySurface/interface/Cylinder.h"
#include "DataFormats/GeometrySurface/interface/Plane.h"
#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/MuonReco/interface/MuonFwd.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"

// Cond Formats
#include "CondFormats/AlignmentRecord/interface/TrackerSurfaceDeformationRcd.h"

// Transient tracks (for extrapolations)
#include "TrackingTools/GeomPropagators/interface/Propagator.h"
#include "TrackingTools/Records/interface/TrackingComponentsRecord.h"
#include "TrackingTools/TrajectoryState/interface/FreeTrajectoryState.h"
#include "TrackingTools/TrajectoryState/interface/TrajectoryStateOnSurface.h"
#include "TrackingTools/TrajectoryState/interface/TrajectoryStateTransform.h"

// B Field
#include "MagneticField/Engine/interface/MagneticField.h"


namespace cmg{
  class L1MuonRecoPropagator : public edm::EDProducer {
    public:
      explicit L1MuonRecoPropagator(const edm::ParameterSet & iConfig);
      virtual ~L1MuonRecoPropagator() { }
  
      virtual void produce(edm::Event & iEvent, const edm::EventSetup& iSetup) override;
  
      TrajectoryStateOnSurface  cylExtrapTrkSam  (reco::TrackRef track, double rho);
      TrajectoryStateOnSurface  surfExtrapTrkSam (reco::TrackRef track, double z);
    
    private:

      // The Magnetic field
      edm::ESHandle<MagneticField> theBField;
       
      // reco muons
      edm::Handle<pat::MuonCollection> mucand;

      // Extrapolator to cylinder
      edm::ESHandle<Propagator> propagatorAlong;
      edm::ESHandle<Propagator> propagatorOpposite;
    
      FreeTrajectoryState freeTrajStateMuon(reco::TrackRef track);
      
      // Trajectory on surface
      TrajectoryStateOnSurface tsos;

  };
}

cmg::L1MuonRecoPropagator::L1MuonRecoPropagator(const edm::ParameterSet & iConfig)
{ }


void 
cmg::L1MuonRecoPropagator::produce(edm::Event & iEvent, const edm::EventSetup & iSetup)
{
    
  // Get the muon candidates
  iEvent.getByLabel("slimmedMuons", mucand);

  // Get the Magnetic field from the setup
  iSetup.get<IdealMagneticFieldRecord>().get(theBField);

  // Get the propagators 
  iSetup.get<TrackingComponentsRecord>().get("SmartPropagatorAny"        , propagatorAlong   );
  iSetup.get<TrackingComponentsRecord>().get("SmartPropagatorAnyOpposite", propagatorOpposite);
  
  for(pat::MuonCollection::const_iterator imu = mucand->begin(); imu != mucand->end(); imu++){
    
	// Take the tracker track and build a transient track out of it
	reco::TrackRef tr_mu = imu->innerTrack();  

	tsos = surfExtrapTrkSam(tr_mu, 790);   // track at ME2+ plane - extrapolation
	if (tsos.isValid()) {
	  double xx = tsos.globalPosition().x();
	  double yy = tsos.globalPosition().y();
	  double rr = sqrt(xx*xx + yy*yy);
	  // muonData->tr_r_me2_p.push_back(rr);
	  double cosphi = xx/rr;
	  // if (yy>=0) 
	  //   muonData->tr_phi_me2_p.push_back(acos(cosphi));
	  // else
	  //   muonData->tr_phi_me2_p.push_back(2*pig-acos(cosphi));	
	}
	else{
	  // muonData->tr_r_me2_p.push_back(-999999);
	  // muonData->tr_phi_me2_p.push_back(-999999);
	}

	tsos = surfExtrapTrkSam(tr_mu, -790);   // track at ME2- plane - extrapolation
	if (tsos.isValid()) {
	  double xx = tsos.globalPosition().x();
	  double yy = tsos.globalPosition().y();
	  double rr = sqrt(xx*xx + yy*yy);
	  // muonData->tr_r_me2_n.push_back(rr);
	  double cosphi = xx/rr;
	  // if (yy>=0) 
	  //   muonData->tr_phi_me2_n.push_back(acos(cosphi));
	  // else
	  //   muonData->tr_phi_me2_n.push_back(2*pig-acos(cosphi));	
	}
	else{
	  // muonData->tr_r_me2_n.push_back(-999999);
	  // muonData->tr_phi_me2_n.push_back(-999999);
	}

	tsos = cylExtrapTrkSam(tr_mu, 500);  // track at MB2 radius - extrapolation
	if (tsos.isValid()) {
	  double xx = tsos.globalPosition().x();
	  double yy = tsos.globalPosition().y();
	  double zz = tsos.globalPosition().z();
	  // muonData->tr_z_mb2.push_back(zz);	
	  double rr = sqrt(xx*xx + yy*yy);
	  double cosphi = xx/rr;
	  // if (yy>=0) 
	  //   muonData->tr_phi_mb2.push_back(acos(cosphi));
	  // else
	  //   muonData->tr_phi_mb2.push_back(2*pig-acos(cosphi));
	}
	else{
	  // muonData->tr_z_mb2.push_back(-999999);
	  // muonData->tr_phi_mb2.push_back(-999999);
	}
    
  }

}

FreeTrajectoryState 
cmg::L1MuonRecoPropagator::freeTrajStateMuon(reco::TrackRef track)
{

  GlobalPoint  innerPoint(track->innerPosition().x(), track->innerPosition().y(),  track->innerPosition().z());
  GlobalVector innerVec  (track->innerMomentum().x(), track->innerMomentum().y(),  track->innerMomentum().z());  
  
  FreeTrajectoryState recoStart(innerPoint, innerVec, track->charge(), &*theBField);
  
  return recoStart;
}

TrajectoryStateOnSurface
cmg::L1MuonRecoPropagator::surfExtrapTrkSam(reco::TrackRef track, double z)
{
  Plane::PositionType pos(0, 0, z);
  Plane::RotationType rot;
  Plane::PlanePointer myPlane = Plane::build(pos, rot);

  FreeTrajectoryState recoStart = freeTrajStateMuon(track);
  TrajectoryStateOnSurface recoProp;
  recoProp = propagatorAlong->propagate(recoStart, *myPlane);
  if (!recoProp.isValid()) {
    recoProp = propagatorOpposite->propagate(recoStart, *myPlane);
  }
  return recoProp;
}

TrajectoryStateOnSurface 
cmg::L1MuonRecoPropagator::cylExtrapTrkSam(reco::TrackRef track, double rho)
{
  Cylinder::PositionType pos(0, 0, 0);
  Cylinder::RotationType rot;
  Cylinder::CylinderPointer myCylinder = Cylinder::build(pos, rot, rho);

  FreeTrajectoryState recoStart = freeTrajStateMuon(track);
  TrajectoryStateOnSurface recoProp;
  recoProp = propagatorAlong->propagate(recoStart, *myCylinder);
  if (!recoProp.isValid()) {
    recoProp = propagatorOpposite->propagate(recoStart, *myCylinder);
  }
  return recoProp;
}

//define this as a plug-in
DEFINE_FWK_MODULE(cmg::L1MuonRecoPropagator);