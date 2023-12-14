//
// Created by brachwal on 30.05.2020.
//

#ifndef D3D_EVENT_ANALYSIS_HH
#define D3D_EVENT_ANALYSIS_HH

#include "globals.hh"
#include <vector>
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4Cache.hh"
#include "VoxelHit.hh"
#include "tls.hh"

class G4Event;
class G4Run;

class NTupleEventAnalisys {

  private:
    ///
    NTupleEventAnalisys() = default;

    ///
    ~NTupleEventAnalisys() = default;

    /// Delete the copy and move constructors
    NTupleEventAnalisys(const NTupleEventAnalisys &) = delete;

    NTupleEventAnalisys &operator=(const NTupleEventAnalisys &) = delete;

    NTupleEventAnalisys(NTupleEventAnalisys &&) = delete;

    NTupleEventAnalisys &operator=(NTupleEventAnalisys &&) = delete;

    ///
    class TTreeEventCollection {
      public:
        G4int m_evtId = -1;
        G4int m_ntupleId = -1;
        std::map<G4String, G4int> m_colId; // variable index within NTuples structure
        bool m_cell_tree_structure = false;
        bool m_voxel_tree_structure = false;
        bool m_cell_in_voxel_tree_structure = false;
        bool m_tracks_analysis = false;
        G4double m_global_time = 0.;
        G4int m_EvtPrimariesN = 0;
        // G4double m_totalDose = 0.;
        std::vector<G4int>    m_CellIdX, m_CellIdY, m_CellIdZ;
        std::vector<G4int>    m_VoxelIdX, m_VoxelIdY, m_VoxelIdZ;
        std::vector<G4int> m_VoxelHitIdX, m_VoxelHitIdY, m_VoxelHitIdZ;
        std::vector<G4double> m_CellIDose;
        std::vector<G4double> m_VoxelHitEDeposit, m_VoxelHitMeanEDeposit, m_VoxelHitDose, m_PrimaryTrkEnergy, m_EvtTime; 
        std::vector<G4double> m_G4EvtPrimaryEnergy;
        std::vector<G4double> m_VoxelPositionX, m_VoxelPositionY, m_VoxelPositionZ;
        std::vector<G4double> m_CellPositionX, m_CellPositionY, m_CellPositionZ;
        std::vector<std::vector<G4int>> m_VoxelHitsTrkId; std::vector<G4int> m_VoxelTrkId;     // primary / secondary
        std::vector<std::vector<G4int>> m_VoxelHitsTrkTypeId; std::vector<G4int> m_VoxelTrkTypeId;// 1:G4Gamma, 2:G4Electron, 3:G4Positron, 4:G4Neutron, 5:G4Proton
        std::vector<std::vector<G4double>> m_VoxelHitsTrkEnergy; std::vector<G4double> m_VoxelTrkEnergy;
        std::vector<std::vector<G4double>> m_VoxelHitsTrkTheta; std::vector<G4double> m_VoxelTrkTheta;
        std::vector<std::vector<G4double>> m_VoxelHitsTrkLength; std::vector<G4double> m_VoxelTrkLength;
        std::vector<std::vector<G4double>> m_VoxelHitsTrkPosX; std::vector<G4double> m_VoxelTrkPositionX;
        std::vector<std::vector<G4double>> m_VoxelHitsTrkPosY; std::vector<G4double> m_VoxelTrkPositionY;
        std::vector<std::vector<G4double>> m_VoxelHitsTrkPosZ; std::vector<G4double> m_VoxelTrkPositionZ;

    };

    ///
    class TTreeCollection {
      public:
        G4String m_name;
        G4String m_description;
        bool m_tracks_analysis = false;
        /// Single or many HitsCollections can be related to single tree,
        /// see NTupleEventAnalisys::DefineTTree definition
        std::vector<G4String> m_hc_names;
    };


    ///
    G4int m_runId = -1;

    ///
    G4double m_degree_rotation = -10000.;

    ///
    G4String m_treeNamePostfix = "TTree";

    
    // std::map<std::string, G4double> cellSumDose;
    // std::map<std::string, G4double> voxelSumDose;
    // // std::string doseMapName = "";

    ///
    static std::vector<TTreeCollection> m_ttree_collection;

    ///
    G4MapCache<G4String,TTreeEventCollection> m_ntuple_collection;

    ///
    void CreateNTuple(const TTreeCollection& treeColl);

    ///
    G4int GetNTupleId(const G4String& treeName);

    ///
    void FillEventCollection(const G4String& treeName, const G4Event *evt, VoxelHitsCollection* hitsColl);

    ///
    void FillNTupleEvent();

    ///
    void ClearEventCollections();



  public:
    ///
    static NTupleEventAnalisys* GetInstance();

    ///
    void BeginOfRun(const G4Run* runPtr, G4bool isMaster);

    ///
    void EndOfEventAction(const G4Event *evt);

    /// Once the scoringVolumeName remain empty it means that tree is related to single hits collection
    static void DefineTTree(const G4String& treeName, const G4String& treeDescription, const G4String& scoringVolumeName=G4String());

    ///
    static void SetTracksAnalysis(const G4String& treeName, bool flag);

    ///
    static G4bool IsAnyTTreeDefined() { return m_ttree_collection.empty() ? false:true; }


    static const std::vector<TTreeCollection>& TreeCollection() { return m_ttree_collection; }

    /// 
    // const std::map<std::string, G4double>& GetSummedVoxelDose() {return voxelSumDose;}
    // ///
    // const std::map<std::string, G4double>& GetSummedCellDose() {return cellSumDose;}

};
#endif //D3D_EVENT_ANALYSIS_HH
