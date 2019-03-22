// =================================================
//  GETmePlots Class
//  ROOT-based GUI for GET electronics diagnostics
//
//  Author:
//    Giordano Cerizza ( cerizza@nscl.msu.edu )
//
// =================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <TROOT.h>
#include <TApplication.h>
#include <TVirtualX.h>
#include <TGClient.h>
#include <TCanvas.h>
#include <TGButton.h>
#include <TGFrame.h>
#include <TGMenu.h>
#include <TGDockableFrame.h>
#include <TGButtonGroup.h>
#include <TRootEmbeddedCanvas.h>
#include <TGComboBox.h>
#include <TGStatusBar.h>
#include <RQ_OBJECT.h>
#include <TGNumberEntry.h>
#include <TH1D.h>
#include <TGLabel.h>
#include <TGFileDialog.h>
#include <TString.h>
#include <TLegend.h>
#include "TPRegexp.h"
#include "TObjString.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <TStyle.h>
#include <TGTab.h>

#include "GETDecoder.h"
#include "AnalyzeFrame.h"

Bool_t fDebug = false;

const Int_t NCOBO=2;
const Int_t NASAD=2;
const Int_t NAGET=4;
const Int_t NCHN=68;
const Int_t NTBS=2;

TString host_default = "";

std::string filename, filename_open, filename_snap;

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_int_distribution<int> dist('a', 'z');

enum CommandIdentifiers {
  M_FILE_OPEN,
  M_FILE_EXIT,

  M_VIEW_ENBL_DOCK,
  M_VIEW_ENBL_HIDE,
  M_VIEW_DOCK,
  M_VIEW_UNDOCK

};
  
const char *filetypes[] = { "All files",     "*",
			    "EVT files",    "*.evt",
			    0,               0
};

class GETmePlotsClass : public TGMainFrame{
  
private:
  TRootEmbeddedCanvas *fEcanvas[NASAD][NAGET];
  TGCompositeFrame    *fF[NASAD][NAGET];
  TString             cname[NASAD][NAGET]; 
  
  TGMenuBar           *fMenuBar;
  TGDockableFrame     *fMenuDock;
  TGPopupMenu         *fMenuFile, *fMenuView;
  TGLayoutHints       *fMenuBarLayout, *fMenuBarItemLayout, *fComboBox;

  char                tmpCOBO[20], tmpASAD[20], tmpAGET[20], tmpCHN[20], tmpTB[20];
  TGComboBox          *fCOBObox, *fASADbox, *fAGETbox, *fCHNbox, *fTBbox;
  Int_t               fCOBO = 0;
  Int_t               fASAD = 0;
  Int_t               fAGET = 0;
  Int_t               fCHN = 0;
  Int_t               fTbs = 256;
  Int_t               fADC = 4095;
  TGTextEntry         *fmaxADC;
  TGTextEntry         *fHostRing;
  TGTextBuffer        *maxadc;
  TGTextBuffer        *hostring;  
  TGStatusBar         *fStatusBar;

  Int_t kk = 0;
  
  TGTextButton        *next;
  TGTextButton        *zoom;
  TGTextButton        *update;
  TGTextButton        *exit;
  TGTextButton        *reset;
  
  GETDecoder          *decoder = nullptr;
  Int_t               numEv =  0;
  TString             h_name[NASAD][NAGET][NCHN];
  TH1D                *h[NASAD][NAGET][NCHN];
  TH1D                *htmp = nullptr;
  TCanvas             *fCanvas = nullptr;
  TCanvas             *fCnv[NASAD][NAGET];
  TCanvas             *ctmp = nullptr;  
  Int_t               winCounter = 0;
  Int_t               winIter = 0;  

  TGTab               *fTab = nullptr;
  TGTabElement        *tab;

  TGCheckButton       *fOnline;
  Bool_t              fPrint=kFALSE;
  Bool_t              fOffline = kFALSE;
  TString             tmpTitle;
  TLegend             *legend;
  TString             leg_text;

  TString             hname;
  Bool_t              fEnable = kFALSE;
  
public:
  GETmePlotsClass(const TGWindow *p,UInt_t w,UInt_t h);
  virtual ~GETmePlotsClass();

  virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t);
  void ResetHisto();
  void CreateHistograms(Int_t asad, Int_t aget);
  void CreateCanvas();
  void SetStatusText(const std::string txt, Int_t id);
  void SetHistoProperties(TH1D* h);
  void UpdateHistograms(TH1D* h, Int_t asad, Int_t aget, Int_t channel);  
  void UpdateCanvas();
  TString GenerateName();
  void EnableAll();
  void DisableAll();
  
};

GETmePlotsClass::GETmePlotsClass(const TGWindow *p,UInt_t w,UInt_t h)
  : TGMainFrame(p, w, h)
{

  // Use hierarchical cleaning
  SetCleanup(kDeepCleanup);

  //////////////////////////////////////////////////////////////////////////////////  
  // Create menubar and popup menus. The hint objects are used to place
  // and group the different menu widgets with respect to eachother.
  //////////////////////////////////////////////////////////////////////////////////
  
  fMenuDock = new TGDockableFrame(this);
  AddFrame(fMenuDock, new TGLayoutHints(kLHintsExpandX, 0, 0, 1, 0));
  fMenuDock->SetWindowName("Gui Menu");

  // Layout options
  fMenuBarLayout = new TGLayoutHints(kLHintsTop | kLHintsExpandX);
  fMenuBarItemLayout = new TGLayoutHints(kLHintsTop | kLHintsLeft, 0, 4, 0, 0);
  fComboBox = new TGLayoutHints(kLHintsTop | kLHintsLeft,5,5,5,5);
  
  // File menu options
  fMenuFile = new TGPopupMenu(gClient->GetRoot());
  fMenuFile->AddEntry("&Attach file", M_FILE_OPEN);
  fMenuFile->AddEntry("&Exit", M_FILE_EXIT);

  fMenuView = new TGPopupMenu(gClient->GetRoot());
  fMenuView->AddEntry("&Dock", M_VIEW_DOCK);
  fMenuView->AddEntry("&Undock", M_VIEW_UNDOCK);
  fMenuView->AddSeparator();
  fMenuView->AddEntry("Enable U&ndock", M_VIEW_ENBL_DOCK);
  fMenuView->AddEntry("Enable &Hide", M_VIEW_ENBL_HIDE);
  fMenuView->DisableEntry(M_VIEW_DOCK);
  
  fMenuDock->EnableUndock(kTRUE);
  fMenuDock->EnableHide(kTRUE);
  fMenuView->CheckEntry(M_VIEW_ENBL_DOCK);
  fMenuView->CheckEntry(M_VIEW_ENBL_HIDE);
  
  // Options for menu bar
  fMenuBar = new TGMenuBar(fMenuDock, 1, 1, kHorizontalFrame);
  fMenuBar->AddPopup("&File", fMenuFile, fMenuBarItemLayout);
  fMenuBar->AddPopup("&View", fMenuView, fMenuBarItemLayout);
  
  // Add menu bar to menu dock
  fMenuDock->AddFrame(fMenuBar, fMenuBarLayout);

  // Bind calls
  fMenuFile->Associate(this);
  fMenuView->Associate(this);
  
  //////////////////////////////////////////////////////////////////////////////////
  // Create a horizontal frame widget with buttons for COBO
  //////////////////////////////////////////////////////////////////////////////////

  TGHorizontalFrame *hframe = new TGHorizontalFrame(this,200,40);

  // Create COBO menu
  const char *title = "COBO";  
  TGLabel *label = new TGLabel(hframe, title);
  hframe->AddFrame(label, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 10, 10, 10, 10));
  
  fCOBObox = new TGComboBox(hframe, 801);
  for (Int_t i = 0; i < NCOBO; i++) {
    sprintf(tmpCOBO, "COBO %d", i);
    fCOBObox->AddEntry(tmpCOBO, i+1);
  }
  hframe->AddFrame(fCOBObox, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 10, 10, 10));
  fCOBObox->Select(1);
  fCOBObox->Resize(100, 20);  
  
  // Next button to skip to the next event
  next = new TGTextButton(hframe,"&Next", 901);
  hframe->AddFrame(next, new TGLayoutHints(kLHintsNormal, 5,5,3,4));

  // Update button
  update = new TGTextButton(hframe,"&Update", 903);
  hframe->AddFrame(update, new TGLayoutHints(kLHintsNormal, 5,5,3,4));  
  
  // Reset button
  reset = new TGTextButton(hframe,"&Reset", 902);
  hframe->AddFrame(reset, new TGLayoutHints(kLHintsNormal, 5,5,3,4));

  // Exit button  
  exit = new TGTextButton(hframe,"&Exit", 904);
  hframe->AddFrame(exit, new TGLayoutHints(kLHintsNormal, 5,5,3,4));

  // Attach online ring
  const char *titleO = "tcp://";
  TGLabel *labelO = new TGLabel(hframe, titleO);
  hframe->AddFrame(labelO, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 10, 0, 10, 10));
  fHostRing = new TGTextEntry(hframe, hostring = new TGTextBuffer(11), 807);
  hostring->AddText(0,host_default);
  hframe->AddFrame(fHostRing, new TGLayoutHints(kLHintsCenterX|kLHintsCenterY, 5, 10, 10, 10));
  fHostRing->Resize(90, 20);

  fOnline = new TGCheckButton(hframe, new TGHotString("Enable/Disable\n     Online"), 909);
  hframe->AddFrame(fOnline, new TGLayoutHints(kLHintsNormal));
  
  AddFrame(hframe);

  // Binding calls
  fCOBObox->Associate(this);
  fHostRing->Associate(this);
  
  next->Associate(this);
  update->Associate(this);  
  reset->Associate(this);
  exit->Associate(this);
  fOnline->Associate(this);

  TGCompositeFrame *hframe2top = new TGCompositeFrame(this, 200, 40, kHorizontalFrame);
  TGCompositeFrame *hframeV = new TGCompositeFrame(hframe2top, 5, 5, kVerticalFrame);
  TGHorizontalFrame *hframeVH = new TGHorizontalFrame(hframeV, 5, 5);
  TGHorizontalFrame *hframeVH2 = new TGHorizontalFrame(hframeV, 5, 5);

  // Create ASAD menu
  const char *title2 = "ASAD";
  TGLabel *label2 = new TGLabel(hframeVH, title2);
  hframeVH->AddFrame(label2, new TGLayoutHints(kLHintsNormal, 5, 5, 0, 0));

  fASADbox = new TGComboBox(hframeVH, 802);
  for (Int_t i = 0; i < NASAD; i++) {
    sprintf(tmpASAD, "%d", i);
    fASADbox->AddEntry(tmpASAD, i+1);
  }
  fASADbox->Select(1);
  fASADbox->Resize(100, 20);
  hframeVH->AddFrame(fASADbox, new TGLayoutHints(kLHintsNormal, 0, 0, 0, 0));

  // Create AGET menu
  const char *title3 = "AGET";
  TGLabel *label3 = new TGLabel(hframeVH, title3);
  hframeVH->AddFrame(label3, new TGLayoutHints(kLHintsNormal, 5, 5, 1, 1));

  fAGETbox = new TGComboBox(hframeVH, 803);
  for (Int_t i = 0; i < NAGET; i++) {
    sprintf(tmpAGET, "%d", i);
    fAGETbox->AddEntry(tmpAGET, i+1);
  }
  fAGETbox->Select(1);
  fAGETbox->Resize(100, 20);
  hframeVH->AddFrame(fAGETbox, new TGLayoutHints(kLHintsNormal, 0, 0, 0, 0));
  hframeV->AddFrame(hframeVH, new TGLayoutHints(kLHintsNormal, 5, 5, 1, 1));

  // Create CHN menu
  const char *title4 = "Channel";
  TGLabel *label4 = new TGLabel(hframeVH2, title4);
  hframeVH2->AddFrame(label4, new TGLayoutHints(kLHintsNormal, 5, 5, 0, 0));

  fCHNbox = new TGComboBox(hframeVH2, 804);
  for (kk = 0; kk < NCHN; kk++) {
    sprintf(tmpCHN, "Channel %d", kk);
    fCHNbox->AddEntry(tmpCHN, kk+1);
  }
  fCHNbox->Select(1);
  fCHNbox->Resize(100, 20);
  hframeVH2->AddFrame(fCHNbox, new TGLayoutHints(kLHintsNormal, 5, 5, 0, 0));
  hframeV->AddFrame(hframeVH2, new TGLayoutHints(kLHintsNormal, 5, 5, 1, 1));

  hframe2top->AddFrame(hframeV, new TGLayoutHints(kLHintsNormal, 5, 5, 1, 1));

  TGHorizontalFrame *hframeH = new TGHorizontalFrame(hframe2top, 1, 1);

  // Next button to skip to the next event
  zoom = new TGTextButton(hframeH,"&Zoom", 908);
  hframeH->AddFrame(zoom, new TGLayoutHints(kLHintsNormal, 5,5,1,1));

  // Create time buckets option selection
  const char *title5 = "Time buckets";  
  TGLabel *label5 = new TGLabel(hframeH, title5);
  hframeH->AddFrame(label5, new TGLayoutHints(kLHintsNormal, 5, 5, 1, 1));

  fTBbox = new TGComboBox(hframeH, 805);
  for (Int_t i = 0; i < NTBS; i++) {
    auto val = (i+1)*256;
    sprintf(tmpTB, "%i", val);
    fTBbox->AddEntry(tmpTB, i+1);
  }
  hframeH->AddFrame(fTBbox, new TGLayoutHints(kLHintsNormal, 5, 5, 1, 1));
  fTBbox->Select(1);
  fTBbox->Resize(100, 20);

  // Create max ADC selection
  const char *title6 = "Max ADC channel";
  TGLabel *label6 = new TGLabel(hframeH, title6);
  hframeH->AddFrame(label6, new TGLayoutHints(kLHintsNormal, 5, 5, 1, 1));

  fmaxADC = new TGTextEntry(hframeH, maxadc = new TGTextBuffer(4), 806);
  char adc[4];
  sprintf(adc, "%d", fADC);
  maxadc->AddText(0,adc);
  hframeH->AddFrame(fmaxADC, new TGLayoutHints(kLHintsNormal, 5, 5, 1, 1));
  fmaxADC->Resize(100, 20);  

  hframe2top->AddFrame(hframeH, new TGLayoutHints(kLHintsNormal, 5, 5, 1, 1));  
  
  AddFrame(hframe2top);    

  // Binding calls
  fASADbox->Associate(this);  
  fAGETbox->Associate(this);
  fCHNbox->Associate(this);  
  zoom->Associate(this);
  fTBbox->Associate(this);
  fmaxADC->Associate(this);
  
  ////////////////////////////////////////////////////////////////////////////  
  // Create tabs for AsAd
  ////////////////////////////////////////////////////////////////////////////

  fTab = new TGTab(this, 200, 200);
  TGLayoutHints *fL = new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5);

  // Tab for AsAd 0-1 and AGET 0-3
  TGCompositeFrame *tf;
  Pixel_t green, white;
  TGTabElement *tabel[2];

  for (Int_t i=0; i< NASAD; i++){
    for (Int_t j=0; j< NAGET; j++){
      tf = fTab->AddTab(Form("AsAd %d - AGET %d", i, j));
      tabel[i] = fTab->GetTabTab(Form("AsAd %d - AGET %d", i, j));
      if (i==0){
	gClient->GetColorByName("dark green", green);
	tabel[i]->ChangeBackground(green);
      }
      else{
	gClient->GetColorByName("white", white);
	tabel[i]->ChangeBackground(white);
      }
      fF[i][j] =  new TGCompositeFrame(tf, 60, 60, kHorizontalFrame);
      cname[i][j] = Form("canvas_%d_%d", i, j);
      // Create canvas widget
      fEcanvas[i][j] = new TRootEmbeddedCanvas(cname[i][j], fF[i][j], 200, 200);
      fF[i][j]->AddFrame(fEcanvas[i][j], fL);
      tf->AddFrame(fF[i][j], fL);
    }
  }

  AddFrame(fTab, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10,10,10,10));
  
  ////////////////////////////////////////////////////////////////////////////  
  // Create status bar
  ////////////////////////////////////////////////////////////////////////////  

  Int_t parts[] = {50, 50};
  fStatusBar = new TGStatusBar(this,200,10,kHorizontalFrame);
  fStatusBar->SetParts(parts,2);
  AddFrame(fStatusBar, new TGLayoutHints(kLHintsBottom | kLHintsLeft | kLHintsExpandX, 0, 0, 2, 0));

  // Create canvas with empty histogram
  CreateCanvas();
  
  // Disable all the buttons except next and the file menu
  DisableAll();
  
  // Set a name to the main frame
  SetWindowName("GETmePlots");

  // Map all subwindows of main frame
  MapSubwindows();

  // Initialize the layout algorithm
  Resize();

  // Map main frame
  MapWindow();

}

void
GETmePlotsClass::DisableAll()
{
  fCOBObox->SetEnabled(kFALSE);
  fASADbox->SetEnabled(kFALSE);
  fAGETbox->SetEnabled(kFALSE);
  fCHNbox->SetEnabled(kFALSE);
  fTBbox->SetEnabled(kFALSE);
  fmaxADC->SetEnabled(kFALSE);

  reset->SetEnabled(kFALSE);
  zoom->SetEnabled(kFALSE);
  fHostRing->SetEnabled(kFALSE);
  
  fEnable =kFALSE;
}

void
GETmePlotsClass::EnableAll()
{
  fCOBObox->SetEnabled(kTRUE);
  fASADbox->SetEnabled(kTRUE);
  fAGETbox->SetEnabled(kTRUE);
  fCHNbox->SetEnabled(kTRUE);  
  fTBbox->SetEnabled(kTRUE);
  fmaxADC->SetEnabled(kTRUE);
  
  reset->SetEnabled(kTRUE);
  zoom->SetEnabled(kTRUE);
  if (filename == "")
    fHostRing->SetEnabled(kTRUE);  
  else
    fHostRing->SetEnabled(kFALSE);      

  fEnable =kTRUE;
}

TString
GETmePlotsClass::GenerateName()
{
  std::string result;
  std::generate_n(std::back_inserter(result), 8, [&]{return dist(mt);});
  hname = (TString)result;
  return hname;
}

void
GETmePlotsClass::SetStatusText(const std::string txt, Int_t id)
{
  // Set text in status bar position id
  auto c = txt.c_str();
  fStatusBar->SetText(c,id);
}

void
GETmePlotsClass::CreateCanvas()
{
  for (Int_t k=0; k<NASAD; k++) {
    for (Int_t j=0; j<NAGET; j++) {
      fCnv[k][j]=fEcanvas[k][j]->GetCanvas();
      fCnv[k][j]->Divide(9,8);

      CreateHistograms(k,j);          
      
      Int_t z = 0;
      for (Int_t i=0; i<NCHN; i++){
	z++;
	fCnv[k][j]->cd(z);
	SetHistoProperties(h[k][j][i]);
	h[k][j][i]->Draw("hist");
      }
      fCnv[k][j]->Modified();
      fCnv[k][j]->Update();
    }
  }
}

void
GETmePlotsClass::SetHistoProperties(TH1D* h)
{
  h->SetStats(0);
  h->SetLineColor(2);
  h->SetLineWidth(3);
}

void
GETmePlotsClass::CreateHistograms(Int_t asad, Int_t aget)
{
  for (Int_t i=0; i<NCHN; i++){
    // Create title
    if (asad == 0)
      tmpTitle = Form("AsAd %d Aget %d Channel %d Event %d", asad, aget, i, (numEv+1)/2);
    else
      tmpTitle = Form("AsAd %d Aget %d Channel %d Event %d", asad, aget, i, (numEv/2));

    tmpTitle = tmpTitle+";Time buckets;ADC channel";
    // Create histograms
    h_name[asad][aget][i] = Form("h_%d_%d_%d", asad, aget, i);
    h[asad][aget][i] = new TH1D(h_name[asad][aget][i],tmpTitle,fTbs,0,fTbs);
  }
}

void
GETmePlotsClass::UpdateHistograms(TH1D* h, Int_t asad, Int_t aget, Int_t channel)
{
  // Update number of bins
  h->SetBins(fTbs, 0, fTbs);
  // Update maximum adc
  h->SetMaximum(fADC);
  // Update title
  if (asad == 0)
    tmpTitle = Form("AsAd %d Aget %d Channel %d Event %d", asad, aget, channel, (numEv+1)/2);
  else if (asad == 1){
    tmpTitle = Form("AsAd %d Aget %d Channel %d Event %d", asad, aget, channel, (numEv/2));
  }
  h->SetTitleSize(5,"T");
  h->SetTitle(tmpTitle);
}

void
GETmePlotsClass::UpdateCanvas()
{
  std::vector<int> v;
  for (Int_t i=0; i<NASAD*NAGET; i++){
    tab = fTab->GetTabTab(i);
    std::string label = tab->GetString();
    for (int i = 0; i < strlen(label.c_str()); i++) {
      if (std::isdigit(label[i])) {
	int num = (int)label[i]-48;
	v.push_back(num);
      }    
    }
  }

  for (Int_t i=0; i<v.size()/2; i++){
    fCnv[v[2*i]][v[2*i+1]]->Modified();
    fCnv[v[2*i]][v[2*i+1]]->Update();    
  }

}

void GETmePlotsClass::ResetHisto()
{
  Int_t z = 0;
  for (Int_t k=0; k<NASAD; k++){
    for (Int_t j=0; j<NAGET; j++){
      for (Int_t i=0; i<NCHN; i++){
	z++;
	fCnv[k][j]->cd(z);
	h[k][j][i]->Reset("ICESM");
      }
    }
  }
}

Bool_t GETmePlotsClass::ProcessMessage(Long_t msg, Long_t parm1, Long_t)
{
  char            str[4];
  Int_t           coboID;
  Int_t           asadID;
  Int_t           agetID;
  Int_t           chnID;
  std::vector<NSCLGET::Hit> frame;
  char            text_bar[100];
  
  // Handle messages send to the GETmePlotsClass object. E.g. all menu button messages.
  // Type of calls that are binded by id
  // kC_TEXTENTRY: button with numerical text entry
  // kC_COMMAND:
  //     - kCM_CHECKBUTTON: check button
  //     - kCM_COMBOBOX: opens a button-like menu with several options
  //     - kCM_BUTTON: normal pushable button
  //     - kCM_MENU: classic file menu style

  switch (GET_MSG(msg)) {

  case kC_TEXTENTRY:
    switch (GET_SUBMSG(msg)) {
    case kTE_TEXTCHANGED:
      switch (parm1) {
      case 806:
	if (atoi(maxadc->GetString()) > 4095 || atoi(maxadc->GetString())<0){
	  if (atoi(maxadc->GetString()) < 0){
	    std::cout << "Minimum value is 0!" << std::endl;
	    fADC = 0;
	  }
	  else if (atoi(maxadc->GetString()) > 4095){
	    std::cout << "Maximum value is 4095!" << std::endl;
	    fADC = 4095;
	  }
	  
	  sprintf(str, "%d", fADC);
	} else {
	  fADC = atoi(maxadc->GetString()); 
	}
	break;
      case 807:
	std::string host = "tcp://"+ (std::string)hostring->GetString();
	filename = host;
	SetStatusText(filename,0);
      }	
    default:
      break;
    }
    
  case kC_COMMAND:
    switch (GET_SUBMSG(msg)) {
            
    case kCM_CHECKBUTTON:
      switch (parm1) {
      case 909:
	if (!fOffline){
	  std::cout << "== [GETDecoder] ONLINE mode ON" << std::endl;
	  EnableAll();
	  fOffline = kTRUE;
	}
	else {
	  std::cout << "== [GETDecoder] ONLINE mode OFF" << std::endl;
	  DisableAll();
	  fOffline = kFALSE;	  
	}
	break;
      case 905:
	if (fPrint){
	  std::cout << "== [GETDecoder] debug mode OFF " << std::endl;
	  fPrint=kFALSE;
	  break;
	}
	std::cout << "== [GETDecoder] debug mode ON " << std::endl;
	fPrint=kTRUE;
	break;
      default:
	break;
      }
      
    case kCM_COMBOBOX:
      switch (parm1) {

      case 801:
	fCOBO = fCOBObox->GetSelected()-1;
	std::cout << "Selected COBO " << fCOBO << std::endl;
	break;
      case 802:
	fASAD = fASADbox->GetSelected()-1;
	std::cout << "Selected ASAD " << fASAD << std::endl;
	break;
      case 803:
	fAGET = fAGETbox->GetSelected()-1;
	std::cout << "Selected AGET " << fAGET << std::endl;
	break;
      case 804:
	fCHN = fCHNbox->GetSelected()-1;
	std::cout << "Selected CHANNEL " << fCHN << std::endl;
	break;		
      case 805:
	fTbs = fTBbox->GetSelected()*256;
	std::cout << "Selected " << fTbs << " time buckets" << std::endl;
	break;	  
      default:	
	break;
      }

    case kCM_BUTTON:
      switch (parm1) {
      case 901:
	if (filename == ""){
	  std::cout << "\t\t\t\t >>> Please open a file...(File->Open). Try again! <<<" << std::endl;
	  break;
	}
	// Create GETDecoder only the first time
	if (!decoder || (filename != filename_open)){
	  decoder = GETDecoder::getInstance();
	  decoder->SetUrls(filename);
	  
	  filename_open = filename;
	  numEv=0;
	}
	numEv++;

	std::cout << "Event " << numEv << std::endl;
	
	frame = decoder->GetFrame();

	// Reset histograms everytime we click next
	ResetHisto();

	winCounter = 0;
	winIter = 0;	
	for(auto&& hit: frame){
	  coboID = hit.s_cobo;
	  asadID = hit.s_asad;
	  agetID = hit.s_aget;	  
	  chnID = hit.s_chan;

	  winCounter++;
	  
	  if (fDebug)
	    std::cout << coboID << " " << asadID << " " << agetID << " " << chnID << std::endl;
	  
	  // Data sanity check
	  if (coboID != fCOBO){
	    std::cout << "Selected COBO doesn't exist in data!" << std::endl;
	    break;
	  }

	  winIter = winCounter - agetID*NCHN;	  
	  for(auto&& hh: hit.s_rawADC){
	    int tb = hh.first;
	    int adc = hh.second;
	    fCnv[asadID][agetID]->cd(winIter);
	    if (fDebug)
	      std::cout << asadID << " " << agetID << " " << winIter << std::endl;
	    if (tb < fTbs) {
	      if (fDebug)
		std::cout << "\t\t\t" << chnID << " " << tb << " " << adc << std::endl;
	      h[asadID][agetID][chnID]->Fill(tb, adc);
	    }
	  }
	  UpdateHistograms(h[asadID][agetID][chnID], asadID, agetID, chnID);
	  h[asadID][agetID][chnID]->Draw("hist");
	}
	UpdateCanvas();
	
	break;
	
      case 902:
	// reset number of events and histograms
	ResetHisto();
	UpdateCanvas();
	decoder = nullptr;
	numEv = 0;
	DisableAll();
	filename = "";
	SetStatusText("", 1);
	break;
	
      case 903:
	// This method is for updating the histogram propertie
	for (Int_t k=0; k<NASAD; k++){
	  for (Int_t j=0; j<NAGET; j++){
	    for (Int_t i=0; i<NCHN; i++){
	      fCnv[k][j]->cd(i);
	      UpdateHistograms(h[k][j][i], k, j, i);
	    }
	  }
	}
	UpdateCanvas();
	break;
      case 904:
	gApplication->Terminate(0);
      case 908:
	ctmp = new TCanvas(GenerateName(), Form("AsAd %i, Aget %i, Channel %i", fASAD, fAGET, fCHN), 700, 500);
	ctmp->cd();
	ctmp->ToggleEventStatus();
	htmp = (TH1D*)h[fASAD][fAGET][fCHN]->Clone();
	UpdateHistograms(htmp, fASAD, fAGET, fCHN);
	htmp->Draw("hist");
	break;

      default:
	break;
      }

    case kCM_MENU:
      switch (parm1) {

      case M_FILE_OPEN:
	{
	  static TString dir(".");
	  TGFileInfo          fi;
	  fi.fFileTypes = filetypes;
	  fi.fIniDir    = StrDup(dir);
	  new TGFileDialog(gClient->GetRoot(), this, kFDOpen, &fi);
	  char buff[1000];
	  snprintf(buff, sizeof(buff), "file://%s", fi.fFilename);
	  filename = buff;
	  dir = fi.fIniDir;
	  SetStatusText(filename,0);
	  if (!fEnable)
	    EnableAll();
	}
	break;

      case M_VIEW_ENBL_DOCK:
	fMenuDock->EnableUndock(!fMenuDock->EnableUndock());
	if (fMenuDock->EnableUndock()) {
	  fMenuView->CheckEntry(M_VIEW_ENBL_DOCK);
	  fMenuView->EnableEntry(M_VIEW_UNDOCK);
	} else {
	  fMenuView->UnCheckEntry(M_VIEW_ENBL_DOCK);
	  fMenuView->DisableEntry(M_VIEW_UNDOCK);
	}
	break;

      case M_VIEW_ENBL_HIDE:
	fMenuDock->EnableHide(!fMenuDock->EnableHide());
	if (fMenuDock->EnableHide()) {
	  fMenuView->CheckEntry(M_VIEW_ENBL_HIDE);
	} else {
	  fMenuView->UnCheckEntry(M_VIEW_ENBL_HIDE);
	}
	break;

      case M_VIEW_DOCK:
	fMenuDock->DockContainer();
	fMenuView->EnableEntry(M_VIEW_UNDOCK);
	fMenuView->DisableEntry(M_VIEW_DOCK);
	break;

      case M_VIEW_UNDOCK:
	fMenuDock->UndockContainer();
	fMenuView->EnableEntry(M_VIEW_DOCK);
	fMenuView->DisableEntry(M_VIEW_UNDOCK);
	break;

      default:
	break;
      }
    default:
      break;
    }
  default:
    break;
  }

  if (fMenuDock->IsUndocked()) {
    fMenuView->EnableEntry(M_VIEW_DOCK);
    fMenuView->DisableEntry(M_VIEW_UNDOCK);
  } else {
    fMenuView->EnableEntry(M_VIEW_UNDOCK);
    fMenuView->DisableEntry(M_VIEW_DOCK);
  }
  
  return kTRUE;
}

GETmePlotsClass::~GETmePlotsClass() {
  // Clean up used widgets: frames, buttons, layout hints
  SetCleanup(kDeepCleanup);
  gApplication->Terminate(0);
}

int main(int argc, char **argv)
{
  TApplication theApp("App", &argc, argv);

  if (gROOT->IsBatch()) {
    fprintf(stderr, "%s: cannot run in batch mode\n", argv[0]);
    return 1;
  }

  GETmePlotsClass mainWindow(gClient->GetRoot(), 200, 200);

  theApp.Run();

  return 0;
}
