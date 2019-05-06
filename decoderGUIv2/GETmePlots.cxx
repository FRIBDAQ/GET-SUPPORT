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
#include <TQObject.h>
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

const Int_t NCOBO=1;
const Int_t NASAD=2;
const Int_t NAGET=4;
const Int_t NCHN=68;
const Int_t NTBS=512;

TString host_default = "spdaq08/GET";

std::string filename, filename_open, filename_snap;

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_int_distribution<int> dist('a', 'z');

enum CommandIdentifiers {
  M_FILE_OPEN,
  M_FILE_EXIT 
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

  Bool_t              tabTable[8];
  
  TGMenuBar           *fMenuBar;
  TGCompositeFrame    *fMenuDock;
  TGPopupMenu         *fMenuFile; 
  TGLayoutHints       *fMenuBarLayout, *fMenuBarItemLayout, *fComboBox;

  char                tmpCOBO[20];
  TGComboBox          *fCOBObox;
  Int_t               fCOBO = 0;
  Int_t               fASAD = 0;
  Int_t               fAGET = 0;
  Int_t               fCHN = 0;
  Int_t               fTbs = 512;
  Int_t               fADC = 4095;
  Int_t    ASADtab;
  Int_t    AGETtab;
  Int_t    oldASADtab = -1;
  Int_t    oldAGETtab = -1;
  TGTextEntry         *fTBbox;  
  TGTextEntry         *fmaxADC;
  TGTextEntry         *fHostRing;
  TGTextBuffer        *ntb;
  TGTextBuffer        *maxadc;
  TGTextBuffer        *hostring;  
  TGStatusBar         *fStatusBar;

  Int_t kk = 0;

  TGTextButton        *snap;  
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
  Bool_t              fOffline = kFALSE;
  TString             tmpTitle;
  TLegend             *legend;
  TString             leg_text;

  TString             hname;
  Bool_t              fEnable = kFALSE;
  Bool_t              fDebug = false;
  Bool_t              fDone = false;
  Bool_t              fSnapshot = false;
  
  
public:
  GETmePlotsClass(const TGWindow *p,UInt_t w,UInt_t h);
  virtual ~GETmePlotsClass();

  virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t);
  void EnableAll();
  void DisableAll();
  void DisableTabs();
  void UpdateTabs();    
  void CreateHistograms();
  void CreateCanvas();
  TString GenerateName();
  void SetStatusText(const std::string txt, Int_t id);
  void ResetHisto();
  void Update();
  void DrawHistograms(int asad, int aget);
  void UpdateHistograms(int asad, int aget);
  void UpdateCanvas(int asad, int aget);  
  void Next();
  void FillHisto();
  void Snapshot();
  static void ZoomChannel(int tbs, int adc);
  
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
  
  fMenuDock = new TGCompositeFrame(this);
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

  // Options for menu bar
  fMenuBar = new TGMenuBar(fMenuDock, 1, 1, kHorizontalFrame);
  fMenuBar->AddPopup("&File", fMenuFile, fMenuBarItemLayout);
  
  // Add menu bar to menu dock
  fMenuDock->AddFrame(fMenuBar, fMenuBarLayout);

  // Bind calls
  fMenuFile->Associate(this);
  
  //////////////////////////////////////////////////////////////////////////////////
  // Create a horizontal frame widget with buttons for COBO, ASAD, AGET
  //////////////////////////////////////////////////////////////////////////////////

  TGHorizontalFrame *hframe = new TGHorizontalFrame(this,200,40);

  // Create COBO menu
  fCOBObox = new TGComboBox(hframe, 801);
  for (Int_t i = 0; i < NCOBO; i++) {
    sprintf(tmpCOBO, "COBO %d", i);
    fCOBObox->AddEntry(tmpCOBO, i+1);
  }
  hframe->AddFrame(fCOBObox, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 10, 10, 10));
  fCOBObox->Select(1);
  fCOBObox->Resize(100, 20);  
  
  // Snapshot button to sample from ringbuffer
  snap = new TGTextButton(hframe,"&Snap", 900);
  hframe->AddFrame(snap, new TGLayoutHints(kLHintsNormal, 5,5,3,4));
  
  // Next button to skip to the next event
  next = new TGTextButton(hframe,"&Next", 901);
  hframe->AddFrame(next, new TGLayoutHints(kLHintsNormal, 5,5,3,4));

  // Update button
  update = new TGTextButton(hframe,"&Update", 902);
  hframe->AddFrame(update, new TGLayoutHints(kLHintsNormal, 5,5,3,4));  
  
  // Reset button
  reset = new TGTextButton(hframe,"&Reset", 903);
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
  //  fAGETbox->Associate(this);
  fHostRing->Associate(this);

  snap->Associate(this);  
  next->Associate(this);
  update->Associate(this);  
  reset->Associate(this);
  exit->Associate(this);
  fOnline->Associate(this);

  TGCompositeFrame *hframe2top = new TGCompositeFrame(this, 200, 40, kHorizontalFrame);
  TGHorizontalFrame *hframeH = new TGHorizontalFrame(hframe2top, 1, 1);
  
  // Create time buckets option selection
  const char *title5 = "Time buckets";  
  TGLabel *label5 = new TGLabel(hframeH, title5);
  hframeH->AddFrame(label5, new TGLayoutHints(kLHintsNormal, 5, 5, 1, 1));

  fTBbox = new TGTextEntry(hframeH, ntb = new TGTextBuffer(3), 805);
  char tb[4];
  sprintf(tb, "%d", fTbs);
  ntb->AddText(0,tb);
  hframeH->AddFrame(fTBbox, new TGLayoutHints(kLHintsNormal, 5, 5, 1, 1));
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
  fTBbox->Associate(this);
  fmaxADC->Associate(this);
  
  ////////////////////////////////////////////////////////////////////////////  
  // Create tabs for AsAd
  ////////////////////////////////////////////////////////////////////////////

  fTab = new TGTab(this, 200, 200);
  TGLayoutHints *fL = new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5);

  // Tab for AsAd 0-1 and AGET 0-3
  TGCompositeFrame *tf;

  for (Int_t asad=0; asad< NASAD; asad++){
    for (Int_t aget=0; aget< NAGET; aget++){
      tf = fTab->AddTab(Form("AsAd %d - AGET %d", asad, aget));
      fF[asad][aget] =  new TGCompositeFrame(tf, 60, 60, kHorizontalFrame);
      cname[asad][aget] = Form("canvas_%d_%d", asad, aget);
      // Create canvas widget
      fEcanvas[asad][aget] = new TRootEmbeddedCanvas(cname[asad][aget], fF[asad][aget], 200, 200);
      fF[asad][aget]->AddFrame(fEcanvas[asad][aget], fL);
      tf->AddFrame(fF[asad][aget], fL);
      tabTable[asad*aget+aget]=false;
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

  // Create histograms
  CreateHistograms();
  
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

  // Creates the zoom canvas and keep it up to date
  gInterpreter->Declare("#include \"../include/ZoomClass.h\"");
  
}

void
GETmePlotsClass::DisableAll()
{
  fCOBObox->SetEnabled(kFALSE);
  fTBbox->SetEnabled(kFALSE);
  fmaxADC->SetEnabled(kFALSE);

  DisableTabs();

  snap->SetEnabled(kFALSE);
  next->SetEnabled(kFALSE);
  update->SetEnabled(kFALSE);    
  reset->SetEnabled(kFALSE);
  fHostRing->SetEnabled(kFALSE);
  
  fEnable =kFALSE;
}

void
GETmePlotsClass::EnableAll()
{
  fCOBObox->SetEnabled(kTRUE);
  fTBbox->SetEnabled(kTRUE);
  fmaxADC->SetEnabled(kTRUE);

  snap->SetEnabled(kTRUE);
  next->SetEnabled(kTRUE);
  update->SetEnabled(kTRUE);      
  reset->SetEnabled(kTRUE);

  if (filename == "")
    fHostRing->SetEnabled(kTRUE);  
  else
    fHostRing->SetEnabled(kFALSE);      

  fEnable =kTRUE;
}

void
GETmePlotsClass::DisableTabs()
{
  for(int i=0; i<8; i++)
    fTab->GetTabTab(i)->SetEnabled(tabTable[i]);
}

void
GETmePlotsClass::UpdateTabs()
{
  Pixel_t back = GetDefaultFrameBackground();
  Pixel_t green;
  gClient->GetColorByName("dark green", green);
  for(int i=0; i<8; i++){
    fTab->GetTabTab(i)->ChangeBackground(back);    
    fTab->GetTabTab(i)->SetEnabled(tabTable[i]);
    fTab->GetTabTab(i)->SetActive();
    if (tabTable[i])
      fTab->GetTabTab(i)->ChangeBackground(green);
  }
  if (ASADtab == 0)
    fTab->SetTab(0);
  else
    fTab->SetTab(4);    
  AGETtab = 0;

}
  
void
GETmePlotsClass::CreateHistograms()
{
  for (Int_t k=0; k<NASAD; k++){
    for (Int_t j=0; j<NAGET; j++){
      for (Int_t i=0; i<NCHN; i++){
	// Create title
	if (k == 0)
	  tmpTitle = Form("AsAd %d Aget %d Channel %d Event %d", k, j, i, (numEv+1)/2);
	else
	  tmpTitle = Form("AsAd %d Aget %d Channel %d Event %d", k, j, i, (numEv/2));
	
	tmpTitle = tmpTitle+";Time buckets;ADC channel";
	// Create histograms
	h_name[k][j][i] = Form("h_%d_%d_%d", k, j, i);
	h[k][j][i] = new TH1D(h_name[k][j][i],tmpTitle,fTbs,0,fTbs);
	h[k][j][i]->SetStats(0);
	h[k][j][i]->SetLineColor(2);
	h[k][j][i]->SetLineWidth(3);
	h[k][j][i]->SetMinimum(0);
	h[k][j][i]->SetMaximum(fADC);
	h[k][j][i]->SetTitleSize(5,"T");
	h[k][j][i]->SetTitle(tmpTitle);
      }
    }
  }
}

void
GETmePlotsClass::CreateCanvas()
{
  for (Int_t k=0; k<NASAD; k++) {
    for (Int_t j=0; j<NAGET; j++) {
      fCnv[k][j]=fEcanvas[k][j]->GetCanvas();
      fCnv[k][j]->Divide(9,8);
      fCnv[k][j]->Modified();
      fCnv[k][j]->Update();
    }
  }
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
GETmePlotsClass::ResetHisto()
{
  for (Int_t k=0; k<NASAD; k++){
    for (Int_t j=0; j<NAGET; j++){
      for (Int_t i=0; i<NCHN; i++){
	h[k][j][i]->Reset("ICESM");
      }
    }
  }
}

void
GETmePlotsClass::Next()
{
  fDone = false;

  if (filename == ""){
    std::cout << "\t\t\t\t >>> Please open a file...(File->Open file) or attach an online ring buffer. Try again! <<<" << std::endl;
    return;
  }

  if (!fSnapshot){
    // Create GETDecoder only the first time
    if (!decoder || (filename != filename_open)){
      if (fDebug)
	std::cout << "Inside GETmePlots !decoder" << std::endl;
      decoder = GETDecoder::getInstance();
      decoder->SetUrls(filename);

      filename_open = filename;
    }
  }
}

void
GETmePlotsClass::FillHisto()
{
  Int_t           coboID;
  Int_t           asadID;
  Int_t           agetID;
  Int_t           chnID;
  std::vector<NSCLGET::Hit> frame;

  if (fDebug)
    std::cout << "Filling histograms..." << std::endl;

  std::cout << "Event " << numEv << std::endl;
  numEv++;

  frame = decoder->GetFrame();

  if (fDebug)
    std::cout << "Inside fillHistograms - before frame loop" << std::endl;

  // Reset tabs
  for (int i=0; i<NASAD*NAGET; i++){
    tabTable[i] = false;
    if (fDebug)
      std::cout << tabTable[i];
  }
  if (fDebug)
    std::cout << std::endl;
  
  ResetHisto();
  
  for(auto&& hit: frame){
    coboID = hit.s_cobo;
    asadID = hit.s_asad;
    agetID = hit.s_aget;
    chnID = hit.s_chan;

    if (fDebug)
      std::cout << coboID << " " << asadID << " " << agetID << " " << chnID << std::endl;

    if (asadID == 0)
      tabTable[agetID] = true;
    else
      tabTable[NAGET+agetID] = true;

    // Data sanity check
    if (coboID != fCOBO){
      std::cout << "Selected COBO doesn't exist in data!" << std::endl;
      break;
    }

    for(auto&& hh: hit.s_rawADC){
      int tb = hh.first;
      int adc = hh.second;
      if (tb <= fTbs) {
	if (fDebug){
	  std::cout << fTbs << std::endl;
	  std::cout << "\t\t\t" << chnID << " " << tb << " " << adc << std::endl;
	}
	h[asadID][agetID][chnID]->SetBins(fTbs, 0, fTbs);
	h[asadID][agetID][chnID]->Fill(tb, adc);
      }
    }
  }

  if (fDebug){
    std::cout << "Done filling histograms..." << std::endl;
    for (int i=0; i<8; i++)
      std::cout << tabTable[i];
    std::cout << std::endl;
  }

  if (tabTable[0] || tabTable[1] || tabTable[2] || tabTable[3])
    ASADtab = 0;    
  else if (tabTable[4] || tabTable[5] || tabTable[6] || tabTable[7]) 
    ASADtab = 1;    

}

void
GETmePlotsClass::Update()
{

  if (fDone == false){
    if (fDebug)
      std::cout << "Plotting histograms in ASADtab " << ASADtab << " AGETtab " << AGETtab << std::endl;
    DrawHistograms(ASADtab, AGETtab);
    oldASADtab = ASADtab;
    oldAGETtab = AGETtab;
  } else {
    if (oldASADtab != ASADtab || oldAGETtab != AGETtab) {
      if (fDebug){
	std::cout << "Hey you switched tab! From ASADtab " << oldASADtab << " to " << ASADtab << " and from AGETtab " << oldAGETtab << " to " << AGETtab << std::endl;
	std::cout << "I need to plot new histograms..." << std::endl;
      }
      DrawHistograms(ASADtab, AGETtab);
      oldASADtab = ASADtab;
      oldAGETtab = AGETtab;
    }
    else {
      if (fDebug)
	std::cout << "You didn't switch time. Which is good. So I'll just update the histogram with the new parameters" << std::endl;
      UpdateHistograms(ASADtab, AGETtab);
    }
  }
  UpdateCanvas(ASADtab, AGETtab);
}

void
GETmePlotsClass::DrawHistograms(int asad, int aget)
{
  Int_t z = 0;
  for (Int_t i=0; i<NCHN; i++){
    ++z;
    fEcanvas[asad][aget]->GetCanvas()->cd(z);
    if (asad == 0)
      tmpTitle = Form("AsAd %d Aget %d Channel %d Event %d", asad, aget, i, (numEv+1)/2);
    else
      tmpTitle = Form("AsAd %d Aget %d Channel %d Event %d", asad, aget, i, (numEv/2));
    h[asad][aget][i]->SetBins(fTbs, 0, fTbs);
    h[asad][aget][i]->SetMinimum(0);
    h[asad][aget][i]->SetMaximum(fADC);
    h[asad][aget][i]->SetTitle(tmpTitle);
    h[asad][aget][i]->Draw("histo");
  }

  oldASADtab = ASADtab;
  oldAGETtab = AGETtab;
  
  fDone = true;
}

void
GETmePlotsClass::UpdateCanvas(int asad, int aget)
{
  fEcanvas[asad][aget]->GetCanvas()->Modified();
  fEcanvas[asad][aget]->GetCanvas()->Update();
  fEcanvas[asad][aget]->GetCanvas()->cd();
  gPad->AddExec("zoom",Form("ZoomClass::ZoomChannel(%d, %d)", fTbs, fADC));
}

void
GETmePlotsClass::UpdateHistograms(int asad, int aget)
{
  Int_t z = 0;
  for (Int_t i=0; i<NCHN; i++){
    ++z;
    fCnv[asad][aget]->cd(z);    
    h[asad][aget][i]->SetBins(fTbs, 0, fTbs);
    h[asad][aget][i]->SetMinimum(0);
    h[asad][aget][i]->SetMaximum(fADC);
    h[asad][aget][i]->Draw("histo");
  }
}

void
GETmePlotsClass::Snapshot()
{
  fSnapshot = true;
  numEv = 0;
  
  if (filename == ""){
    std::cout << "\t\t\t\t >>> Please open a file...(File->Open file) or attach an online ring buffer. Try again! <<<" << std::endl;
    return;
  }
  
  if (filename != "")
    EnableAll();
  
  if (!decoder || (filename != filename_open)){
    if (fDebug)
      std::cout << "Inside GETmePlots::Snapshot !decoder" << std::endl;
    decoder = GETDecoder::getInstance();
    filename = decoder->SetDataSink(filename, "file:///tmp/snapshot.evt", 100);
    
    filename_open = "ringbuffer";
  }
  std::cout << "Snapshot of 100 ring items from " << filename << " ready to be analyzed!" << std::endl;
}


Bool_t GETmePlotsClass::ProcessMessage(Long_t msg, Long_t parm1, Long_t)
{
  // Handle messages send to the GETmePlotsClass object. E.g. all menu button messages.
  // Type of calls that are binded by id
  // kC_TEXTENTRY: button with numerical text entry
  // kC_COMMAND:
  //     - kCM_CHECKBUTTON: check button
  //     - kCM_COMBOBOX: opens a button-like menu with several options
  //     - kCM_BUTTON: normal pushable button
  //     - kCM_MENU: classic file menu style
  char            str[5];

  switch (GET_MSG(msg)) {

  case kC_TEXTENTRY:
    switch (GET_SUBMSG(msg)) {
    case kTE_TEXTCHANGED:
      switch (parm1) {

      case 805:
	if (atoi(ntb->GetString()) > 512){
	  std::cout << "Minimum value is 512!" << std::endl;
	  fTbs = 512;
	  ntb->Clear();	  
	  sprintf(str, "%d", fTbs);
	  ntb->AddText(0,str);	  
	}
	else
	  {
	    fTbs = atoi(ntb->GetString());
	    if (fDebug)
	      std::cout << "Number of tb: " << fTbs << std::endl;
	  }
	break;
	
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
	  break;
	}	
      default:
	break;
      }
      
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
	
      default:
	break;
      }
      
    case kCM_COMBOBOX:

      switch (parm1) {
      case 801:
	{
	fCOBO = fCOBObox->GetSelected()-1;
	std::cout << "Selected COBO " << fCOBO << std::endl;
	}
	break;
	
      default:	
	break;
      }

    case kCM_TAB:
      switch (parm1) {
      case 0:
	{
	  ASADtab = 0;
	  AGETtab = 0;
	  if (fDebug)
	    std::cout << ASADtab << " " << AGETtab << std::endl;
	}
	break;
      case 1:
	{
	  ASADtab = 0;
	  AGETtab = 1;
	  if (fDebug)
	    std::cout << ASADtab << " " << AGETtab << std::endl;
	}
	break;
      case 2:
	{
	  ASADtab = 0;
	  AGETtab = 2;
	  if (fDebug)
	    std::cout << ASADtab << " " << AGETtab << std::endl;
	}
	break;
      case 3:
	{
	  ASADtab = 0;
	  AGETtab = 3;
	  if (fDebug)
	    std::cout << ASADtab << " " << AGETtab << std::endl;
	}
	break;
      case 4:
	{
	  ASADtab = 1;
	  AGETtab = 0;
	  if (fDebug)
	    std::cout << ASADtab << " " << AGETtab << std::endl;
	}
	break;
      case 5:
	{
	  ASADtab = 1;
	  AGETtab = 1;
	  if (fDebug)
	    std::cout << ASADtab << " " << AGETtab << std::endl;
	}
	break;
      case 6:
	{
	  ASADtab = 1;
	  AGETtab = 2;
	  if (fDebug)
	    std::cout << ASADtab << " " << AGETtab << std::endl;
	}
	break;
      case 7:
	{
	  ASADtab = 1;
	  AGETtab = 3;
	  if (fDebug)
	    std::cout << ASADtab << " " << AGETtab << std::endl;
	}
	break;
	
      default:	
	break;
      }
      
    case kCM_BUTTON:

      switch (parm1) {
      case 900:
	Snapshot();
	break;
      case 901:
	{
	  Next();
	  FillHisto();
	  UpdateTabs();
	  Update();
	}
	break;
      case 902:
	Update();
	break;
      case 903:
	{
	  ResetHisto();
	  UpdateCanvas(ASADtab,AGETtab);
	  decoder = nullptr;
	  numEv = 0;
	  DisableAll();
	  filename = "";
	  SetStatusText("", 1);
	}
	break;
      case 904:
	gApplication->Terminate(0);
	
      default:
	break;
      }
      
    default:
      break;
    }
    
  default:
    break;
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
