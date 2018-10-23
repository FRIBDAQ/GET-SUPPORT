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
#include <RQ_OBJECT.h>
#include <TGNumberEntry.h>
#include <TH1D.h>
#include <TGLabel.h>
#include <TGFileDialog.h>
#include "GETDecoder.hh"
#include <TString.h>

const Int_t NCOBO=2;
const Int_t NASAD=2;
const Int_t NAGET=4;
const Int_t NCHN=68;
const Int_t NTBS=2;

TString filename;

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
  TRootEmbeddedCanvas *fEcanvas;
  
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
  Int_t               fTbs, fADC=4095;
  TGTextEntry         *fmaxADC;
  TGTextBuffer        *maxadc;
  
  GETDecoder          *decoder=nullptr;
  Int_t               numEv = 0;
  TH1D                *h;
  TCanvas             *fCanvas;
  
  TGCheckButton       *fDebug;
  Bool_t              fPrint=kFALSE;

public:
  GETmePlotsClass(const TGWindow *p,UInt_t w,UInt_t h);
  virtual ~GETmePlotsClass();

  virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t);
  
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
  fMenuFile->AddEntry("&Open...", M_FILE_OPEN);
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
  // Create a horizontal frame widget with buttons for COBO, ASAD, AGET, and CHN
  //////////////////////////////////////////////////////////////////////////////////

  TGHorizontalFrame *hframe = new TGHorizontalFrame(this,200,40);

  // Create COBO menu
  const char *title = "COBO";  
  TGLabel *label = new TGLabel(hframe, title);
  hframe->AddFrame(label, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 10, 10, 10, 10));
  
  fCOBObox = new TGComboBox(hframe, 801);
  for (Int_t i = 0; i < NCOBO; i++) {
    sprintf(tmpCOBO, "COBO %i", i);
    fCOBObox->AddEntry(tmpCOBO, i+1);
  }
  hframe->AddFrame(fCOBObox, new TGLayoutHints(kLHintsCenterX|kLHintsCenterY, 5, 10, 10, 10));
  fCOBObox->Resize(100, 20);  

  // Create ASAD menu
  const char *title2 = "ASAD";  
  TGLabel *label2 = new TGLabel(hframe, title2);
  hframe->AddFrame(label2, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 10, 10, 10, 10));
  
  fASADbox = new TGComboBox(hframe, 802);
  for (Int_t i = 0; i < NASAD; i++) {
    sprintf(tmpASAD, "ASAD %i", i);
    fASADbox->AddEntry(tmpASAD, i+1);
  }
  hframe->AddFrame(fASADbox, new TGLayoutHints(kLHintsCenterX|kLHintsCenterY, 5, 10, 10, 10));
  fASADbox->Resize(100, 20);

  AddFrame(hframe);

  // Binding calls
  fCOBObox->Associate(this);
  fASADbox->Associate(this);  
  
  TGHorizontalFrame *hframe2 = new TGHorizontalFrame(this,200,40);

  // Create AGET menu
  const char *title3 = "AGET";  
  TGLabel *label3 = new TGLabel(hframe2, title3);
  hframe2->AddFrame(label3, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 10, 10, 10, 10));
  
  fAGETbox = new TGComboBox(hframe2, 803);
  for (Int_t i = 0; i < NAGET; i++) {
    sprintf(tmpAGET, "AGET %i", i);
    fAGETbox->AddEntry(tmpAGET, i+1);
  }
  hframe2->AddFrame(fAGETbox, new TGLayoutHints(kLHintsCenterX|kLHintsCenterY, 5, 10, 10, 10));
  fAGETbox->Resize(100, 20);  

  // Create CHN menu
  const char *title4 = "Channel";  
  TGLabel *label4 = new TGLabel(hframe2, title4);
  hframe2->AddFrame(label4, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 10, 10, 10, 10));
  
  fCHNbox = new TGComboBox(hframe2, 804);
  for (Int_t i = 0; i < NCHN; i++) {
    sprintf(tmpCHN, "Channel %i", i);
    fCHNbox->AddEntry(tmpCHN, i+1);
  }
  hframe2->AddFrame(fCHNbox, new TGLayoutHints(kLHintsCenterX|kLHintsCenterY, 5, 10, 10, 10));
  fCHNbox->Resize(100, 20);

  AddFrame(hframe2);

  // Binding calls
  fAGETbox->Associate(this);
  fCHNbox->Associate(this);  
  
  TGHorizontalFrame *hframe3 = new TGHorizontalFrame(this,200,40);

  // Create time buckets option selection
  const char *title5 = "Time buckets";  
  TGLabel *label5 = new TGLabel(hframe3, title5);
  hframe3->AddFrame(label5, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 10, 10, 10, 10));
  
  fTBbox = new TGComboBox(hframe3, 805);
  for (Int_t i = 0; i < NTBS; i++) {
    auto val = (i+1)*256;
    sprintf(tmpTB, "%i", val);
    fTBbox->AddEntry(tmpTB, i+1);
  }
  hframe3->AddFrame(fTBbox, new TGLayoutHints(kLHintsCenterX|kLHintsCenterY, 5, 10, 10, 10));
  fTBbox->Resize(100, 20);

  // Create max ADC selection
  const char *title6 = "Max ADC channel";
  TGLabel *label6 = new TGLabel(hframe3, title6);
  hframe3->AddFrame(label6, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 10, 10, 10, 10));
  fmaxADC = new TGTextEntry(hframe3, maxadc = new TGTextBuffer(4), 806);
  maxadc->AddText(0,"4095");
  hframe3->AddFrame(fmaxADC, new TGLayoutHints(kLHintsCenterX|kLHintsCenterY, 5, 10, 10, 10));
  fmaxADC->Resize(100, 20);  
  
  AddFrame(hframe3);

  // Binding calls
  fTBbox->Associate(this);
  fmaxADC->Associate(this);
  
  ////////////////////////////////////////////////////////////////////////////
  // Create a horizontal frame widget with buttons
  ////////////////////////////////////////////////////////////////////////////

  TGHorizontalFrame *hframe4 = new TGHorizontalFrame(this,200,40);

  // Next button to skip to the next event
  TGTextButton *next = new TGTextButton(hframe4,"&Next", 901);
  hframe4->AddFrame(next, new TGLayoutHints(kLHintsNormal, 5,5,3,4));

  // Reset button
  TGTextButton *reset = new TGTextButton(hframe4,"&Reset", 902);
  hframe4->AddFrame(reset, new TGLayoutHints(kLHintsNormal, 5,5,3,4));

  // Update button
  TGTextButton *update = new TGTextButton(hframe4,"&Update", 903);
  hframe4->AddFrame(update, new TGLayoutHints(kLHintsNormal, 5,5,3,4));    

  // Exit button  
  TGTextButton *exit = new TGTextButton(hframe4,"&Exit", 904);
  hframe4->AddFrame(exit, new TGLayoutHints(kLHintsNormal, 5,5,3,4));

  // Check button for debug
  fDebug = new TGCheckButton(hframe4, new TGHotString("Enable/Disable\n     Debug"), 905);
  hframe4->AddFrame(fDebug, new TGLayoutHints(kLHintsNormal));
  
  AddFrame(hframe4, new TGLayoutHints(kLHintsNormal, 2,2,2,2));  

  // Binding calls
  next->Associate(this);
  reset->Associate(this);
  update->Associate(this);
  exit->Associate(this);    
  fDebug->Associate(this);
  
  ////////////////////////////////////////////////////////////////////////////  
  // Create canvas widget
  ////////////////////////////////////////////////////////////////////////////
  
  fEcanvas = new TRootEmbeddedCanvas("Ecanvas",this,200,200);
  AddFrame(fEcanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10,10,10,10));
  
  // Set a name to the main frame
  SetWindowName("GETmePlots");

  // Map all subwindows of main frame
  MapSubwindows();

  // Initialize the layout algorithm
  Resize();

  // Map main frame
  MapWindow();
}

Bool_t GETmePlotsClass::ProcessMessage(Long_t msg, Long_t parm1, Long_t)
{
  char str[4];
  Int_t coboID;
  Int_t asadID;
  GETBasicFrame* frame;
  Int_t *rawadc;
  TString hname;
  TString title;
  
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
      }
    default:
      break;
    }
      
  case kC_COMMAND:
    switch (GET_SUBMSG(msg)) {

    case kCM_CHECKBUTTON:
      switch (parm1) {
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
	std::cout << "Selected channel " << fCHN << std::endl;
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
	if (filename == "")
	  std::cout << "Please open a file...(File->Open). Try again!" << std::endl;
	// Create GETDecoder only the first time
	if (!decoder){
	  decoder = new GETDecoder(filename);
	  std::cout <<"== [GETDecoder] diagnostics macro for COBO " << fCOBO << " ASAD: " << fASAD << " AGET: " << fAGET << " CHANNEL: " << fCHN << std::endl;
	}
	numEv++;
	// Get basic frame
	frame = decoder -> GetBasicFrame();
	decoder->SetNumTbs(fTbs);
	
	if (fPrint)
	  frame->Print();
	
	if (frame == nullptr){
	  std::cout << "== [GETDecoder] End of file" << std::endl;
	  gApplication->Terminate(0);
	}
	
	coboID = frame -> GetCoboID();
	asadID = frame -> GetAsadID();

	// Data sanity check
	if (coboID != fCOBO){
	  std::cout << "Selected COBO doesn't exist in data!" << std::endl;
	  gApplication->Terminate(0);
	}
	if (asadID != fASAD){
	  std::cout << "Selected ASAD doesn't exist in data!" << std::endl;
	  gApplication->Terminate(0);
	}
	// Access data and fill the histogram
	rawadc = frame -> GetSample(fAGET, fCHN);
	hname = Form("histogram_%d", numEv);
	h = new TH1D(hname,";Time buckets;ADC channel",fTbs,0,fTbs);
	for (Int_t iTb = 0; iTb < fTbs; iTb++)
	  h->Fill(iTb, rawadc[iTb]);
	h->SetStats(0);
	h->SetMaximum(fADC);
	title = Form("Cobo %d AsAd %d Aget %d Channel %d", fCOBO, fASAD, fAGET, fCHN);
	h->SetTitle(title);
	h->GetXaxis()->SetTitle("Time buckets");
	h->GetYaxis()->SetTitle("ADC channels");
	h->Draw("hist");
	
	fCanvas = fEcanvas->GetCanvas();
	fCanvas->cd();
	fCanvas->Update();
	break;
      case 902:
	decoder = nullptr;
	decoder = new GETDecoder(filename);
	break;
      case 903:
	h->Reset("ICESM");
	fCanvas->cd();
	fCanvas->Update();
	break;	
      case 904:
	gApplication->Terminate(0);
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
