#include "GenericPhantom.hh"

#include "GeometryBuilder.hh"
#include "Services.hh"

////////////////////////////////////////////////////////////////////////////////
///
GenericPhantom::GenericPhantom()
  : IPhysicalVolume("GenericPhantom")
{}

////////////////////////////////////////////////////////////////////////////////
///
GenericPhantom* GenericPhantom::GetInstance() {
  static GenericPhantom instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void GenericPhantom::Construct(G4VPhysicalVolume* parentPV) {
  auto geometryBuilder = GeometryBuilder::GetInstance();
  geometryBuilder->Build(parentPV);
}

////////////////////////////////////////////////////////////////////////////////
///
void GenericPhantom::WriteInfo() {
  G4cout << "GenericPhantom: DB/3MF geometry imported without patient construction." << G4endl;
}
