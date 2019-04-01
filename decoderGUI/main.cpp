#include <QApplication>

#include <stdlib.h>
#include <TApplication.h>
#include <TSystem.h>
#include <TStyle.h>
#include <TROOT.h>

#include "GETmePlots.h"

int main(int argc, char *argv[])
{
  gROOT->GetInterpreter();
  
  TApplication rootapp("Simple Qt ROOT Application", &argc, argv);
  QApplication app(argc, argv);

  GETmePlots window;
  window.resize(window.sizeHint());
  window.setGeometry( 100, 100, 700, 500 );
  window.show();

  QObject::connect( qApp, SIGNAL(lastWindowClosed()), qApp, SLOT(quit()) );
  
  return app.exec();
}
