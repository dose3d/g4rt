#ifndef GEOMETRY_PARSER_HH
#define GEOMETRY_PARSER_HH

#include <string>
#include <vector>
#include <array>

#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <G4ThreeVector.hh>

namespace py = pybind11;


// Holds parsed geometry data for one component
struct GeometryData {
    std::string                     component;      // Component name
    std::string                     body;           // Body identifier
    G4ThreeVector                   com;            // Center of mass
    std::string                     sc_id;          // Scintilator identifier
    std::vector<std::array<int,3>>  nodes;          // Node IDs
    std::vector<G4ThreeVector>      vertices;       // Vertex positions
    std::vector<G4ThreeVector>      normals;        // Vertex normals
};

// Parses geometry definitions from Excel using Python
class GeometryParser {
public:
    GeometryParser();
    ~GeometryParser();

    // Load and parse sheets from Excel and CSV
    void load(const std::string& filename,
              const std::string& csv_filename,
              const std::string& sheet);

    // Access parsed geometry data
    const std::vector<GeometryData>& data() const { return geoms_; }

private:
    py::object m_parser;                            // Python parser object
    std::vector<GeometryData> geoms_;               // Parsed geometry entries
};

#endif // GEOMETRY_PARSER_HH
