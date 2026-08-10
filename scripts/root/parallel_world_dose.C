// Usage: root -l -q 'scripts/root/parallel_world_dose.C("dose.csv","z",0.,"dose_qa.pdf")'
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TStyle.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
struct DoseRow { double x,y,z,dose; };
std::vector<DoseRow> readDose(const char* name) {
  std::ifstream in(name); if (!in) throw std::runtime_error(std::string("Cannot open ")+name);
  std::vector<DoseRow> rows; std::string line;
  while (std::getline(in,line)) { if(line.empty()||line[0]=='#'||line.rfind("Label,",0)==0) continue;
    std::stringstream ss(line); std::vector<std::string> f; std::string v; while(std::getline(ss,v,',')) f.push_back(v);
    if(f.size()>=13) rows.push_back({std::stod(f[7]),std::stod(f[8]),std::stod(f[9]),std::stod(f[10])}); }
  return rows;
}
std::vector<double> uniqueSorted(std::vector<double> v) { std::sort(v.begin(),v.end()); v.erase(std::unique(v.begin(),v.end()),v.end()); return v; }
}
void parallel_world_dose(const char* filename, const char* axis="z", double coordinate_mm=0., const char* output="parallel_world_dose_qa.pdf") {
  auto rows=readDose(filename); if(rows.empty()) throw std::runtime_error("Dose CSV has no rows");
  int n=axis[0]=='x'?0:axis[0]=='y'?1:2, h=n==0?1:0, v=n==2?1:2;
  auto at=[](const DoseRow&r,int i){return i==0?r.x:i==1?r.y:r.z;};
  double selected=at(rows[0],n); for(auto&r:rows) if(std::abs(at(r,n)-coordinate_mm)<std::abs(selected-coordinate_mm)) selected=at(r,n);
  std::vector<double> xs,ys; for(auto&r:rows) if(std::abs(at(r,n)-selected)<1e-9){xs.push_back(at(r,h));ys.push_back(at(r,v));}
  auto ux=uniqueSorted(xs),uy=uniqueSorted(ys); if(ux.empty()||uy.empty()) throw std::runtime_error("Selected slice empty");
  double dx=ux.size()>1?ux[1]-ux[0]:1,dy=uy.size()>1?uy[1]-uy[0]:1;
  auto slice=new TH2D("doseSlice",Form("Dose slice, %c = %.3g mm;%c [mm];%c [mm]",axis[0],selected,"xyz"[h],"xyz"[v]),128,ux.front()-dx/2,ux.back()+dx/2,128,uy.front()-dy/2,uy.back()+dy/2);
  double maxDose=std::max_element(rows.begin(),rows.end(),[](auto&a,auto&b){return a.dose<b.dose;})->dose;
  auto spectrum=new TH1D("doseDistribution","Scored voxel dose distribution;Dose [Gy];Voxels",80,0,maxDose>0?maxDose*1.001:1.);
  for(auto&r:rows){spectrum->Fill(r.dose);if(std::abs(at(r,n)-selected)<1e-9)slice->Fill(at(r,h),at(r,v),r.dose);}
  double c1=at(rows[0],h),c2=at(rows[0],v); for(auto&r:rows){if(std::abs(at(r,h))<std::abs(c1))c1=at(r,h);if(std::abs(at(r,v))<std::abs(c2))c2=at(r,v);}
  std::vector<std::pair<double,double>> points; for(auto&r:rows)if(std::abs(at(r,h)-c1)<1e-9&&std::abs(at(r,v)-c2)<1e-9)points.push_back({at(r,n),r.dose});
  std::sort(points.begin(),points.end()); std::vector<double> px,py;for(auto&p:points){px.push_back(p.first);py.push_back(p.second);}
  auto profile=new TGraph(px.size(),px.data(),py.data());profile->SetTitle(Form("Central profile;%c [mm];Dose [Gy]",axis[0]));profile->SetMarkerStyle(20);
  gStyle->SetOptStat(1110);auto canvas=new TCanvas("parallelWorldDoseQA","Parallel-world dose QA",1500,500);canvas->Divide(3,1);
  canvas->cd(1);slice->Draw("COLZ");canvas->cd(2);profile->Draw("APL");canvas->cd(3);gPad->SetLogy();spectrum->Draw();canvas->SaveAs(output);
}
