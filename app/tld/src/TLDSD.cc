#include "TLDSD.hh"
#include "TLD.hh"
#include "StepAnalysis.hh"
#include <G4VProcess.hh>
#include "Services.hh"
#include "PatientTrackInfo.hh"
#include "PrimaryParticleInfo.hh"
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
///
TLDSD::TLDSD(const G4String& sdName, const G4ThreeVector& centre, G4int idX, G4int idY, G4int idZ, G4int surfaceScoringLayers)
:VPatientSD(sdName,centre), m_surface_scoring_layers(std::max(0, surfaceScoringLayers)){
  m_id_x = idX;
  m_id_y = idY;
  m_id_z = idZ;
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool TLDSD::IsSurfaceVoxel(const ScoringVolume* scoringVolumePtr, G4int voxelIdX, G4int voxelIdY, G4int voxelIdZ) const {
  if (m_surface_scoring_layers <= 0 || scoringVolumePtr == nullptr || !scoringVolumePtr->IsVoxelised())
    return true;

  const auto layers = m_surface_scoring_layers;
  const auto nx = scoringVolumePtr->m_nVoxelsX;
  const auto ny = scoringVolumePtr->m_nVoxelsY;
  const auto nz = scoringVolumePtr->m_nVoxelsZ;

  auto isAxisSurface = [layers](G4int idx, G4int n) {
    if (n <= 0)
      return false;
    return idx < layers || idx >= (n - layers);
  };

  return isAxisSurface(voxelIdX, nx) ||
         isAxisSurface(voxelIdY, ny) ||
         isAxisSurface(voxelIdZ, nz);
}

////////////////////////////////////////////////////////////////////////////////
///
void TLDSD::ProcessHitsCollectionSurfaceAware(const G4String& hitsCollectionName, G4Step* aStep){
  auto position = aStep->GetPreStepPoint()->GetPosition();
  auto scoringVolumePtr = GetScoringVolumePtr(hitsCollectionName);
  auto inScoringVolume = scoringVolumePtr->IsInside(position);
  auto isOnBorder = scoringVolumePtr->IsOnBorder(position);

  if (!inScoringVolume || (isOnBorder && ((aStep->GetTotalEnergyDeposit())==0.))) {
    if (!isOnBorder && aStep->GetTotalEnergyDeposit() > 0) {
      G4cout << "hit: " << position << " cell(" << m_id_x << "," << m_id_y << "," << m_id_z
             << ") ScoringBox x(" << scoringVolumePtr->m_rangeMinX << " - " << scoringVolumePtr->m_rangeMaxX
             << ") y(" << scoringVolumePtr->m_rangeMinY << " - " << scoringVolumePtr->m_rangeMaxY
             << ") z(" << scoringVolumePtr->m_rangeMinZ << " - " << scoringVolumePtr->m_rangeMaxZ
             << ")" << G4endl;
    }
    return;
  }

  auto voxelIdX = scoringVolumePtr->GetVoxelID(0, position);
  auto voxelIdY = scoringVolumePtr->GetVoxelID(1, position);
  auto voxelIdZ = scoringVolumePtr->GetVoxelID(2, position);

  if (!IsSurfaceVoxel(scoringVolumePtr, voxelIdX, voxelIdY, voxelIdZ))
    return;

  auto voxelId = scoringVolumePtr->LinearizeIndex(voxelIdX, voxelIdY, voxelIdZ);

  if (voxelId >= 0 && voxelId < static_cast<G4int>(scoringVolumePtr->m_channelHCollectionIndex.size())) {
    if (scoringVolumePtr->m_channelHCollectionIndex[voxelId] == -1) {
      auto voxelHit = new VoxelHit();
      voxelHit->SetVolume(scoringVolumePtr->GetVoxelVolume());
      voxelHit->SetCentre(scoringVolumePtr->GetVoxelCentre(voxelId));
      voxelHit->SetId(voxelIdX, voxelIdY, voxelIdZ);
      voxelHit->SetGlobalId(m_id_x, m_id_y, m_id_z);
      voxelHit->SetStoreTracks(Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StoreTracks"));
      voxelHit->SetGlobalCentre(GetSDCentre());
      voxelHit->Fill(aStep);
      voxelHit->FillTrackUserInfo<PatientTrackInfo>(aStep);
      voxelHit->FillPrimaryParticleUserInfo<PrimaryParticleInfo>();

      auto channelHCollectionIndex = scoringVolumePtr->m_voxelHCollectionPtr->insert(voxelHit) - 1;
      scoringVolumePtr->m_channelHCollectionIndex[voxelId] = channelHCollectionIndex;
    }
    else {
      auto channelHCollectionIndex = scoringVolumePtr->m_channelHCollectionIndex[voxelId];
      auto voxelHit = (*scoringVolumePtr->m_voxelHCollectionPtr)[channelHCollectionIndex];
      voxelHit->Update(aStep);
      voxelHit->FillTrackUserInfo<PatientTrackInfo>(aStep);
    }
  } else {
    auto maxId = static_cast<G4int>(scoringVolumePtr->m_channelHCollectionIndex.size()) - 1;
    DEBUG_GEO("Out of scope ChannelId: {}. Max voxel ID is: {}.\nPosition: {}\nIdX={}, IdY={}, IdZ={}",
              voxelId, maxId, position, voxelIdX, voxelIdY, voxelIdZ);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// This method is being called for each G4Step in sensitive volume
G4bool TLDSD::ProcessHits(G4Step* aStep, G4TouchableHistory*) {
  auto theTouchable = dynamic_cast<const G4TouchableHistory *>(aStep->GetPreStepPoint()->GetTouchable());
  auto volumeName = theTouchable->GetVolume()->GetName();
  if (!G4StrUtil::contains(volumeName, "TLD"))
    DEBUG_GEO("ProcessHits volume name ", volumeName);
  
  if (Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StoreTracks")) {
    auto aTrack = aStep->GetTrack();
    auto trackInfo = aTrack->GetUserInformation();
    if(trackInfo){
      dynamic_cast<PatientTrackInfo*>(trackInfo)->FillInfo(aStep);
    }
    else{
      trackInfo = new PatientTrackInfo();
      dynamic_cast<PatientTrackInfo*>(trackInfo)->FillInfo(aStep);
      aTrack->SetUserInformation(trackInfo);
    }
  }

  auto thisSdHCNames = GetScoringVolumeNames();
  for(const auto& hcName : ControlPoint::GetHitCollectionNames()){
    if(find(thisSdHCNames.begin(), thisSdHCNames.end(), hcName) != thisSdHCNames.end()) {
      auto* scoringVolume = GetScoringVolumePtr(hcName);
      if (m_surface_scoring_layers > 0 && scoringVolume != nullptr && scoringVolume->IsVoxelised())
        ProcessHitsCollectionSurfaceAware(hcName, aStep);
      else
        ProcessHitsCollection(hcName, aStep);
    }
  }
  return true;
}
