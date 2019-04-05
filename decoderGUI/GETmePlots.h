#ifndef GETMEPLOTS_H
#define GETMEPLOTS_H

#include <QDebug>
#include <QDialog>

#include "GETDecoder.h"
#include "AnalyzeFrame.h"

#include "TString.h"
#include "TH1D.h"

class QAction;
class QGroupBox;
class QLabel;
class QLineEdit;
class QMenu;
class QMenuBar;
class QPushButton;
class QTextEdit;
class QComboBox;
class QCheckBox;
class QTabWidget;
class QFileInfo;
class TCanvas;
class QTimer;
class QStatusBar;
class QGCanvas;
class QRootCanvas;

const Int_t              NCOBO=2;
const Int_t              NASAD=2;
const Int_t              NAGET=4;
const Int_t              NCHN=68;
const Int_t              NTBS=2;
extern Int_t             fCOBO;
extern Int_t             fASAD;
extern Int_t             fAGET;
extern Int_t             fCHN;
extern Int_t             fTbs;
extern Int_t             fADC;
extern TString           tmpTitle;
extern Int_t             numEv;
extern std::string       fileName;
extern std::string       fileName_open;
extern TString           h_name[NASAD][NAGET][NCHN];
extern TH1D              *h[NASAD][NAGET][NCHN];
extern QGCanvas          *canvas[NASAD][NAGET];      
extern TCanvas           *fCanvas[NASAD][NAGET];
extern QRootCanvas       *ctmp;
extern TCanvas           *canvastmp;
extern QPushButton *zoom;

class QRootCanvas : public QWidget
{
     Q_OBJECT

 public:
     QRootCanvas( QWidget *parent = 0);
     virtual ~QRootCanvas() {}
     TCanvas* getCanvas() { return ccanvas;}

 protected:

     TCanvas           *ccanvas;
     virtual void    mouseMoveEvent( QMouseEvent *e );
     virtual void    mousePressEvent( QMouseEvent *e );
     virtual void    mouseReleaseEvent( QMouseEvent *e );
     virtual void    paintEvent( QPaintEvent *e );
     virtual void    resizeEvent( QResizeEvent *e );
};


class QGCanvas : public QWidget
{
  Q_OBJECT
     
 public:
  QGCanvas(int asad, int aget, QWidget *parent = 0);
  virtual ~QGCanvas() {}
  QGCanvas& operator = (const QGCanvas &t);
  
  TCanvas* getCanvas(int asad, int aget) { return fCanvas[asad][aget]; }
  TCanvas* getCanvas() { return canvastmp; }  

 protected:
     virtual void    paintEvent( QPaintEvent *e );
     virtual void    resizeEvent( QResizeEvent *e );
     virtual void    mouseMoveEvent( QMouseEvent *e );
     virtual void    mousePressEvent( QMouseEvent *e );
     virtual void    mouseReleaseEvent( QMouseEvent *e );
};

class FillTheTab : public QWidget
{
  Q_OBJECT

 public slots:
   void Reset();
   void Update();   
   void Zoom();
   void handle_root_events();
   void clicked1();
   
 protected:
   QTimer         *fRootTimer;
     
 private:

   QGCanvas    *canvas[NASAD][NAGET];      

   TH1D *htmp;
   TString hname;

 public:
   explicit FillTheTab(int asad, int aget, QWidget *parent = 0);
   void resetHistograms();
   void zoomHistograms();      
   void updateCanvas();   
   TString GenerateName();
   void DrawHistograms(int asad, int aget);
   void UpdateHistograms(int asad, int aget);   
   
};

class GETmePlots : public QWidget
{
  Q_OBJECT
  
 public:
  GETmePlots();
  ~GETmePlots() {}

 private slots:
    void openFile();
    
 public slots:
    void COBOindexChanged();
    void ASADindexChanged();
    void AGETindexChanged();        
    void CHNindexChanged();
    void TBvalueChanged(const QString & text);
    void ADCvalueChanged(const QString & text);                
    void TabSelected();
    void OnlineSelected(const QString & text);    
    void Snapshot();
    void Next();
    void Draw();
    

 private:
    void createActions();      
    void createMenu();
    void createTopGroupBox();
    void createBottomGroupBox();   
    void createTabs();
    void createConnections();
    void createStatusBar();
    void SetStatusBar(std::string msg);
    void createHistograms();
    void EnableAll();
    void DisableAll();
    void DisableTabs();        
    
    QMenuBar *menuBar;
    QMenu *fileMenu;

    QPushButton* buttons[5];

    QTabWidget *tabWidget;
    
    QGroupBox *topGroupBox;
    QGroupBox *bottomGroupBox;   
    QGroupBox* gridBox;
    QGroupBox* gridBox2;
    
    QLabel *fTbsLabel;
    QLabel *fADCLabel;      
    QLabel* fOnlineLabel;
    
    QAction *openAct;
    QAction *onlineAct;
    QAction *exitAct;
    
    QComboBox* fCOBObox, *fASADbox, *fAGETbox, *fCHNbox;
    
    QLineEdit* fOnlinePath;
    QLineEdit* fADCvalue, *fTBbox;
    
    QCheckBox* fOnlineCheck;   

    QString fileN;
    QStatusBar* statusBar;
    
    FillTheTab* tab[NASAD][NAGET];

    GETDecoder* decoder;

    CDataSource* m_pDataSource;
    CRingItem*  pItem;
    CRingItemProcessor processor;
    
};

#endif 
