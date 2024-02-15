#include "ControlPoint.hh"
#include "Services.hh"
#include "NTupleEventAnalisys.hh"
#include "IO.hh"
#include "TFile.h"
#include "TTree.h"
#include "TChain.h"

#ifdef G4MULTITHREADED
    #include "G4Threading.hh"
    #include "G4MTRunManager.hh"
#endif

double ControlPoint::FIELD_MASK_POINTS_DISTANCE = 0.5;
std::string ControlPoint::m_sim_dir = "sim";

////////////////////////////////////////////////////////////////////////////////
///
ControlPointConfig::ControlPointConfig(int id, int nevts, double rot)
: Id(id), NEvts(nevts),RotationInDeg(rot){}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPointRun::Merge(const G4Run* aRun){
    G4cout << "### Run " << aRun->GetRunID() << " merging..." << G4endl;
}


////////////////////////////////////////////////////////////////////////////////
///
ControlPoint::ControlPoint(const ControlPointConfig& config): m_config(config){
    G4cout << " DEBUG: ControlPoint:Ctr: rotation: " << config.RotationInDeg << G4endl;
    SetRotation(config.RotationInDeg);
    FillPlanFieldMask();
}

////////////////////////////////////////////////////////////////////////////////
///
ControlPoint::ControlPoint(const ControlPoint& cp):m_config(cp.m_config){
    m_rotation = new G4RotationMatrix(*cp.m_rotation);
    m_plan_mask_points = cp.m_plan_mask_points;
    m_sim_mask_points = cp.m_sim_mask_points;
    m_mlc_a_positioning = cp.m_mlc_a_positioning;
    m_mlc_b_positioning = cp.m_mlc_b_positioning;
}

////////////////////////////////////////////////////////////////////////////////
///
ControlPoint::ControlPoint(ControlPoint&& cp):m_config(cp.m_config){
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
};

////////////////////////////////////////////////////////////////////////////////
///
bool ControlPoint::InitializeRunScoringCollection(const G4String& scoring_name) {
    LOGSVC_INFO("RUN SCORING INITIALIZATION...");
    if(!G4Threading::IsWorkerThread ()){
        for(const auto& scoring_type: m_scoring_types){
            LOGSVC_INFO("Adding new map for scoring type: {}",Scoring::to_string(scoring_type));
            if(!m_mt_hashed_scoring_map.Has(scoring_name))
                m_mt_hashed_scoring_map.Insert(scoring_name,ScoringMap());
            auto& scoring_collection = m_mt_hashed_scoring_map.Get(scoring_name);
            scoring_collection[scoring_type] = Service<GeoSvc>()->Patient()->GetScoringHashedMap(scoring_type);
        }
    } else {
        LOGSVC_CRITICAL("Control Point Run Initialization should be done only from master thread!");
        return false;
    }
    m_run_initialized = true;
    return m_run_initialized;
}

////////////////////////////////////////////////////////////////////////////////
///
G4Run* ControlPoint::GenerateRun(bool scoring){
    /// TODO: Lock and store every new generated run
    /// It seems Kernel desn't cleanup memory!!!
    //if(m_run_initialized & scoring){
        //return new ControlPointRun(&m_mt_hashed_scoring_map.Get());
    //}
    return new ControlPointRun();
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
void ControlPoint::WriteIntegratedDoseToFile(bool tfile, bool csv) const {
    LOGSVC_DEBUG("Writing integrated dose to file");

// GetPatientScoring(Scoring::Type stype){
//     std::string name = Scoring::to_string(stype);
//   if (m_hashed_scoring.find(stype) != m_hashed_scoring.end()) {
//     LOGSVC_DEBUG("Override existing scoring: {}",name)
//   }
//   m_hashed_scoring[stype] = Service<GeoSvc>()->Patient()->GetScoringHashedMap(name,true);
//   if(m_hashed_scoring[stype].empty()){
//     LOGSVC_WARN("Empty scoring map instantiated for {}!",name);
//   }
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
void ControlPoint::FillScoringData(){
    if(m_is_scoring_data_filled) 
        return;

    LOGSVC_INFO("Filling scoring data (CP-{})",GetId());

    auto getTreeName = [&](){
        // TODO: Once the m_scoring_types includes Voxel the [...]Voxelised[...] tree should be picked up
        auto scoring_collection = NTupleEventAnalisys::TreeCollection();
        for(const auto& scoring : scoring_collection){
            LOGSVC_INFO("Scoring type: {}",scoring.m_name);
        }
        return std::string("Dose3DVoxelisedTTree"); // FIXED
    };

    auto treeName = getTreeName();

    bool voxelised = false;
    if(treeName.find("Voxel") != std::string::npos)
        voxelised = true;
    else {
        // Once there is no corresponding TTree we shouldn't consider anymore
        // this scoring type:
        m_scoring_types.erase(Scoring::Type::Voxel);
    }

    auto isHashedScoringExists = [&](Scoring::Type type){
        if (m_hashed_scoring_map.find(type) != m_hashed_scoring_map.end())
            return true;
        return false;
    };

    for(const auto& scoring_type: m_scoring_types){
        if(!isHashedScoringExists(scoring_type)){
            LOGSVC_INFO("Adding new map for scoring type: {}",Scoring::to_string(scoring_type));
            m_hashed_scoring_map[scoring_type] = Service<GeoSvc>()->Patient()->GetScoringHashedMap(scoring_type);
        }
    }

    TTree* tree = nullptr;
    // Now get data from the corresponding tfile/tree
    auto singleRootTfile =  GetSimOutputTFileName();
    std::cout << "singleRootTfile " << singleRootTfile << std::endl;

    // auto chain = 
    if (svc::checkIfFileExist(singleRootTfile)){
        auto f = std::make_unique<TFile>(TString(singleRootTfile));
        tree = static_cast<TTree*>(f->Get(TString(treeName)));
        std::cout << "got  " << singleRootTfile << std::endl;

    }
    else{
        auto subjob_dir = GetOutputDir()+"/subjobs";
        std::cout << "getting  " << subjob_dir << std::endl;

        auto filelist = svc::getFilesInDir(subjob_dir);
        tree = new TChain(treeName.c_str());
        for (const auto &file : filelist){
            auto f_s = std::make_unique<TFile>(TString(file.c_str()));
            auto tree_s = static_cast<TTree*>(f_s->Get(TString(treeName)));

            std::cout << "File  "  << file << std::endl;
            std::cout << "Tree entries:  "  << tree_s->GetEntriesFast() << std::endl;

            dynamic_cast<TChain*>(tree)->AddFile(file.c_str());
        }
        // tree = static_cast<TTree*>(f->GetTree());
        std::cout << "Chain n trees " << dynamic_cast<TChain*>(tree)->GetNtrees() << std::endl;

    }

    G4int cIdx; tree->SetBranchAddress("CellIdX",&cIdx);
    G4int cIdy; tree->SetBranchAddress("CellIdY",&cIdy);
    G4int cIdz; tree->SetBranchAddress("CellIdZ",&cIdz);
    G4double c_dose; tree->SetBranchAddress("CellDose",&c_dose);

    G4int vIdx;
    G4int vIdy;
    G4int vIdz;
    G4double v_dose; 
    if(voxelised){
        tree->SetBranchAddress("VoxelIdX",&vIdx);
        tree->SetBranchAddress("VoxelIdY",&vIdy);
        tree->SetBranchAddress("VoxelIdZ",&vIdz);
        tree->SetBranchAddress("VoxelDose",&v_dose);
    }

    auto nentries = tree->GetEntries();
    LOGSVC_INFO("Reading {}",GetSimOutputTFileName());
    LOGSVC_INFO("NEntries {} ({})",nentries,getTreeName());
    for (Long64_t i=0;i<nentries;i++) {
        tree->GetEntry(i);
        auto hash_str = std::to_string(cIdx);
        hash_str+= std::to_string(cIdy);
        hash_str+= std::to_string(cIdz);
        auto hash_key_c = std::hash<std::string>{}(hash_str);
        auto& cell_hit = m_hashed_scoring_map[Scoring::Type::Cell].at(hash_key_c);
        cell_hit.SetDose(cell_hit.GetDose()+c_dose);
        if(voxelised){
            hash_str+= std::to_string(vIdx);
            hash_str+= std::to_string(vIdy);
            hash_str+= std::to_string(vIdz);
            auto hash_key = std::hash<std::string>{}(hash_str);
            auto& voxel_hit = m_hashed_scoring_map[Scoring::Type::Voxel].at(hash_key);
            voxel_hit.SetDose(voxel_hit.GetDose()+v_dose);
        }
    }
    m_is_scoring_data_filled = true;
    FillScoringDataTagging();
    LOGSVC_INFO("Filling scoring data (CP-{}) - done!",GetId());
    delete tree;
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPoint::FillScoringDataTagging(ScoringMap* scoring_data){
    LOGSVC_INFO("Filling scoring tagging....");

    auto& hashed_scoring_map = scoring_data ? *scoring_data : m_hashed_scoring_map;

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
        auto mask_tag = GetInFieldMaskTag(hit.GetCentre());
        auto geo_tag = 1./sqrt(hit.GetCentre().diff2(geoCentre));
        auto wgeo_tag = 1./sqrt(hit.GetCentre().diff2(wgeoCentre));
        hit.FillTagging(mask_tag, geo_tag, wgeo_tag);
    };

    for(auto& scoring_type: m_scoring_types){
        LOGSVC_INFO("Scoring type {}",Scoring::to_string(scoring_type));
        auto& data = hashed_scoring_map[scoring_type];
        in_field_scoring_volume.clear();
        for(auto& hit : data){
            if(IsInField(hit.second.GetCentre()))
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

////////////////////////////////////////////////////////////////////////////////
///
void ControlPoint::WriteFieldMaskToTFile() const {
    auto fname = GetOutputFileName()+"_field_mask.root";
    auto dir_name = "RT_Plan/CP_"+std::to_string(GetId());
    auto file = IO::CreateOutputTFile(fname,dir_name);
    auto cp_dir = file->GetDirectory(dir_name.c_str());
    for(const auto& type : m_data_types){
        auto field_mask = GetFieldMask(type);
        if(field_mask.size()>0){
            auto linearized_mask_vec = svc::linearizeG4ThreeVector(field_mask);
            std::string name = "FieldMask_"+type;
            cp_dir->WriteObject(&linearized_mask_vec,name.c_str());
            file->Write();
        }
    }
    file->Close();
    LOGSVC_INFO("Writing Field Mask to file: {} - done!",file->GetName()); // LOGSVC_DEBUG
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPoint::WriteFieldMaskToCsv() const {
    for(const auto& type : m_data_types){
        LOGSVC_INFO("Writing field mask (type={}) to CSV...",type);
        auto field_mask = GetFieldMask(type);
        if(field_mask.size()>0){
            auto file = GetOutputFileName()+"_field_mask_"+svc::tolower(type)+".csv";
            std::string header = "X [mm],Y [mm],Z [mm]";
            std::ofstream c_outFile;
            c_outFile.open(file.c_str(), std::ios::out);
            c_outFile << header << std::endl;
            for(auto& mp : field_mask)
                c_outFile << mp.getX() << "," << mp.getY() << "," << mp.getZ() << std::endl;
            c_outFile.close();
            LOGSVC_INFO("Writing Field Mask to file {} - done!",file);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPoint::WriteVolumeDoseAndTaggingToTFile(){
    if(!m_is_scoring_data_filled) 
        FillScoringData();
    auto fname = GetOutputFileName()+"_dose_and_volume_tagging.root";
    auto dir_name = "RT_Plan/CP_"+std::to_string(GetId());
    auto file = IO::CreateOutputTFile(fname,dir_name);
    auto cp_dir = file->GetDirectory(dir_name.c_str());
    for(const auto& scoring_type: m_scoring_types) {
        auto& data = m_hashed_scoring_map[scoring_type];
        auto type = Scoring::to_string(scoring_type);
        std::vector<double> linearized_mask_vec;
        std::vector<double> infield_tag_vec;
        std::vector<double> geo_tag_vec;
        std::vector<double> geo_tag_weighted_vec;
        std::vector<double> dose_vec;
        for(auto& scoring : data){
            auto pos = scoring.second.GetCentre();
            for(const auto& i :  svc::linearizeG4ThreeVector(pos))
                linearized_mask_vec.emplace_back(i);
            infield_tag_vec.emplace_back(scoring.second.GetMaskTag());
            geo_tag_vec.emplace_back(scoring.second.GetGeoTag());
            geo_tag_weighted_vec.emplace_back(scoring.second.GetWeigthedGeoTag());
            dose_vec.emplace_back(scoring.second.GetDose());
        }
        std::string name_pos = "VolumeFieldMaskTagPosition_"+type;
        std::string name_ftag = "VolumeFieldMaskTagValue_"+type;
        std::string name_gtag = "VolumeGeoTagValue_"+type;
        std::string name_wgtag = "VolumeWeightedGeoTagValue_"+type;
        std::string name_dose = "Dose_"+type;
        cp_dir->WriteObject(&linearized_mask_vec,name_pos.c_str());
        cp_dir->WriteObject(&infield_tag_vec,name_ftag.c_str());
        cp_dir->WriteObject(&geo_tag_vec,name_gtag.c_str());
        cp_dir->WriteObject(&geo_tag_weighted_vec,name_wgtag.c_str());
        cp_dir->WriteObject(&dose_vec,name_dose.c_str());
        file->Write();
    }
    file->Close();
    LOGSVC_INFO("Writing Scoring Volume Field Mask to file {} - done!",file->GetName());
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPoint::WriteVolumeDoseAndTaggingToCsv(){
    if(!m_is_scoring_data_filled) 
        FillScoringData();
    
    auto writeVolumeHitDataRaw = [](std::ofstream& file, const VoxelHit& hit, bool voxelised){
        auto cxId = hit.GetGlobalID(0);
        auto cyId = hit.GetGlobalID(1);
        auto czId = hit.GetGlobalID(2);
        auto vxId = hit.GetID(0);
        auto vyId = hit.GetID(1);
        auto vzId = hit.GetID(2);
        auto volume_centre = hit.GetCentre();
        auto dose = hit.GetDose();
        auto geoTag = hit.GetGeoTag();
        auto wgeoTag = hit.GetWeigthedGeoTag();
        auto inField = hit.GetMaskTag();
        file <<cxId<<","<<cyId<<","<<czId;
        if(voxelised)
            file <<","<<vxId<<","<<vyId<<","<<vzId;
        file <<","<<volume_centre.getX()<<","<<volume_centre.getY()<<","<<volume_centre.getZ();
        file <<","<<dose<<","<< inField <<","<< geoTag<<","<< wgeoTag;
        file <<","<< dose / ( geoTag * inField );
        file <<","<< dose / ( wgeoTag * inField ) << std::endl;
    };

    for(const auto& scoring_type: m_scoring_types) {
        auto& data = m_hashed_scoring_map[scoring_type];
        auto type = Scoring::to_string(scoring_type);
        auto file = GetOutputFileName()+"_dose_and_volume_tagging_"+svc::tolower(type)+".csv";
        std::string header = "Cell IdX,Cell IdY,Cell IdZ,X [mm],Y [mm],Z [mm],Dose,MaskTag,GeoTag,wGeoTag,GeoMaskTagDose,wGeoMaskTagDose";
        if(scoring_type==Scoring::Type::Voxel)
            header = "Cell IdX,Cell IdY,Cell IdZ,Voxel IdX,Voxel IdY,Voxel IdZ,X [mm],Y [mm],Z [mm],Dose,MaskTag,GeoTag,wGeoTag,GeoMaskTagDose,wGeoMaskTagDose";

        std::ofstream c_outFile;
        c_outFile.open(file.c_str(), std::ios::out);
        c_outFile << header << std::endl;
        for(auto& scoring : data){
            writeVolumeHitDataRaw(c_outFile, scoring.second, scoring_type==Scoring::Type::Voxel);
            // auto pos = scoring.second.GetCentre();
            // auto trans_pos = TransformToMaskPosition(pos);
            // auto inFieldTag = scoring.second.GetMaskTag();
            // c_outFile   << pos.getX() << ","
            //             << pos.getY() << "," 
            //             << pos.getZ() << "," 
            //             << trans_pos.getX() << ","
            //             << trans_pos.getY() << "," 
            //             << trans_pos.getZ() << "," 
            //             << inFieldTag << std::endl;
        }
        c_outFile.close();
        LOGSVC_INFO("Writing scoring volume field mask to file {} - done!",file);
    }
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPoint::ClearCachedData() {
    m_hashed_scoring_map.clear();
    m_is_scoring_data_filled = false;
}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPoint::IntegrateAndWriteTotalDoseToTFile(){

}

////////////////////////////////////////////////////////////////////////////////
///
void ControlPoint::IntegrateAndWriteTotalDoseToCsv(){
    // Integration is based on individual control points integration dumped to csv

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

////////////////////////////////////////////////////////////////////////////////
///
void ControlPoint::WriteAndClearMTCache(){
    //FillScoringDataTagging(&m_mt_hashed_scoring_map.Get());
    // Clear cache:
    // m_mt_hashed_scoring_map
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


