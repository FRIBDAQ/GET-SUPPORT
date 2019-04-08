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

inline void ZoomClass::ZoomChannel(int tbs, int adc) {

  TObject *select = gPad->GetSelected();
  if(!select) return;
  if (!select->InheritsFrom(TH1::Class())) {
    gPad->SetUniqueID(0);
    return;
  }
  TH1 *h = (TH1*)select;
  gPad->GetCanvas()->FeedbackMode(kTRUE);

  //create or set the new canvas c2
  TVirtualPad *padsav = gPad;
  TCanvas *c2 = (TCanvas*)gROOT->GetListOfCanvases()->FindObject("c2");
  if(c2) delete c2->GetPrimitive("");
  else   c2 = new TCanvas("c2","",710,10,700,500);
  c2->SetGrid();
  c2->cd();

  TH1D* htmp = (TH1D*)h->Clone();
  htmp->SetBins(tbs, 0, tbs-1);
  htmp->SetStats(0);
  htmp->SetLineColor(2);
  htmp->SetLineWidth(3);
  htmp->SetMinimum(0);
  htmp->SetMaximum(adc);
  htmp->Draw("histo");
  c2->Update();
  padsav->cd();

}

#endif
