#include "GeometryParser.hh"
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <G4SystemOfUnits.hh>
#include <stdexcept>

namespace py = pybind11;

GeometryParser::GeometryParser()
 : m_parser_(py::module::import("xmlx_geometry_parser"))
{}

GeometryParser::~GeometryParser() {

}

void GeometryParser::load(const std::string& filename,
                          const std::string& csv_filename,
                          const std::string& sheet)
{

  py::object sheet_arg = sheet.empty()
    ? py::object(py::none())
    : py::object(py::str(sheet));
  // wywołanie get_geometries
  py::object py_list = m_parser_.attr("get_geometries")(
    filename,
    csv_filename,
    sheet_arg
  );
  auto vec = py_list.cast<std::vector<py::dict>>();

  geoms_.clear();
  geoms_.reserve(vec.size());
  for (auto &d : vec) {
    GeometryData gd;
    gd.component = std::string(py::str(d["component"]));
    gd.body      = std::string(py::str(d["body"]));
    // COM
    auto com_py = d["com"].cast<std::vector<double>>();
    gd.com = { (-90.0)*mm, (-90.0)*mm, (-195.0) *mm};
    // SC id
    gd.sc_id = std::string(py::str(d["sc_id"]));
    // nodes
    gd.nodes = d["nodes"].cast<std::vector<std::array<int,3>>>();
    // vertices
    auto verts_py = d["vertices"].cast<std::vector<std::array<double,3>>>();
    gd.vertices.reserve(verts_py.size());
    for (auto &v : verts_py)
      gd.vertices.emplace_back(v[0]*mm, v[1]*mm, v[2]*mm);
    // normals
    auto norms_py = d["normals"].cast<std::vector<std::array<double,3>>>();
    gd.normals.reserve(norms_py.size());
    for (auto &n : norms_py)
      gd.normals.emplace_back(n[0], n[1], n[2]);

    geoms_.push_back(std::move(gd));
  }
}
