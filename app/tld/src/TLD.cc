#include "TLD.hh"
#include "G4SystemOfUnits.hh"
#include "GeoSvc.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Sphere.hh"
#include "G4UnionSolid.hh"
#include "G4ProductionCuts.hh"
#include "TLDSD.hh"
#include "ConfigSvc.hh"
#include "G4UserLimits.hh"
#include "NTupleEventAnalisys.hh"
#include "RunAnalysis.hh"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <vector>
#include "Services.hh"
#include "G4VisExtent.hh"  // for GetExtent()

namespace {
G4Mutex TldMutex = G4MUTEX_INITIALIZER;
}

G4double TLD::SIZE = 4.5 * mm;

G4bool TLD::m_set_tld_scorer = true;
G4bool TLD::m_set_tld_voxelised_scorer = true;

////////////////////////////////////////////////////////////////////////////////
// TLD::CellScorer
// Enable or disable cell scorer for TLD
////////////////////////////////////////////////////////////////////////////////
/// static
void TLD::CellScorer(G4bool val) {
  m_set_tld_scorer = val;
}

////////////////////////////////////////////////////////////////////////////////
// TLD::CellVoxelisedScorer
// Enable or disable voxelised cell scorer for TLD
////////////////////////////////////////////////////////////////////////////////
/// static
void TLD::CellVoxelisedScorer(G4bool val) {
  m_set_tld_voxelised_scorer = val;
  if (!val) {
    Service<RunSvc>()->GetScoringTypes().erase(Scoring::Type::Voxel);
  }
}

////////////////////////////////////////////////////////////////////////////////
// TLD constructor
////////////////////////////////////////////////////////////////////////////////
TLD::TLD(const G4String& label, const G4ThreeVector& centre, G4String tldMediumName, G4String stlGeometryFilePath)
    : VPatient(label), m_tld_medium(tldMediumName), m_stl_geometry_file_path(stlGeometryFilePath), m_centre(centre) {
}

////////////////////////////////////////////////////////////////////////////////
// TLD destructor
////////////////////////////////////////////////////////////////////////////////
TLD::~TLD() {
  Destroy();
  if (m_step_limit) delete m_step_limit;
}

void TLD::SetIDs(G4int x, G4int y, G4int z) {
  m_id_x = x;
  m_id_y = y;
  m_id_z = z;
}

////////////////////////////////////////////////////////////////////////////////
// Write info (TODO)
////////////////////////////////////////////////////////////////////////////////
void TLD::WriteInfo() { INFO_GEO("The Dose3D cell {} info: Implement me.", GetName()); }

////////////////////////////////////////////////////////////////////////////////
// Destroy volumes and clean up
////////////////////////////////////////////////////////////////////////////////
void TLD::Destroy() {
  INFO_GEO("Destroying the TLD volume.");
}

////////////////////////////////////////////////////////////////////////////////
// Set number of voxels along an axis
////////////////////////////////////////////////////////////////////////////////
void TLD::SetNVoxels(char axis, int nv) {
  switch (std::tolower(axis)) {
    case 'x':
      m_tld_voxelization_x = nv;
      break;
    case 'y':
      m_tld_voxelization_y = nv;
      break;
    case 'z':
      m_tld_voxelization_z = nv;
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Construct TLD geometry and place in the world
////////////////////////////////////////////////////////////////////////////////
void TLD::Construct(G4VPhysicalVolume* parentWorld) {
  auto label = GetName();
  auto material = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", m_tld_medium);

  G4ThreeVector size(TLD::SIZE, TLD::SIZE, 0.9 * mm);
  G4LogicalVolume* tldLV = nullptr;

  if (m_stl_geometry_file_path != "None") {
    std::string path = std::string(PROJECT_DATA_PATH) + "/" + m_stl_geometry_file_path;

    std::ifstream fin(path, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(fin)), {});
    fin.close();
    const std::string bom = "ï»¿";
    if (content.rfind(bom, 0) == 0) {
      content.erase(0, bom.size());
    }
    std::string tmpPath = path + ".clean.stl";
    std::ofstream fout(tmpPath, std::ios::binary);
    fout << content;
    fout.close();
    m_stlMesh = CADMesh::TessellatedMesh::FromSTL(tmpPath);
    G4VSolid* solidMesh = m_stlMesh->GetSolid();
    solidMesh->SetName(label + "Solid");
    tldLV = new G4LogicalVolume(solidMesh, material.get(), label + "LV");
  } else {
    G4double innerR = 0.0;
    G4double outerR = size.x() / 2.0;
    G4double halfH = size.z() / 2.0;
    G4double startAng = 0.0;
    G4double spanAng = 360.0 * deg;

    G4VSolid* tube = new G4Tubs(label + "Tube", innerR, outerR, halfH, startAng, spanAng);

    G4double thetaMin = 0.0 * deg;
    G4double thetaMax = 90.0 * deg;
    G4VSolid* cap = new G4Sphere(label + "SphereCap", 0.0, outerR,
                                 0.0, 360.0 * deg,
                                 thetaMin, thetaMax);

    G4ThreeVector capOffset(0, 0, halfH);
    G4VSolid* combinedSolid = new G4UnionSolid(label + "CombinedSolid", tube, cap, nullptr, capOffset);
    tldLV = new G4LogicalVolume(combinedSolid, material.get(), label + "LV");
  }
  m_global_centre = m_centre;

  m_centre = svc::transformPosition(m_global_centre, this, svc::Transform::GlobalToLocal);
  SetPhysicalVolume(new G4PVPlacement(nullptr, m_centre, label + "PV", tldLV, parentWorld, false, 0));

  auto regVol = new G4Region(label + "_G4RegionCuts");
  auto cuts = new G4ProductionCuts();
  cuts->SetProductionCut(0.1 * mm);
  regVol->SetProductionCuts(cuts);
  tldLV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(tldLV);

  SetVolume(size.getX() * size.getY() * size.getZ());
}

////////////////////////////////////////////////////////////////////////////////
// Update (placeholder)
////////////////////////////////////////////////////////////////////////////////
G4bool TLD::Update() { return true; }

////////////////////////////////////////////////////////////////////////////////
// Check if sensitive detector is voxelised
////////////////////////////////////////////////////////////////////////////////
bool TLD::IsRunCollectionScoringVolumeVoxelised(const G4String& run_collection) const {
  return GetSD()->GetRunCollectionReferenceScoringVolume(run_collection, true) != nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// Define sensitive detector and scoring volume
////////////////////////////////////////////////////////////////////////////////
void TLD::DefineSensitiveDetector() {
  G4AutoLock lock(&TldMutex);
  if (!m_patientSD.Get()) {
    auto pv = GetPhysicalVolume();
    auto lv = pv->GetLogicalVolume();
    G4VSolid* solid = lv->GetSolid();

    G4VisExtent vExtent = solid->GetExtent();
    G4double xmin = vExtent.GetXmin();
    G4double xmax = vExtent.GetXmax();
    G4double ymin = vExtent.GetYmin();
    G4double ymax = vExtent.GetYmax();
    G4double zmin = vExtent.GetZmin();
    G4double zmax = vExtent.GetZmax();

    G4double halfX = 0.5 * (xmax - xmin);
    G4double halfY = 0.5 * (ymax - ymin);
    G4double halfZ = 0.5 * (zmax - zmin);

    auto envBox = G4Box(GetName() + "ScoringBox", halfX, halfY, halfZ);
    G4double offsetX = 0.5 * (xmin + xmax);
    G4double offsetY = 0.5 * (ymin + ymax);
    G4double offsetZ = 0.5 * (zmin + zmax);
    G4ThreeVector sdCentre = m_centre + G4ThreeVector(offsetX, offsetY, offsetZ);

    auto label = GetName();
    m_patientSD.Put(new TLDSD(label + "_SD", sdCentre, m_id_x, m_id_y, m_id_z, m_surface_scoring_layers));
    auto patientSD = m_patientSD.Get();

    G4int nvx = 1, nvy = 1, nvz = 1;
    if (TLD::m_set_tld_voxelised_scorer) {
      nvx = m_tld_voxelization_x;
      nvy = m_tld_voxelization_y;
      nvz = m_tld_voxelization_z;
    }

    std::string runCollName = label.substr(0, label.find('_'));
    std::string hcName = label + "_HC";

    G4cout << "[DEBUG] TLD::DefineSensitiveDetector name=" << label
           << " runColl=" << runCollName
           << " centre=" << m_global_centre
           << " voxels=" << nvx << "," << nvy << "," << nvz
           << " surface_layers=" << m_surface_scoring_layers
           << G4endl;

    patientSD->AddScoringVolume(runCollName, hcName, envBox, nvx, nvy, nvz);
    VPatient::SetSensitiveDetector(label + "LV", patientSD);
  }
}
