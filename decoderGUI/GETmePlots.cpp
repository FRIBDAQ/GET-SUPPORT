#include <QtWidgets>

#include <iostream>

#include <TCanvas.h>
#include <TVirtualX.h>
#include <TSystem.h>
#include <TFormula.h>
#include <TF1.h>
#include <TH1.h>
#include <TFrame.h>
#include <TTimer.h>

#include "GETmePlots.h"

Int_t    fCOBO = 0;
Int_t    fASAD = 0;
Int_t    fAGET = 0;
Int_t    fCHN = 0;
Int_t    fTbs = 256;
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

  setMinimumSize(1200, 400);
  setWindowTitle(tr("GETmePlots"));
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
  connect(fCHNbox, SIGNAL(currentIndexChanged(int)), this, SLOT(CHNindexChanged()));
  // TB box
  connect(fTBbox, SIGNAL(currentIndexChanged(int)), this, SLOT(TBindexChanged()));     
  // Max ADC
  connect(fADCvalue, SIGNAL(textChanged(const QString &)), this, SLOT(ADCvalueChanged(const QString &)));
  // Zoom button
  connect(zoom, SIGNAL(clicked()), tab[ASADtab][AGETtab], SLOT(Zoom())); 
  // Tab identifier
  connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(TabSelected()));
  // Online option identifier
  connect(fOnlineCheck, SIGNAL(toggled(bool)), fOnlinePath, SLOT(setEnabled(bool)));
  connect(fOnlineCheck, SIGNAL(toggled(bool)), buttons[0], SLOT(setEnabled(bool)));  
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
GETmePlots::CHNindexChanged()
{
  QString text = fCHNbox->currentText();
  fCHN = text.split(" ")[1].toInt();
  if (fDebug)
    std::cout << "You have selected channel " << fCHN << std::endl;  
}

void
GETmePlots::TBindexChanged()
{
  QString text = fTBbox->currentText();
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
  canvas[asad][aget]->getCanvas(asad,aget)->Divide(9,8);
  for (Int_t i=0; i<NCHN; i++){
    z++;
    canvas[asad][aget]->getCanvas(asad,aget)->cd(z);
    if (asad == 0)
      tmpTitle = Form("AsAd %d Aget %d Channel %d Event %d", asad, aget, i, (numEv+1)/2);
    else 
      tmpTitle = Form("AsAd %d Aget %d Channel %d Event %d", asad, aget, i, (numEv/2));
    h[asad][aget][i]->SetBins(fTbs, 0, fTbs);
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
  fCHNbox = new QComboBox(new QLabel(tr("Channel")));
  for(int i = 0; i < 68; i++)
    fCHNbox->addItem("Channel " + QString::number(i));
  fCHNbox->setCurrentIndex(fCHN);
  lo->addWidget(fCHNbox, 0, 1);  

  // Zoom button
  zoom = new QPushButton(tr("Zoom"));
  lo->addWidget(zoom, 1, 1);

  // Time bucket selection
  fTbsLabel = new QLabel(tr("Time buckets"));    
  fTBbox = new QComboBox(new QLabel(tr("")));
  for(int i = 1; i < 3; i++)
    fTBbox->addItem(QString::number(i*256));
  fTBbox->setCurrentIndex(0);
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
  l->addWidget(canvas[asad][aget]);      
  
}

QGCanvas::QGCanvas(int asad, int aget, QWidget *parent)
  : QWidget(parent, 0)
{
  // QRootCanvas constructor.

  // set options needed to properly update the canvas when resizing the widget
  // and to properly handle context menus and momoyuse move events
  //  setAttribute(Qt::WA_OpaquePaintEvent, true);
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
  if (fDebug)
    std::cout << "Update clicked" << std::endl;

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
  updateCanvas();
}

void
GETmePlots::Snapshot()
{
  fSnapshot = true;
  numEv = 0;
  
  if (fileName == ""){
    std::cout << "\t\t\t\t >>> Please open a file...(File->Open file) or attach an online ring buffer. Try again! <<<" << std::endl;
    return;
  }
  
  if (!decoder || (fileName != fileName_open)){
    if (fDebug)
      std::cout << "Inside GETmePlots::Snapshot !decoder" << std::endl;
    decoder = GETDecoder::getInstance();
    fileName = decoder->SetDataSink(fileName, "file:///tmp/snapshot.evt", 100);
    
    fileName_open = fileName;
  }
  std::cout << "Snapshot of 100 ring items from " << fileName << " ready to be analyzed!" << std::endl;
  
}

void
GETmePlots::Next()
{
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

  // reset histograms
  for (Int_t k=0; k<NASAD; k++)
    for (Int_t j=0; j<NAGET; j++)
      for (Int_t i=0; i<NCHN; i++)
	h[k][j][i]->Reset("ICESM");  
  
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
      if (tb < fTbs) {
	if (fDebug)
	  std::cout << "\t\t\t" << chnID << " " << tb << " " << adc << std::endl;
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
  QWidget* w = new QWidget;
  
  w->show();
  // QMainCanvas constructor.
  QVBoxLayout *l = new QVBoxLayout(w);
  // the QRootCanvas indices don't mean anything in this case
  l->addWidget(ctmp = new QGCanvas(fASAD, fAGET, w));

  ctmp->getCanvas(fASAD,fAGET)->cd();
  htmp = (TH1D*)h[fASAD][fAGET][fCHN]->Clone();
  htmp->SetTitle(Form("AsAd %d Aget %d Channel %d Event %d", fASAD, fAGET, fCHN, 0));
  htmp->SetBins(fTbs, 0, fTbs);
  htmp->SetMaximum(fADC);
  htmp->Draw("histo");
  ctmp->getCanvas(fASAD,fAGET)->Modified();
  ctmp->getCanvas(fASAD,fAGET)->Update();
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
}
