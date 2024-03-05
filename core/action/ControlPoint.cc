#include "ControlPoint.hh"
#include "Services.hh"
#include "NTupleEventAnalisys.hh"
#include "RunAnalysis.hh"
#include "IO.hh"
#include "TFile.h"
#include "TTree.h"
#include "TChain.h"
#include "D3DCell.hh"
#include "G4SDManager.hh"
#ifdef G4MULTITHREADED
    #include "G4Threading.hh"
    #include "G4MTRunManager.hh"
#endif

double ControlPoint::FIELD_MASK_POINTS_DISTANCE = 0.5;
std::string ControlPoint::m_sim_dir = "sim";

namespace {
    G4Mutex CPMutex = G4MUTEX_INITIALIZER;
}

////////////////////////////////////////////////////////////////////////////////
///
ControlPointConfig::ControlPointConfig(int id, int nevts, double rot)
: Id(id), NEvts(nevts),RotationInDeg(rot){}

////////////////////////////////////////////////////////////////////////////////
/// TODO:: Jesli komorka nie jest voxelised nie ma sensu dodawac Scoring::Voxel
void ControlPointRun::InitializeScoringCollection(){
    std::string worker = G4Threading::IsWorkerThread() ? "worker" : "master";
    auto scoring_types = Service<RunSvc>()->GetScoringTypes();
    ///
    /// !!!! TODO:: tobe removed by store only in control Point!!
    /// RunAnalysis::GetRunCollection() jest to nadmiarowe!!!
    /// Albo taki storage powinien byc tutaj prywatny!!!
    ///
    auto run_collections = RunAnalysis::GetRunCollection(); 
    LOGSVC_INFO("Run scoring initialization for #{} collections ({})",run_collections.size(),worker);
    for(const auto& scoring : run_collections){
        auto scoring_name = scoring.first;
        for(const auto& scoring_type: scoring_types){
            LOGSVC_INFO("Adding new scoring map: {}/{}",scoring_name,Scoring::to_string(scoring_type));
            if(m_hashed_scoring_map.find(scoring_name)==m_hashed_scoring_map.end())
                m_hashed_scoring_map.insert(std::pair<G4String,ScoringMap>(scoring_name,ScoringMap()));
            auto& scoring_collection = m_hashed_scoring_map.at(scoring_name);
            scoring_collection[scoring_type] = Service<GeoSvc>()->Patient()->GetScoringHashedMap(scoring_name,scoring_type);
            if(scoring_collection[scoring_type].empty()){
                LOGSVC_INFO("Erasing {}",Scoring::to_string(scoring_type));
                scoring_collection.erase(scoring_type);
            }
            else
                LOGSVC_INFO("Scoring collection size: {}",scoring_collection.at(scoring_type).size());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPointRun::Merge(const G4Run* worker_run){
    LOGSVC_INFO("Run-{} merging...",worker_run->GetRunID());
    auto cell_size = D3DCell::SIZE;
    auto cell_volume = cell_size*cell_size*cell_size;
    auto merge = [&](ScoringMap& left, const ScoringMap& right){
        for(auto& scoring : left){
            G4double total_dose(0);
            auto& type = scoring.first;
            bool isVoxel = type == Scoring::Type::Voxel ? true : false;
            LOGSVC_INFO("Scoring type: {}",Scoring::to_string(type));
            auto& hashed_scoring_left = scoring.second;
            const auto& hashed_scoring_right = right.at(type);
            for(auto& hashed_voxel : hashed_scoring_left){
                
                hashed_voxel.second.Cumulate(hashed_scoring_right.at(hashed_voxel.first),isVoxel); // VoxelHit+=VoxelHit
                auto voxel_volume = hashed_voxel.second.GetVolume();
                    
                if(isVoxel && voxel_volume < cell_volume){
                    //LOGSVC_INFO("Voxel / Cell volume: {} / {}",voxel_volume,cell_volume);
                    total_dose += hashed_voxel.second.GetDose()*voxel_volume/cell_volume;
                } else {
                    total_dose += hashed_voxel.second.GetDose();
                }

                // if(isVoxel){
                //     G4cout<<G4endl;
                //     // hashed_voxel.second.Print();
                //     // G4cout<<G4endl;
                //     // hashed_scoring_right.at(hashed_voxel.first).Print();
                //     G4cout<< "Dose: " <<hashed_scoring_right.at(hashed_voxel.first).GetDose() << G4endl;
                //     G4cout<< "Total: " << total_dose << G4endl;
                //     G4cout<<G4endl;
                // }
            }
            LOGSVC_INFO("Total dose: {}",total_dose);
        } 
    };

    for(auto& scoring : m_hashed_scoring_map){
        auto scoring_name = scoring.first;
        LOGSVC_INFO("Merging collection: {}",scoring_name);
        auto& master_scoring = scoring.second;
        // LOGSVC_DEBUG("Master scoring #types: {}",master_scoring.size());
        const auto& worker_scoring = dynamic_cast<const ControlPointRun*>(worker_run)->m_hashed_scoring_map.at(scoring_name);
        // LOGSVC_DEBUG("Worker scoring #types: {}",worker_scoring.size());
        merge(master_scoring,worker_scoring);
    }

    // Realease memory after merging... we don't need this anymore.
    dynamic_cast<const ControlPointRun*>(worker_run)->m_hashed_scoring_map.clear();

}

////////////////////////////////////////////////////////////////////////////////
///
ScoringMap& ControlPointRun::GetScoringCollection(const G4String& name){
    if (m_hashed_scoring_map.find(name) == m_hashed_scoring_map.end())
        LOGSVC_ERROR("Couldn't find scoring collection in current run: {}",name)
    return m_hashed_scoring_map.at(name);
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPointRun::EndOfRun(){
    if(m_hashed_scoring_map.size()>0){
        LOGSVC_INFO("ControlPointRun::EndOfRun...");
        FillDataTagging();
    }
    else {
        LOGSVC_INFO("ControlPointRun::EndOfRun:: Nothing to do.");
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPointRun::FillDataTagging(){
    auto current_cp = Service<RunSvc>()->CurrentControlPoint();
    for(auto& scoring_map: m_hashed_scoring_map){
        LOGSVC_INFO("ControlPointRun::Filling data tagging for {} run collection",scoring_map.first);
        auto& hashed_scoring_map = scoring_map.second;

        std::vector<const VoxelHit*> in_field_scoring_volume;

        auto getActivityGeoCentre = [&](bool weighted){
            G4ThreeVector sum{0,0,0};
            G4double total_dose{0};
            std::for_each(  in_field_scoring_volume.begin(),
                            in_field_scoring_volume.end(),
                            [&](const VoxelHit* iv) {
                        sum += weighted ? iv->GetCentre() * iv->GetDose() : iv->GetCentre();
                        total_dose += iv->GetDose();
                        });
            if(total_dose<1e-30){
                LOGSVC_WARN("No activity found!");
            }
            if(weighted){
                LOGSVC_INFO("getActivityGeoCentre:weighted: total dose: {}",total_dose);
                return total_dose == 0 ? sum : sum / total_dose;
            }
            else{
                auto size = in_field_scoring_volume.size();
                LOGSVC_INFO("getActivityGeoCentre: size: {}",size);
                return size > 0 ? sum / size : sum;
            }
        };

        auto fillScoringVolumeTagging = [&](VoxelHit& hit, const G4ThreeVector& geoCentre, const G4ThreeVector& wgeoCentre){
            auto mask_tag = current_cp->GetInFieldMaskTag(hit.GetCentre());
            auto geo_tag = 1./sqrt(hit.GetCentre().diff2(geoCentre));
            auto wgeo_tag = 1./sqrt(hit.GetCentre().diff2(wgeoCentre));
            hit.FillTagging(mask_tag, geo_tag, wgeo_tag);
        };

        for(auto& scoring: hashed_scoring_map){
            auto scoring_type = scoring.first;
            LOGSVC_INFO("Scoring type {}",Scoring::to_string(scoring_type));
            auto& data = scoring.second;
            in_field_scoring_volume.clear();
            for(auto& hit : data){
                if(current_cp->IsInField(hit.second.GetCentre()))
                    in_field_scoring_volume.push_back(&hit.second);
            }
            auto geo_centre = getActivityGeoCentre(false);
            LOGSVC_INFO("Geocentre: {}",geo_centre);
            auto wgeo_centre = getActivityGeoCentre(true);
            LOGSVC_INFO("WGeocentre: {}",wgeo_centre);
            for(auto& hit : data){
                fillScoringVolumeTagging(hit.second,geo_centre,wgeo_centre);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
///
ControlPoint::ControlPoint(const ControlPointConfig& config): m_config(config){
    G4cout << " DEBUG: ControlPoint:Ctr: rotation: " << config.RotationInDeg << G4endl;
    m_scoring_types = Service<RunSvc>()->GetScoringTypes();
    SetRotation(config.RotationInDeg);
    FillPlanFieldMask();
}

////////////////////////////////////////////////////////////////////////////////
///
ControlPoint::ControlPoint(const ControlPoint& cp):m_config(cp.m_config){
    m_rotation = new G4RotationMatrix(*cp.m_rotation);
    m_scoring_types = cp.m_scoring_types;
    m_plan_mask_points = cp.m_plan_mask_points;
    m_sim_mask_points = cp.m_sim_mask_points;
    m_mlc_a_positioning = cp.m_mlc_a_positioning;
    m_mlc_b_positioning = cp.m_mlc_b_positioning;
}

////////////////////////////////////////////////////////////////////////////////
///
ControlPoint::ControlPoint(ControlPoint&& cp):m_config(cp.m_config){
    m_scoring_types = cp.m_scoring_types;
    m_rotation = cp.m_rotation;
    cp.m_rotation = nullptr;
    m_plan_mask_points = cp.m_plan_mask_points;
    m_sim_mask_points = cp.m_sim_mask_points;
    m_mlc_a_positioning = cp.m_mlc_a_positioning;
    m_mlc_b_positioning = cp.m_mlc_b_positioning;
}

////////////////////////////////////////////////////////////////////////////////
///
ControlPoint::~ControlPoint() {
    if (m_rotation) delete m_rotation; m_rotation = nullptr;
    // for(auto run : m_mt_run){
    //     delete run;
    //     //TODO m_mt_run.erase(run);
    // }
};

////////////////////////////////////////////////////////////////////////////////
/// Multi-thread safe method
G4Run* ControlPoint::GenerateRun(bool scoring){
    G4AutoLock lock(&CPMutex);
    m_mt_run.push_back(new ControlPointRun(scoring));
    m_cp_run.Put(m_mt_run.back());
    return m_mt_run.back();
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPoint::SetRotation(double rotationInDegree) { 
    m_config.RotationInDeg = rotationInDegree;
    if(m_rotation) 
        delete m_rotation;
    G4ThreeVector AxisOfRotation = G4ThreeVector(0.,1.,0.).unit();
    m_rotation = new G4RotationMatrix();
    // Geant4::SystemOfUnitsconst::rad = 1.;
    // Geant4::SystemOfUnitsconst::deg = rad*pi/180.;
    m_rotation->rotate(rotationInDegree*deg, AxisOfRotation); // convert deg to rad
}

////////////////////////////////////////////////////////////////////////////////
///
std::string ControlPoint::GetOutputDir() {
    auto dir = Service<ConfigSvc>()->GetValue<std::string>("RunSvc","OutputDir")+
                "/"+m_sim_dir;
    IO::CreateDirIfNotExits(dir);
    return dir;
}

////////////////////////////////////////////////////////////////////////////////
///
std::string ControlPoint::GetOutputFileName() const {
    auto job = Service<RunSvc>()->GetJobNameLabel();
    return GetOutputDir()+"/cp-"+ std::to_string(GetId()); // No extension here!  
}

////////////////////////////////////////////////////////////////////////////////
///
std::string ControlPoint::GetSimOutputTFileName(bool workerMT) const {
    auto numberOfThreads = Service<ConfigSvc>()->GetValue<int>("RunSvc", "NumberOfThreads");
    std::string postfix =  "_sim.root";
    if(workerMT){ // && numberOfThreads > 1
        auto subjob_dir = GetOutputDir()+"/subjobs";
        IO::CreateDirIfNotExits(subjob_dir);
        return subjob_dir+"/cp-"+std::to_string(GetId())+postfix;
    }
    
    return GetOutputFileName()+postfix;
}


////////////////////////////////////////////////////////////////////////////////
///
const std::vector<G4ThreeVector>& ControlPoint::GetFieldMask(const std::string& type) const {
    if(type=="Plan") {
        if(m_plan_mask_points.empty())
            LOGSVC_WARN("Returning empty plan mask point vector");
        return m_plan_mask_points;
    } else if(type=="Sim") {
        if(m_sim_mask_points.Get().empty())
            LOGSVC_WARN("Returning empty sim mask point vector");
        return m_sim_mask_points.Get();
    }
    G4String msg = "Couldn't recognize control point field mask type!";
    LOGSVC_CRITICAL(msg.data());
    G4Exception("ControlPoint", "GetFieldMask", FatalErrorInArgument , msg);
    return m_plan_mask_points;
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPoint::FillPlanFieldMask(){
    // It should happen once for single control point at the time
    // of the object instantation. See RunSvc::SetSimulationConfiguration
    if(m_plan_mask_points.empty()){
        LOGSVC_DEBUG("Filling the field mask points");
    }
    else{
        G4String msg = "Control Point mask is already filled! This shouldn't happen!";
        LOGSVC_CRITICAL(msg.data());
        G4Exception("ControlPoint", "FillPlanFieldMask", FatalErrorInArgument , msg);
    }

    std::string shape = Service<ConfigSvc>()->GetValue<std::string>("RunSvc", "FieldShape");
    LOGSVC_DEBUG("Using the {} field shape and {} deg rotation",shape,GetDegreeRotation());

    if( shape.compare("Rectangular")==0 ||
        shape.compare("Elipsoidal")==0){
        FillPlanFieldMaskForRegularShapes(shape);
    }
    if(shape.compare("RTPlan")==0){
        FillPlanFieldMaskFromRTPlan();
    }
    if(m_plan_mask_points.empty()){
        G4String msg = "Field Mask not filled! Verify job configuration!";
        LOGSVC_CRITICAL(msg.data());
        G4Exception("ControlPoint", "FillPlanFieldMask", FatalErrorInArgument, msg);
    }
    LOGSVC_DEBUG("Filled with {} number of points",m_plan_mask_points.size());
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPoint::FillPlanFieldMaskForRegularShapes(const std::string& shape){
    double current_x,current_y, current_z;
    current_z = -30.25; // MLC Possition at 0 deg beam
    double x_range, y_range;

    auto rotate = [&](const G4ThreeVector& position) -> G4ThreeVector {
        return m_rotation ? *m_rotation * position : position;
    };
    x_range = Service<ConfigSvc>()->GetValue<double>("RunSvc", "FieldSizeA");
    y_range = Service<ConfigSvc>()->GetValue<double>("RunSvc", "FieldSizeB");
    if(x_range < 0 || y_range < 0){
        G4String msg = "Field size is not correct";
        LOGSVC_CRITICAL("Field size is not correct: A {}, B {}",x_range,y_range);
        G4Exception("ControlPoint", "FillPlanFieldMask", FatalErrorInArgument, msg);
    }
    double min_x = - x_range / 2.;
    double max_x = + x_range / 2.;
    double dx = FIELD_MASK_POINTS_DISTANCE;
    int n_x = (max_x - min_x ) / dx ;
    double min_y = - y_range / 2.;
    double max_y = + y_range / 2.;
    double dy = FIELD_MASK_POINTS_DISTANCE;
    int n_y = (max_y - min_y ) / dy;
    current_x = min_x + (0.5 * FIELD_MASK_POINTS_DISTANCE);
    current_y = min_y + (0.5 * FIELD_MASK_POINTS_DISTANCE);
    for(int i = 0; i<n_x; i++){
        current_y = min_y + (0.5 * FIELD_MASK_POINTS_DISTANCE);
        for(int j = 0; j<n_y; j++){
            if(shape.compare("Elipsoidal")==0){
                if ((pow(current_x,2)/ pow((x_range / 2.),2) + pow(current_y,2)/pow((y_range / 2.),2)) < 1 ){
                    m_plan_mask_points.push_back(rotate(G4ThreeVector(current_x,current_y,current_z)));
                } 
            } else { // Rectangular
                m_plan_mask_points.push_back(rotate(G4ThreeVector(current_x,current_y,current_z)));
            }
            current_y += dy;
        }
        current_x += dx ;
    }
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPoint::FillPlanFieldMaskFromRTPlan(){
    G4String msg = "Field Mask RTPlan type not implemented yet!";
    LOGSVC_CRITICAL(msg.data());
    G4Exception("ControlPoint", "FillPlanFieldMask", FatalErrorInArgument, msg);
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPoint::DumpVolumeMaskToFile(std::string scoring_vol_name, const std::map<std::size_t, VoxelHit>& volume_scoring) const {
    auto output_dir = Service<ConfigSvc>()->GetValue<std::string>("RunSvc", "OutputDir");
    const std::string file = output_dir+"/cp-"+std::to_string(GetId())+"_scoring_volume"+scoring_vol_name+"mask.csv";
    std::string header = "X [mm],Y [mm],Z [mm],mX [mm],mY [mm],mZ [mm],inFieldTag";
    std::ofstream c_outFile;
    c_outFile.open(file.c_str(), std::ios::out);
    c_outFile << header << std::endl;
    for(auto& vol : volume_scoring){
        auto pos = vol.second.GetCentre();
        auto trans_pos = TransformToMaskPosition(pos);
        auto inFieldTag = vol.second.GetMaskTag();
        // std::cout << "z: " << pos.getZ() << "  trans z: "<< trans_pos.getZ() << std::endl;
        c_outFile << pos.getX() << "," << pos.getY() << "," << pos.getZ();
        c_outFile << "," << trans_pos.getX() << "," << trans_pos.getY() << "," << trans_pos.getZ() << "," << inFieldTag << std::endl;

    }
    c_outFile.close();
}
////////////////////////////////////////////////////////////////////////////////
///
G4ThreeVector ControlPoint::TransformToMaskPosition(const G4ThreeVector& position) const {
    // Build the plane equation
    auto orign = *m_rotation * G4ThreeVector(0,0,-1000);
    auto normalVector = *m_rotation * G4ThreeVector(0,0,1);
    auto maskPoint = m_plan_mask_points.at(0);
    // 
    auto plane_normal_x = normalVector.getX();
    auto plane_normal_y = normalVector.getY();
    auto plane_normal_z = normalVector.getZ();
    // 

    auto point_on_mask_x = maskPoint.getX();
    auto point_on_mask_y = maskPoint.getY();
    auto point_on_mask_z = maskPoint.getZ();
    // 
    auto voxcel_to_origin_x = orign.getX() - position.getX();
    auto voxcel_to_origin_y = orign.getY() - position.getY();
    auto voxcel_to_origin_z = orign.getZ() - position.getZ();

    auto voxel_centre_x = position.getX();
    auto voxel_centre_y = position.getY();
    auto voxel_centre_z = position.getZ();

    G4double t = ((plane_normal_x*point_on_mask_x + plane_normal_y*point_on_mask_y + plane_normal_z*point_on_mask_z) -
                (plane_normal_x*voxel_centre_x + plane_normal_y*voxel_centre_y + plane_normal_z*voxel_centre_z)) / 
                (plane_normal_x*voxcel_to_origin_x + plane_normal_y*voxcel_to_origin_y + plane_normal_z*voxcel_to_origin_z);

    // Find the crosssection of the line from voxel centre to origin laying the plane:
    G4double cp_vox_x = voxel_centre_x + (voxcel_to_origin_x) * t;
    G4double cp_vox_y = voxel_centre_y + (voxcel_to_origin_y) * t;
    G4double cp_vox_z = voxel_centre_z + (voxcel_to_origin_z) * t;
    return G4ThreeVector(cp_vox_x,cp_vox_y,cp_vox_z);
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool ControlPoint::IsInField(const G4ThreeVector& position, G4bool transformedToMaskPosition) const {
    // std::cout << "IsInField in..." << std::endl;
    if(m_plan_mask_points.empty()) 
        return false; // TODO: add exception throw...
    G4ThreeVector pos = position;
    if(transformedToMaskPosition==false)
        pos = TransformToMaskPosition(position);
    auto dist_treshold = FIELD_MASK_POINTS_DISTANCE * mm; //FIELD_MASK_POINTS_DISTANCE*sqrt(2);
    // LOGSVC_INFO("In field distance trehshold {}",dist_treshold);
    for(const auto& mp : m_plan_mask_points){
        auto dist = sqrt(mp.diff2(pos));
        if(dist < dist_treshold)
            return true;
    }
    return false;
} 

////////////////////////////////////////////////////////////////////////////////
///
G4bool ControlPoint::IsInField(const G4ThreeVector& position) const {
    return IsInField(position, false);
} 

////////////////////////////////////////////////////////////////////////////////
///
G4double ControlPoint::GetInFieldMaskTag(const G4ThreeVector& position) const {
    G4double closest_dist{10.e9};
    auto maskLevelPosition = TransformToMaskPosition(position);
    if(IsInField(maskLevelPosition, true)){
        return 1;
    }
    else{
        for(const auto& mp : m_plan_mask_points){
            auto current_dist = sqrt(mp.diff2(maskLevelPosition));
            if(current_dist>0){
                if(closest_dist>current_dist)
                    closest_dist = current_dist;
            }
        }
        return 1. / (closest_dist/FIELD_MASK_POINTS_DISTANCE);
    }
    return 1.;
}

void ControlPoint::FillEventCollections(G4HCofThisEvent* evtHC){
    auto run_collections = RunAnalysis::GetRunCollection(); 
    for(const auto& run_collection: run_collections){
        // LOGSVC_DEBUG("RunAnalysis::EndOfEvent: RunColllection {}",run_collection.first);
        for(const auto& hc: run_collection.second){
            // Related SensitiveDetector collection ID (Geant4 architecture)
            auto collID = G4SDManager::GetSDMpointer()->GetCollectionID(hc);
            // collID==-1 the collection is not found
            // collID==-2 the collection name is ambiguous
            if(collID<0){
                LOGSVC_INFO("ControlPoint::FillEventCollections: HC: {} / G4SDManager Err: {}", hc, collID);
            }
            else {
                auto thisHitsCollPtr = evtHC->GetHC(collID);
                if(thisHitsCollPtr) // The particular collection is stored at the current event.
                    FillEventCollection(run_collection.first,dynamic_cast<VoxelHitsCollection*>(thisHitsCollPtr));
            }
        }
    }
}

void ControlPoint::FillEventCollection(const G4String& run_collection, VoxelHitsCollection* hitsColl){
    int nHits = hitsColl->entries();
    auto hc_name = hitsColl->GetName();
    if(nHits==0){
        return; // no hits in this event
    }
    auto& scoring_collection = GetRun()->GetScoringCollection(run_collection);
    for (int i=0;i<nHits;i++){ // a.k.a. voxel loop
        auto hit = dynamic_cast<VoxelHit*>(hitsColl->GetHit(i));
        for(const auto& scoring_type: m_scoring_types){
            auto& current_scoring_collection = scoring_collection[scoring_type];
            switch (scoring_type){
                case Scoring::Type::Cell:
                    //TODO FillCellEventCollection(current_scoring_collection,hit);
                    break;
                case Scoring::Type::Voxel:
                    //TODO FillVoxelEventCollection(current_scoring_collection,hit);
                    break;
                default:
                    break;
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
///
// G4ThreeVector ControlPoint::GetWeightedActivityGeoCentre(const std::map<std::size_t, VoxelHit>& data) const {
//   std::vector<const VoxelHit*> in_field_scoring_volume;
//   for(auto& scoring_volume : data){
//     auto inField = IsInField(scoring_volume.second.GetCentre());
//     if(inField)
//       in_field_scoring_volume.push_back(&scoring_volume.second);
//   }
//   G4ThreeVector sum{0,0,0};
//   G4double total_dose{0};
//   std::for_each(in_field_scoring_volume.begin(), in_field_scoring_volume.end(), [&](const VoxelHit* iv) {
//         sum += iv->GetCentre() * iv->GetDose();
//         total_dose += iv->GetDose();
//     });
//   return total_dose == 0 ? sum : sum / total_dose;
// }


