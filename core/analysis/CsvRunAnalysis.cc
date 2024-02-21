#include "CsvRunAnalysis.hh"
#include "Services.hh"
#include "ControlPoint.hh"

////////////////////////////////////////////////////////////////////////////////
///
CsvRunAnalysis *CsvRunAnalysis::GetInstance() {
    static CsvRunAnalysis instance = CsvRunAnalysis();
    return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void CsvRunAnalysis::WriteDoseToCsv(const G4Run* runPtr){
    LOGSVC_INFO("CsvRunAnalysis::WriteDoseToCsv...");
    
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

    auto cp = Service<RunSvc>()->CurrentControlPoint();
    const auto& scoring_maps = cp->GetRun()->GetScoringCollections();
    for(auto& scoring_map: scoring_maps){
        LOGSVC_INFO("CsvRunAnalysis::WriteDoseToCsv for {} run collection:",scoring_map.first);
        for(auto& scoring: scoring_map.second){
            auto scoring_type = scoring.first;
            auto& data = scoring.second;
            auto type_str = svc::tolower(Scoring::to_string(scoring_type));
            auto coll_str = svc::tolower(scoring_map.first);
            auto file = cp->GetOutputFileName()+"_"+coll_str+"_"+type_str+".csv";
            std::string header = "Cell IdX,Cell IdY,Cell IdZ,X [mm],Y [mm],Z [mm],Dose,MaskTag,GeoTag,wGeoTag,GeoMaskTagDose,wGeoMaskTagDose";
            if(scoring_type==Scoring::Type::Voxel)
                header = "Cell IdX,Cell IdY,Cell IdZ,Voxel IdX,Voxel IdY,Voxel IdZ,X [mm],Y [mm],Z [mm],Dose,MaskTag,GeoTag,wGeoTag,GeoMaskTagDose,wGeoMaskTagDose";

            std::ofstream c_outFile;
            c_outFile.open(file.c_str(), std::ios::out);
            c_outFile << header << std::endl;
            for(auto& scoring : data){
                writeVolumeHitDataRaw(c_outFile, scoring.second, scoring_type==Scoring::Type::Voxel);
            }
            c_outFile.close();
            LOGSVC_INFO("Output file closed: {}",file);
        }
        LOGSVC_INFO("CsvRunAnalysis::WriteDoseToCsv for {} run collection - done!",scoring_map.first);
    }
}


