#ifndef MODULAR_PHANTOM_HH
#define MODULAR_PHANTOM_HH

#include "G4PVPlacement.hh"
#include "IPhysicalVolume.hh"

///\class ModularPhantom
class ModularPhantom : public IPhysicalVolume {
  public:

  /// 
  static ModularPhantom *GetInstance();
  
  /// 
  void Construct(G4VPhysicalVolume *parentPV) override;
  
  /// 
  void Destroy() override;
  
  /// 
  G4bool Update() override;
  
  /// 
  void Reset() override { G4cout << "Implement me." << G4endl; }
  
  ///  
  void WriteInfo() override;
  
  
  private:
  
  /// 
  ModularPhantom();
  
  /// 
  ~ModularPhantom();


  ///
  std::string m_phantomMedium = "RW3";

  /// 
  G4double m_centrePositionX = 0.0;
  G4double m_centrePositionY = 0.0;
  G4double m_centrePositionZ = 0.0;



};

#endif  // MODULAR_PHANTOM_HH
