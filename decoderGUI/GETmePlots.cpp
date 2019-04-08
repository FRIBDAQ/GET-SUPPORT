#include <QtWidgets>

#include <iostream>

#include "TROOT.h"
#include "TRandom.h"
#include <TCanvas.h>
#include <TVirtualX.h>
#include <TSystem.h>
#include <TFormula.h>
#include <TF1.h>
#include <TH1.h>
#include <TH2.h>
#include <TFrame.h>
#include <TTimer.h>

#include "GETmePlots.h"

Int_t    fCOBO = 0;
Int_t    fASAD = 0;
Int_t    fAGET = 0;
Int_t    fCHN = 0;
Int_t    fTbs = 512;
Int_t    fADC = 4095;
TString  tmpTitle = "";
Int_t    numEv = 0;
std::string fileName = "";
std::string fileName_open = "";
Int_t    winCounter = 0;
Int_t     winIter = 0;
Int_t    ASADtab;
Int_t    AGETtab;
Int_t    oldASADtab = -1;
Int_t    oldAGETtab = -1;
TString  h_name[NASAD][NAGET][NCHN];
TH1D     *h[NASAD][NAGET][NCHN];
QGCanvas *canvas[NASAD][NAGET];
TCanvas  *fCanvas[NASAD][NAGET];
QRootCanvas *ctmp;
TCanvas  *canvastmp;
QPushButton *zoom;
QWidget* w;

Int_t           coboID = 0;
Int_t           asadID = 0;
Int_t           agetID = 0;
Int_t           chnID = 0;
std::vector<NSCLGET::Hit> frame;

Bool_t fDebug = false;
Bool_t fDone = false;
Bool_t tabTable[8];
Bool_t fSnapshot = false;

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_int_distribution<int> dist('a', 'z');

GETmePlots::GETmePlots()
{
  createActions();
  createMenu();
  createTopGroupBox();
  createBottomGroupBox();
  createTabs();
  createStatusBar();
  
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->setMenuBar(menuBar);
  mainLayout->addWidget(topGroupBox);
  mainLayout->addWidget(bottomGroupBox);  
  mainLayout->addWidget(tabWidget);
  mainLayout->addWidget(statusBar);
  setLayout(mainLayout);
  
  createConnections();
  DisableAll();

  setWindowFlags(Qt::WindowStaysOnTopHint);
  setObjectName("main");
  setMinimumSize(1200, 400);
  setWindowTitle(tr("GETmePlots"));

  installEventFilter(this);
  gInterpreter->Declare("#include \"/scratch/cerizza/GET.master/decoderGUI/ZoomClass.h\"");

}

GETmePlots::~GETmePlots()
{
}

void
GETmePlots::createActions()
{
  openAct = new QAction(tr("&Open file..."), this);
  openAct->setStatusTip(tr("Open an existing file"));
  connect(openAct, &QAction::triggered, this, &GETmePlots::openFile);

  exitAct = new QAction(tr("E&xit"), this);
  exitAct->setStatusTip(tr("Exit the application"));
  connect(exitAct, &QAction::triggered, this, &QWidget::close);

  createHistograms();
}

void
GETmePlots::createHistograms()
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
GETmePlots::createConnections()
{
  // snapshot button
  connect(buttons[0], SIGNAL(clicked()), this, SLOT(Snapshot()));  
  // next button
  connect(buttons[1], SIGNAL(clicked()), this, SLOT(Next()));
  connect(buttons[1], SIGNAL(clicked()), this, SLOT(Draw()));
  connect(buttons[1], SIGNAL(clicked()), tab[ASADtab][AGETtab], SLOT(Update()));    
  // update button
  connect(buttons[2], SIGNAL(clicked()), tab[ASADtab][AGETtab], SLOT(Update()));
  // reset button
  connect(buttons[3], SIGNAL(clicked()), tab[ASADtab][AGETtab], SLOT(Reset()));
  // exit button
  connect(buttons[4], SIGNAL(clicked()), this, SLOT(close()));
  // COBO box
  connect(fCOBObox, SIGNAL(currentIndexChanged(int)), this, SLOT(COBOindexChanged())); 
  // ASAD box
  connect(fASADbox, SIGNAL(currentIndexChanged(int)), this, SLOT(ASADindexChanged()));   
  // AGET box
  connect(fAGETbox, SIGNAL(currentIndexChanged(int)), this, SLOT(AGETindexChanged()));   
  // CHN box
  connect(fCHNbox, SIGNAL(textChanged(const QString &)), this, SLOT(CHNvalueChanged(const QString &)));
  // TB box
  connect(fTBbox, SIGNAL(textChanged(const QString &)), this, SLOT(TBvalueChanged(const QString &)));     
  // Max ADC
  connect(fADCvalue, SIGNAL(textChanged(const QString &)), this, SLOT(ADCvalueChanged(const QString &)));
  // Zoom button
  connect(zoom, SIGNAL(clicked()), tab[ASADtab][AGETtab], SLOT(Zoom())); 
  // Tab identifier
  connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(TabSelected()));
  // Online option identifier
  connect(fOnlineCheck, SIGNAL(toggled(bool)), fOnlinePath, SLOT(setEnabled(bool)));
  connect(fOnlineCheck, SIGNAL(toggled(bool)), buttons[0], SLOT(setEnabled(bool)));
  connect(fOnlineCheck, SIGNAL(toggled(bool)), this, SLOT(Toggled()));  
  // Online ringbuffer
  connect(fOnlinePath, SIGNAL(textChanged(const QString &)), this, SLOT(OnlineSelected(const QString &)));  

}

void
GETmePlots::EnableAll()
{
  for (int i=0; i<4; i++)
    buttons[i]->setEnabled(kTRUE);
  fCOBObox->setEnabled(kTRUE);
  fCHNbox->setEnabled(kTRUE);
  fTBbox->setEnabled(kTRUE);
  fADCvalue->setEnabled(kTRUE);

  zoom->setEnabled(kTRUE);
  
}

void
GETmePlots::DisableAll()
{
  for (int i=0; i<4; i++)
    buttons[i]->setEnabled(kFALSE);
  fCOBObox->setEnabled(kFALSE);
  fASADbox->setEnabled(kFALSE);
  fAGETbox->setEnabled(kFALSE);
  fCHNbox->setEnabled(kFALSE);
  fTBbox->setEnabled(kFALSE);
  fADCvalue->setEnabled(kFALSE);

  DisableTabs();
  
  zoom->setEnabled(kFALSE);
  fOnlinePath->setEnabled(kFALSE);
  
}

void
GETmePlots::DisableTabs()
{
  for(int i=1; i<8; i++){
    tabWidget->setTabEnabled(i, tabTable[i]);
    if (fDebug)
      std::cout << "ASADtab: " << ASADtab << " and AGETtab: " << AGETtab << " with index " << i << " is set to " << tabTable[i] << std::endl;
  }
}

void
GETmePlots::Toggled()
{
  if (fOnlineCheck->isChecked()){
    QString msg = fOnlinePath->text();
    fileName = msg.toUtf8().constData();
    fileName = "tcp://"+fileName;
    
    std::cout << fileName << std::endl;
  }
}

void
GETmePlots::COBOindexChanged()
{
  QString text = fCOBObox->currentText();
  fCOBO = text.split(" ")[1].toInt();
  if (fDebug)
    std::cout << "You have selected COBO " << fCOBO << std::endl;
}

void
GETmePlots::ASADindexChanged()
{
  QString text = fASADbox->currentText();
  fASAD = text.split(" ")[1].toInt();
  if (fDebug)
    std::cout << "You have selected ASAD " << fASAD << std::endl;  
}

void
GETmePlots::AGETindexChanged()
{
  QString text = fAGETbox->currentText();
  fAGET = text.split(" ")[1].toInt();
  if (fDebug)
    std::cout << "You have selected AGET " << fAGET << std::endl;  
}

void
GETmePlots::CHNvalueChanged(const QString & text)
{
  fCHN = text.split(" ")[0].toInt();
  if (fDebug)
    std::cout << "You have selected channel " << fCHN << std::endl;  
}

void
GETmePlots::TBvalueChanged(const QString & text)
{
  fTbs = text.split(" ")[0].toInt();
  if (fDebug)
    std::cout << "You have selected " << fTbs << " time buckets" << std::endl;  
}

void
GETmePlots::ADCvalueChanged(const QString & text)
{
  fADC = text.split(" ")[0].toInt();
}

void
GETmePlots::TabSelected()
{
  QString currentTabText = tabWidget->tabText(tabWidget->currentIndex());
  ASADtab = currentTabText.split(" ")[1].toInt();
  AGETtab = currentTabText.split(" ")[3].toInt();  

  fASADbox->setCurrentIndex(ASADtab);
  fAGETbox->setCurrentIndex(AGETtab);  
}
 
void
GETmePlots::OnlineSelected(const QString & text)
{
  fileName = text.toUtf8().constData();
  fileName = "tcp://"+fileName;
  
  std::cout << fileName << std::endl;
  if (fDebug)
    fileName = "file:///scratch/cerizza/GET-SUPPORT/decoderGUIv3/run-0050-00.evt";
  if (fileName != "")
    EnableAll();
}
 
void
FillTheTab::DrawHistograms(int asad, int aget)
{
  Int_t z = 0;
  // Create histograms
  for (Int_t i=0; i<NCHN; i++){
    z++;
    canvas[asad][aget]->getCanvas(asad,aget)->cd(z);
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
  fDone = true;
}

void
FillTheTab::UpdateHistograms(int asad, int aget)
{
  Int_t z = 0;
  for (Int_t i=0; i<NCHN; i++){
    z++;
    canvas[asad][aget]->getCanvas(asad,aget)->cd(z);
    h[asad][aget][i]->SetBins(fTbs, 0, fTbs);
    h[asad][aget][i]->SetMinimum(0);
    h[asad][aget][i]->SetMaximum(fADC);
    h[asad][aget][i]->Draw("histo");
  }
}

void
GETmePlots::createMenu()
{
  menuBar = new QMenuBar;

  fileMenu = new QMenu(tr("&File"), this);
  fileMenu->addAction(openAct);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAct);
  menuBar->addMenu(fileMenu);
}

void
GETmePlots::createStatusBar(){
  statusBar = new QStatusBar;
  statusBar->showMessage(tr(""));
}

void
GETmePlots::SetStatusBar(std::string msg)
{
  statusBar->showMessage(tr(msg.c_str()));
}

void
GETmePlots::openFile()
{
  fileN = QFileDialog::getOpenFileName(this,
				       tr("Open file"), "",
				       tr("EVT files (*.evt);;All Files (*)"));
  
  fileName = fileN.toUtf8().constData();
  fileName = "file://" + fileName;  
  
  if (fileN.isEmpty()){
    std::cout << "\t\t\t\t >>> Please open a file...(File->Open file). Try again! <<<" << std::endl;
    return;
  } else {
    SetStatusBar(fileName);
    EnableAll();
  }
}

void
GETmePlots::createTopGroupBox()
{
  topGroupBox = new QGroupBox(tr(""));
  QHBoxLayout *layout = new QHBoxLayout;

  // COBO selection
  fCOBObox = new QComboBox(new QLabel(tr("COBO")));
  for(int i = 0; i < 2; i++)
    fCOBObox->addItem("COBO " + QString::number(i));
  fCOBObox->setCurrentIndex(fCOBO);
  layout->addWidget(fCOBObox);
  
  // Button definition
  const char* v[5] = {"Snap","Next","Update","Reset","Exit"};
  for (int i = 0; i < 5; ++i) {
    buttons[i] = new QPushButton(tr(v[i]));
    layout->addWidget(buttons[i]);
  }
  
  // Online + selection button
  gridBox = new QGroupBox;
  QGridLayout *lo = new QGridLayout;
  fOnlineLabel = new QLabel(tr("tcp://")); 
  fOnlinePath = new QLineEdit;
  fOnlinePath->setText(tr("spdaq08/GET"));
  lo->addWidget(fOnlineLabel, 0, 0);
  lo->addWidget(fOnlinePath, 0, 1);  
  fOnlineCheck = new QCheckBox(tr("&Enable/Disable Online"));
  fOnlineCheck->setChecked(false);
  lo->addWidget(fOnlineCheck, 0, 2);
  gridBox->setLayout(lo);
  layout->addWidget(gridBox);
  
  topGroupBox->setLayout(layout);
}

void
GETmePlots::createBottomGroupBox()
{
  bottomGroupBox = new QGroupBox(tr(""));

  gridBox2 = new QGroupBox;
  QGridLayout *lo = new QGridLayout;
  // ASAD selection
  fASADbox = new QComboBox(new QLabel(tr("ASAD")));
  for(int i = 0; i < 2; i++)
    fASADbox->addItem("ASAD " + QString::number(i));
  fASADbox->setCurrentIndex(fASAD);
  lo->addWidget(fASADbox, 0, 0);
  
  // AGET selection
  fAGETbox = new QComboBox(new QLabel(tr("AGET")));
  for(int i = 0; i < 4; i++)
    fAGETbox->addItem("AGET " + QString::number(i));
  fAGETbox->setCurrentIndex(fAGET);
  lo->addWidget(fAGETbox, 1, 0);  
  
  // Channel selection
  fChnLabel = new QLabel(tr("Channel"));    
  fCHNbox = new QLineEdit;
  fCHNbox->setText(QString::number(fCHN));
  fCHNbox->setMaximumWidth(40);
  lo->addWidget(fChnLabel,0,1);
  lo->addWidget(fCHNbox,0,2);
  
  // Zoom button
  zoom = new QPushButton(tr("Zoom"));
  lo->addWidget(zoom, 1, 1);

  // Time bucket selection
  fTbsLabel = new QLabel(tr("Time buckets"));    
  fTBbox = new QLineEdit;
  fTBbox->setText(QString::number(fTbs));
  lo->addWidget(fTbsLabel, 0, 3);  
  lo->addWidget(fTBbox, 0, 4);  

  // Max ADC value
  fADCLabel = new QLabel(tr("Max ADC channel"));    
  fADCvalue = new QLineEdit;
  fADCvalue->setText(QString::number(fADC));
  lo->addWidget(fADCLabel, 1, 3);  
  lo->addWidget(fADCvalue, 1, 4);  
  
  bottomGroupBox->setLayout(lo);
}


void
GETmePlots::createTabs()
{
  tabWidget = new QTabWidget;
  std::string v1[NASAD] = {"ASAD 0 ","ASAD 1 "};
  std::string v2[NAGET] = {"AGET 0", "AGET 1", "AGET 2", "AGET 3"};
  for (int asad=0; asad<NASAD; asad++){
    for (int aget=0; aget<NAGET; aget++){
      tab[asad][aget] = new FillTheTab(asad, aget);
      tabWidget->addTab(tab[asad][aget], tr((v1[asad]+v2[aget]).c_str()));
      tabTable[asad*aget+aget]=false;
    }
  }
}

FillTheTab::FillTheTab(int asad, int aget, QWidget *parent)
  : QWidget(parent)
{
  // QMainCanvas constructor.
  QVBoxLayout *l = new QVBoxLayout(this);
  canvas[asad][aget] = new QGCanvas(asad, aget, this);
  canvas[asad][aget]->getCanvas(asad,aget)->Divide(9,8);
  l->addWidget(canvas[asad][aget]);      
}

QGCanvas::QGCanvas(int asad, int aget, QWidget *parent)
  : QWidget(parent, 0)
{
  // QRootCanvas constructor.

  // set options needed to properly update the canvas when resizing the widget
  // and to properly handle context menus and momoyuse move events
  setMinimumSize(300, 200);
  setUpdatesEnabled(kFALSE);
  setMouseTracking(kTRUE);

  int wid[NASAD][NAGET];
  // register the QWidget in TVirtualX, giving its native window id
  wid[asad][aget] = gVirtualX->AddWindow((ULong_t)winId(), 600, 400);
  // create the ROOT TCanvas, giving as argument the QWidget registered id
  fCanvas[asad][aget] = new TCanvas(Form("canvas_%d_%d", asad, aget), width(), height(), wid[asad][aget]);   
}

void QGCanvas::resizeEvent( QResizeEvent * )
{
  // Handle resize events
  if (fCanvas[ASADtab][AGETtab]) {
    fCanvas[ASADtab][AGETtab]->Resize();
    fCanvas[ASADtab][AGETtab]->Update();
  }
}

void QGCanvas::paintEvent( QPaintEvent * )
{
  // Handle paint events.
  if (fCanvas[ASADtab][AGETtab]) {
    fCanvas[ASADtab][AGETtab]->Resize();
    fCanvas[ASADtab][AGETtab]->Update();
  }
}

void QGCanvas::mouseMoveEvent(QMouseEvent *e)
{
  // Handle mouse move events.

  if (fCanvas[ASADtab][AGETtab]) {
    if (e->buttons() & Qt::LeftButton) {
      fCanvas[ASADtab][AGETtab]->HandleInput(kButton1Motion, e->x(), e->y());
      //    }
      //    else if (e->buttons() & Qt::MidButton) {
      //      fCanvas[ASADtab][AGETtab]->HandleInput(kButton2Motion, e->x(), e->y());
      //    } else if (e->buttons() & Qt::RightButton) {
      //      fCanvas[ASADtab][AGETtab]->HandleInput(kButton3Motion, e->x(), e->y());
    } else {
      fCanvas[ASADtab][AGETtab]->HandleInput(kMouseMotion, e->x(), e->y());
    }
  }
}

void QGCanvas::mousePressEvent(QMouseEvent *e)
{
  // Handle mouse button press events.

  if (fCanvas[ASADtab][AGETtab]) {
    switch (e->button()) {
    case Qt::LeftButton :
      fCanvas[ASADtab][AGETtab]->HandleInput(kButton1Down, e->x(), e->y());
      break;
      //    case Qt::MidButton :
      //      fCanvas[ASADtab][AGETtab]->HandleInput(kButton2Down, e->x(), e->y());
      //      break;
      //    case Qt::RightButton :
      // does not work properly on Linux...
      // ...adding setAttribute(Qt::WA_PaintOnScreen, true)
      // seems to cure the problem
      //      fCanvas[ASADtab][AGETtab]->HandleInput(kButton3Down, e->x(), e->y());
      //      break;
    default:
      break;
    }
  }
}

void QGCanvas::mouseReleaseEvent(QMouseEvent *e)
{
  // Handle mouse button release events.

  if (fCanvas[ASADtab][AGETtab]) {
    switch (e->button()) {
    case Qt::LeftButton :
      fCanvas[ASADtab][AGETtab]->HandleInput(kButton1Up, e->x(), e->y());
      break;
      //    case Qt::MidButton :
      //      fCanvas[ASADtab][AGETtab]->HandleInput(kButton2Up, e->x(), e->y());
      //      break;
      //    case Qt::RightButton :
      // does not work properly on Linux...
      // ...adding setAttribute(Qt::WA_PaintOnScreen, true)
      // seems to cure the problem
      //      fCanvas[ASADtab][AGETtab]->HandleInput(kButton3Up, e->x(), e->y());
      //      break;
    default:
      break;
    }
  }
}


void
FillTheTab::Reset()
{
  resetHistograms();
  updateCanvas();
}

void
FillTheTab::Update()
{
  if (fDebug){
    std::cout << "Update clicked" << std::endl;
    std::cout << "COBO:   " << fCOBO << std::endl;
    std::cout << "ASAD:   " << fASAD << std::endl;
    std::cout << "AGET:   " << fAGET << std::endl;
    std::cout << "CHN:    " << fCHN  << std::endl;
    std::cout << "TB:     " << fTbs  << std::endl;
    std::cout << "maxADC: " << fADC  << std::endl;
    std::cout << "Source: " << fileName << std::endl;
  }  

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
  updateCanvas();
}

void
FillTheTab::Zoom()
{
  zoomHistograms();
  //  updateCanvas();
}

void
GETmePlots::Snapshot()
{
  fSnapshot = true;
  numEv = 0;

  /*
  QString msg = fOnlinePath->text();
  fileName = msg.toUtf8().constData();
  fileName = "tcp://"+fileName;
  */

  std::cout << fileName << std::endl;
  
  if (fileName == ""){
    std::cout << "\t\t\t\t >>> Please open a file...(File->Open file) or attach an online ring buffer. Try again! <<<" << std::endl;
    return;
  }

  if (fileName != "")
    EnableAll();
  
  if (!decoder || (fileName != fileName_open)){
    if (fDebug)
      std::cout << "Inside GETmePlots::Snapshot !decoder" << std::endl;
    decoder = GETDecoder::getInstance();
    fileName = decoder->SetDataSink(fileName, "file:///tmp/snapshot.evt", 100);    
    
    fileName_open = "ringbuffer";
  }
  std::cout << "Snapshot of 100 ring items from " << fileName << " ready to be analyzed!" << std::endl;
}

void
GETmePlots::Next()
{
  fDone = false;

  
  if (fileName == ""){
    std::cout << "\t\t\t\t >>> Please open a file...(File->Open file) or attach an online ring buffer. Try again! <<<" << std::endl;
    return;
  }

  if (!fSnapshot){
    // Create GETDecoder only the first time
    if (!decoder || (fileName != fileName_open)){
      if (fDebug)
	std::cout << "Inside GETmePlots !decoder" << std::endl;
      decoder = GETDecoder::getInstance();
      decoder->SetUrls(fileName);
      
      fileName_open = fileName;
    }
  }
}

void
GETmePlots::Draw()
{
  if (fDebug)
    std::cout << "Filling histograms..." << std::endl;

  std::cout << "Event " << numEv << std::endl;  
  numEv++;

  frame = decoder->GetFrame();  
  
  // Enable/disable tabs
  if (fDebug)
    std::cout << "Inside fillHistograms - before frame loop" << std::endl;
  
  for (int i=0; i<8; i++){
    tabTable[i] = false;
    if (fDebug)
      std::cout << tabTable[i];
  }
  if (fDebug)
    std::cout << std::endl;

  // Reset all histograms
  for (Int_t j=0; j<NAGET; j++){
    for (Int_t i=0; i<NCHN; i++){
      h[ASADtab][j][i]->Reset("ICESM");
      for (Int_t k=0; k<fTbs; k++)
	h[ASADtab][j][i]->Fill(k,-1);
    }
  }
  
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
  
  if (asadID == 0)
    tabWidget->setCurrentIndex(0);
  else
    tabWidget->setCurrentIndex(4);    
  for (int i=0; i<8; i++)
    tabWidget->setTabEnabled(i, tabTable[i]);
}

void
FillTheTab::resetHistograms()
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
FillTheTab::zoomHistograms()
{
  w = new QWidget;
  w->show();
  QVBoxLayout *l = new QVBoxLayout();

  //  QPushButton *closeButton, *b;

  //  closeButton = new QPushButton(tr("Close"));
  l->addWidget(ctmp = new QRootCanvas(w));  
  //  l->addWidget(b = new QPushButton("Draw Histogram", w));
  //  l->addWidget(closeButton);

  //  connect(closeButton, SIGNAL(clicked()), w, SLOT(close()));
  //  connect(b, SIGNAL(clicked()), this, SLOT(clicked1()));

  clicked1();

  // Generate random name
  std::string wName = std::to_string(fASAD)+"_"+std::to_string(fAGET)+"_"+std::to_string(fCHN);
  
  w->setObjectName(QString::fromStdString(wName));
  QString objName = w->objectName();
  //  std::cout << objName.toUtf8().constData() << std::endl;
  
  fRootTimer = new QTimer(w);
  QObject::connect( fRootTimer, SIGNAL(timeout()), this, SLOT(handle_root_events()) );
  fRootTimer->start( 20 );
  
  w->setLayout(l);
  std::string title = "ASAD " + std::to_string(fASAD) + " AGET " + std::to_string(fAGET) + " CHANNEL " + std::to_string(fCHN);
  w->setWindowTitle(QString::fromStdString(title)); 

}

void FillTheTab::handle_root_events()
{
  //call the inner loop of ROOT
  gSystem->ProcessEvents();
}


TString
FillTheTab::GenerateName()
{
  std::string result;
  std::generate_n(std::back_inserter(result), 8, [&]{return dist(mt);});
  hname = (TString)result;
  return hname;
}
  
void
FillTheTab::updateCanvas()
{
  canvas[ASADtab][AGETtab]->getCanvas(ASADtab,AGETtab)->Modified();
  canvas[ASADtab][AGETtab]->getCanvas(ASADtab,AGETtab)->Update();  

  canvas[ASADtab][AGETtab]->getCanvas(ASADtab,AGETtab)->cd();
  gPad->AddExec("zoom",Form("ZoomClass::ZoomChannel(%d, %d)", fTbs, fADC));    
}

QRootCanvas::QRootCanvas(QWidget *parent) : QWidget(parent, 0), ccanvas(0)
{
  // QRootCanvas constructor.

  // set options needed to properly update the canvas when resizing the widget
  // and to properly handle context menus and mouse move events
  setAttribute(Qt::WA_OpaquePaintEvent, true);
  setMinimumSize(200, 200);
  setUpdatesEnabled(kFALSE);
  setMouseTracking(kTRUE);

  // register the QWidget in TVirtualX, giving its native window id
  int wid = gVirtualX->AddWindow((ULong_t)winId(), 200, 200);
  // create the ROOT TCanvas, giving as argument the QWidget registered id
  ccanvas = new TCanvas("Root Canvas", width(), height(), wid);
}

//______________________________________________________________________________
void QRootCanvas::resizeEvent( QResizeEvent * )
{
  // Handle resize events.

  if (ccanvas) {
    ccanvas->Resize();
    ccanvas->Update();
  }
}

//______________________________________________________________________________
void QRootCanvas::paintEvent( QPaintEvent * )
{
  // Handle paint events.

  if (ccanvas) {
    ccanvas->Resize();
    ccanvas->Update();
  }
}

void QRootCanvas::mouseMoveEvent(QMouseEvent *e)
{
  // Handle mouse move events.

  if (ccanvas) {
    if (e->buttons() & Qt::LeftButton) {
      ccanvas->HandleInput(kButton1Motion, e->x(), e->y());
    } else if (e->buttons() & Qt::MidButton) {
      ccanvas->HandleInput(kButton2Motion, e->x(), e->y());
    } else if (e->buttons() & Qt::RightButton) {
      ccanvas->HandleInput(kButton3Motion, e->x(), e->y());
    } else {
      ccanvas->HandleInput(kMouseMotion, e->x(), e->y());
    }
  }
}

void QRootCanvas::mousePressEvent( QMouseEvent *e )
{
  // Handle mouse button press events.

  if (ccanvas) {
    switch (e->button()) {
    case Qt::LeftButton :
      ccanvas->HandleInput(kButton1Down, e->x(), e->y());
      break;
    case Qt::MidButton :
      ccanvas->HandleInput(kButton2Down, e->x(), e->y());
      break;
    case Qt::RightButton :
      // does not work properly on Linux...
      // ...adding setAttribute(Qt::WA_PaintOnScreen, true)
      // seems to cure the problem
      ccanvas->HandleInput(kButton3Down, e->x(), e->y());
      break;
    default:
      break;
    }
  }
}

void QRootCanvas::mouseReleaseEvent( QMouseEvent *e )
{
  // Handle mouse button release events.

  if (ccanvas) {
    switch (e->button()) {
    case Qt::LeftButton :
      ccanvas->HandleInput(kButton1Up, e->x(), e->y());
      break;
    case Qt::MidButton :
      ccanvas->HandleInput(kButton2Up, e->x(), e->y());
      break;
    case Qt::RightButton :
      // does not work properly on Linux...
      // ...adding setAttribute(Qt::WA_PaintOnScreen, true)
      // seems to cure the problem
      ccanvas->HandleInput(kButton3Up, e->x(), e->y());
      break;
    default:
      break;
    }
  }
}

void FillTheTab::clicked1()
{
  ctmp->getCanvas()->Clear();
  ctmp->getCanvas()->cd();
  ctmp->getCanvas()->SetBorderMode(0);
  ctmp->getCanvas()->SetFillColor(0);
  ctmp->getCanvas()->SetGrid();
  htmp = (TH1D*)h[fASAD][fAGET][fCHN]->Clone();
  htmp->SetTitle(Form("AsAd %d Aget %d Channel %d Event %d", fASAD, fAGET, fCHN, numEv));
  htmp->SetBins(fTbs, 0, fTbs);
  htmp->SetMinimum(0);
  htmp->SetMaximum(fADC);
  htmp->Draw("histo");
  /*
  TH2F *hpxpy  = new TH2F("hpxpy","py vs px",40,-4,4,40,-4,4);
  hpxpy->SetStats(0);
  Double_t px,py;
  for (Int_t i = 0; i < 50000; i++) {
    gRandom->Rannor(px,py);
    hpxpy->Fill(px,py);
  }
  hpxpy->Draw("col");
  */
  ctmp->getCanvas()->Modified();
  ctmp->getCanvas()->Update();  

  //  ctmp->getCanvas()->cd();
  //  gPad->AddExec("dynamic","MyClass::MouseEvent()");
}

bool GETmePlots::eventFilter(QObject* obj, QEvent* event)
{
  if (event->type()==QEvent::KeyPress) {
    QKeyEvent* key = static_cast<QKeyEvent*>(event);
    if ( (key->key()==Qt::Key_Enter) || (key->key()==Qt::Key_Return) ) {
      //Enter or return was pressed
      tab[ASADtab][AGETtab]->Update();
    } else {
      return QObject::eventFilter(obj, event);
    }
    return true;
  } else {
    return QObject::eventFilter(obj, event);
  }
  return false;
}

void GETmePlots::closeEvent(QCloseEvent *event)
{
  foreach (QWidget * widget, QApplication::topLevelWidgets())  {
    if (widget == this)
      continue;
    widget->close();
  }
  event->accept();
}
