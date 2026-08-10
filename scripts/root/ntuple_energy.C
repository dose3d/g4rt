// Usage: root -l -q 'scripts/root/ntuple_energy.C("job.root","ParentWorldDoseTTree","energy.pdf")'
#include <TCanvas.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TH1.h>
#include <TTree.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
void ntuple_energy(const char* filename,const char* treeName="ParentWorldDoseTTree",const char* output="ntuple_energy.pdf") {
  TFile file(filename,"READ");auto tree=dynamic_cast<TTree*>(file.Get(treeName));if(!tree)throw std::runtime_error(std::string("Missing TTree: ")+treeName);
  struct Obs{const char* branch;const char* title;};std::vector<Obs> candidates={{"VoxelEDeposit","Voxel energy deposit;Edep [MeV];Entries"},{"VoxelMeanEDeposit","Mean voxel energy deposit;Mean Edep [MeV];Entries"},{"VoxelTrkE","Track energy at scored step;Energy [MeV];Entries"},{"G4EvtPrimaryE","Primary energy;Energy [MeV];Entries"}},available;
  for(auto&o:candidates)if(tree->GetBranch(o.branch))available.push_back(o);else std::cout<<"[skip] Branch not stored: "<<o.branch<<'\n';
  if(available.empty())throw std::runtime_error("No supported energy branches");auto canvas=new TCanvas("energyQA","G4RT energy QA",700*available.size(),600);canvas->Divide(available.size(),1);
  for(size_t i=0;i<available.size();++i){canvas->cd(i+1);gPad->SetLogy();std::string h="energy_"+std::to_string(i);tree->Draw((std::string(available[i].branch)+">>"+h+"(100)").c_str(),"","goff");auto hist=dynamic_cast<TH1*>(gDirectory->Get(h.c_str()));if(hist){hist->SetTitle(available[i].title);hist->Draw();}}
  canvas->SaveAs(output);
}
