#ifndef ZOOMCLASS_H
#define ZOOMCLASS_H

#include <TROOT.h>
#include <TCanvas.h>

using namespace std;

class ZoomClass {

public:
  ZoomClass();
  ~ZoomClass();
  static void ZoomChannel(int tbs, int adc);
  
};

inline ZoomClass::ZoomClass() {}
inline ZoomClass::~ZoomClass() {}

inline void ZoomClass::ZoomChannel(int tbs, int adc)
//inline void ZoomClass::ZoomChannel(Int_t event, Int_t x, Int_t y, TObject *selected)
{
  //  int event = gPad->GetEvent();
  //  if (event != 11) return;

  TCanvas* current = gPad->GetCanvas();
  std::string cname = current->GetName();

  TObject *select = gPad->GetSelected();
  if (!select) return;
  gPad->GetCanvas()->FeedbackMode(kTRUE);

  if (!select->InheritsFrom(TPad::Class())) return;
  TPad *pad = (TPad*)select;    
  std::string pname = pad->GetName();

  int px = pad->GetEventX();
  int py = pad->GetEventY();
  double xd = pad->AbsPixeltoX(px);
  double yd = pad->AbsPixeltoY(py);
  float x = pad->PadtoX(xd);
  float y = pad->PadtoY(yd);
  TPad *ppad = current->Pick(px, py, 0);

  /*
  if (cname != pname){
    std::cout << "Pad name: " << pname << std::endl;
    std::cout << px << " " << py << " " << ppad->GetName() << std::endl;
    std::cout << "AbsPixeltoX " << xd << " " << yd << std::endl;
    std::cout << "PadtoX " << x << " " << y << std::endl;
  }
  */
  
  TH1D* htmp = new TH1D;
  
  TObject *obj;
  TIter next(pad->GetListOfPrimitives());
  if (cname != pname){
    while ((obj = next())) {
      if (obj->InheritsFrom(TH1::Class())) {
	TH1* h = (TH1*)obj;
	htmp = (TH1D*)h->Clone();
      }
    }

    TCanvas *c = (TCanvas*)gROOT->GetListOfCanvases()->FindObject("c");
    if(c) delete c->GetPrimitive("");
    else c = new TCanvas("c","",700,500);
    c->SetGrid();
    c->cd();
    htmp->SetBins(tbs, 0, tbs-1);
    htmp->SetStats(0);
    htmp->SetLineColor(2);
    htmp->SetLineWidth(3);
    htmp->SetMinimum(0);
    htmp->SetMaximum(adc);
    htmp->Draw("histo");
    c->Update();
  }

}

#endif
