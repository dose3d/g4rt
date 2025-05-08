#ifndef GEOMETRY_PARSER_HH
#define GEOMETRY_PARSER_HH
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <G4ThreeVector.hh>
#include <array>
#include <vector>
#include <string>

namespace py = pybind11;

struct GeometryData {
  std::string                     component;
  std::string                     body;
  G4ThreeVector                   com;
  std::string                     sc_id;
  std::vector<std::array<int,3>>  nodes;
  std::vector<G4ThreeVector>      vertices;
  std::vector<G4ThreeVector>      normals;
};

class GeometryParser {
public:
  GeometryParser();
  ~GeometryParser();

  /// Ładuje wszystkie geometrie z Excela
  void load(const std::string& filename,
    const std::string& csv_filename,
    const std::string& sheet);

  /// Zwraca listę obiektów
  const std::vector<GeometryData>& data() const { return geoms_; }

private:
  py::object     m_parser_;
  std::vector<GeometryData> geoms_;
};

#endif  // GEOMETRY_PARSER_HH
