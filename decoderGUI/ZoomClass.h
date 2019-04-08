#ifndef ZOOMCLASS_H
#define ZOOMCLASS_H

#include <TROOT.h>
#include <TCanvas.h>

using namespace std;

Int_t    fCOBO = 0;
Int_t    fASAD = 0;
Int_t    fAGET = 0;
Int_t    fCHN = 0;
Int_t    fTbs = 512;
Int_t    fADC = 4095;

class ZoomClass {

public:
  ZoomClass();
  ~ZoomClass();
  static void ZoomChannel(int tbs, int adc);

  //private:
  //  TCanvas* m_canv;
  //  int m_CLICK;

};

inline ZoomClass::ZoomClass() {
  //  m_canv   = 0;
  //  m_CLICK  = 11;
}

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

  /*
  //erase old position and draw a line at current position
  int pyold = gPad->GetUniqueID();
  int px = gPad->GetEventX();
  int py = gPad->GetEventY();
  float uxmin = gPad->GetUxmin();
  float uxmax = gPad->GetUxmax();
  int pxmin = gPad->XtoAbsPixel(uxmin);
  int pxmax = gPad->XtoAbsPixel(uxmax);
  if(pyold) gVirtualX->DrawLine(pxmin,pyold,pxmax,pyold);
  gVirtualX->DrawLine(pxmin,py,pxmax,py);
  gPad->SetUniqueID(py);
  Float_t upy = gPad->AbsPixeltoY(py);
  Float_t y = gPad->PadtoY(upy);
  */
  
  //create or set the new canvas c2
  TVirtualPad *padsav = gPad;
  TCanvas *c2 = (TCanvas*)gROOT->GetListOfCanvases()->FindObject("c2");
  if(c2) delete c2->GetPrimitive("");
  else   c2 = new TCanvas("c2","",710,10,700,500);
  c2->SetGrid();
  c2->cd();

  //draw slice corresponding to mouse position
  /*
  Int_t biny = h->GetYaxis()->FindBin(y);
  TH1D *hp = h->ProjectionX("",biny,biny);
  hp->SetFillColor(38);
  char title[80];
  sprintf(title,"Projection of biny=%d",biny);
  hp->SetName("Projection");
  hp->SetTitle(title);
  hp->Fit("gaus","ql");
  hp->GetFunction("gaus")->SetLineColor(kRed);
  hp->GetFunction("gaus")->SetLineWidth(6);
  */
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

/*
inline void MyClass::MouseEvent() {

  TObject *select = gPad->GetSelected();
  if(!select) return;
  if (!select->InheritsFrom(TH2::Class())) {gPad->SetUniqueID(0); return;}
  TH2 *h = (TH2*)select;
  gPad->GetCanvas()->FeedbackMode(kTRUE);

  //create or set the new canvas c2
  TVirtualPad *padsav = gPad;
  TCanvas *c2 = (TCanvas*)gROOT->GetListOfCanvases()->FindObject("c2");
  if(c2) delete c2->GetPrimitive("Projection");
  else   c2 = new TCanvas("c2","Projection Canvas",710,10,700,500);
  c2->SetGrid();
  c2->cd();

  TH2D* htmp = (TH2D*)h->Clone();
  htmp->Draw();
  c2->Update();
  padsav->cd();
}
*/

#endif
