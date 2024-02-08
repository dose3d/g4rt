#include "gtest/gtest.h"
#include "D3DDetector.hh"
#include "D3DMLayer.hh"
#include "D3DCell.hh"
#include "G4Box.hh"
#include "Services.hh"
#include "G4NistManager.hh"
#include "LogSvc.hh"
#include "PatientTest.hh"

// [WIP] Test detector construction that includes only the cell's positioning
TEST_F(PatientTest, Dose3DBasicAssemby4x4x4){
    auto toml_file = m_projectPath + "/Dose3D/test/d3d_basic_detector_4x4x4_test.toml";
    SvcSetup(toml_file);
    auto world = WorldSetup();
    auto patient = new D3DDetector(); // TODO: make this std::unique
    patient->SetTomlConfigFile(toml_file);
    patient->Construct(world); 
    // TODO: Verify
    // 1. number of layers
    // 2. number of cells in each layer
    // 3. positioning of the constructed cells vs assumed positioning
    EXPECT_TRUE(true);
}

// TODO
TEST_F(PatientTest, Dose3DCsvAssemby4x4x4){
    
    SvcSetup(m_projectPath + "/Dose3D/test/d3d_basic_detector_4x4x4_test.toml");
    auto world = WorldSetup();
    auto patient = new D3DDetector(); // TODO: make this std::unique
    patient->Construct(world); 
    EXPECT_TRUE(true);
}

// TODO
TEST_F(PatientTest, Dose3DStlCsvAssembly2x2x2){
    SvcSetup(m_projectPath + "/Dose3D/test/d3d_basic_detector_4x4x4_test.toml");
    auto world = WorldSetup();
    auto patient = new D3DDetector(); // TODO: make this std::unique
    patient->Construct(world); 
    EXPECT_TRUE(true);
}

