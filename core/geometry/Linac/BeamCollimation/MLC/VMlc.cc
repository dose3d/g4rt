#include "VMlc.hh"
#include "Services.hh"
#include "ControlPoint.hh"
#include "G4PrimaryVertex.hh"

G4ThreeVector VMlc::m_isocentre = G4ThreeVector();


VMlc::VMlc(const std::string& name) : TomlConfigurable(name) {
    m_isocentre = Service<ConfigSvc>()->GetValue<G4ThreeVector>("WorldConstruction", "Isocentre");
}

////////////////////////////////////////////////////////////////////////////////
///
G4ThreeVector VMlc::GetPositionInMaskPlane(const G4ThreeVector& position){
    // Build the plane equation
    auto cp = Service<RunSvc>()->CurrentControlPoint();
    auto rotation = cp->GetRotation();
    auto sid = Service<ConfigSvc>()->GetValue<G4double>("LinacGeometry", "SID") * mm;
    auto orign = *rotation * G4ThreeVector(0,0,sid);
    auto normalVector = *rotation * G4ThreeVector(0,0,1);
    auto maskPoint = cp->GetPlanMaskPoints().at(0);
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
    return svc::round_with_prec(G4ThreeVector(cp_vox_x,cp_vox_y,cp_vox_z),8);
}

G4ThreeVector VMlc::GetPositionInMaskPlane(const G4PrimaryVertex* vrtx){
    auto position = vrtx->GetPosition();
    auto direction = vrtx->GetPrimary()->GetMomentum().unit();
    G4double z = m_isocentre.z();
    G4double deltaZ = z - position.getZ();
    G4double zRatio = deltaZ / direction.getZ(); 
    G4double x = position.getX() + zRatio * direction.getX(); // x + deltaX;
    G4double y = position.getY() + zRatio * direction.getY(); // y + deltaY;
    return G4ThreeVector(x, y, z);
}

