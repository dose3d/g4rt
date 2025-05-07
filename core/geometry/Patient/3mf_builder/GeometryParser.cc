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
                       const std::string& sheet)
{
  // wywołanie get_geometries
  py::object py_list = m_parser_.attr("get_geometries")(
    filename,
    sheet.empty() ? py::none() : py::str(sheet)
  );
  auto vec = py_list.cast<std::vector<py::dict>>();

  geoms_.clear();
  geoms_.reserve(vec.size());
  for (auto &d : vec) {
    GeometryData gd;
    gd.component = d["component"].cast<std::string>();
    gd.body      = d["body"].cast<std::string>();
    // COM
    auto com_py = d["com"].cast<std::vector<double>>();
    gd.com = { com_py[0]*mm, com_py[1]*mm, com_py[2]*mm };
    // SC id
    gd.sc_id = d["sc_id"].cast<std::string>();
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
