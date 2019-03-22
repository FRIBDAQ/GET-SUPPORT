// =================================================
//  GETDecoder Class
//
//
//  Author:
//    Giordano Cerizza ( cerizza@nscl.msu.edu )
//
// =================================================

#ifndef GETDECODER
#define GETDECODER
#include <iostream>
// NSCLDAQ Headers:

#include <CDataSource.h>              // Abstract source of ring items.
#include <CDataSourceFactory.h>       // Turn a URI into a concrete data source
#include <CRingItem.h>                // Base class for ring items.
#include <DataFormat.h>                // Ring item data formats.
#include <Exception.h>                // Base class for exception handling.
#include <CRingItemFactory.h>         // creates specific ring item from generic.
#include <CRingScalerItem.h>          // Specific ring item classes.
#include <CRingStateChangeItem.h>     //                 |
#include <CRingTextItem.h>            //                 |
#include <CPhysicsEventItem.h>         //                |
#include <CRingPhysicsEventCountItem.h> //               |
#include <CDataFormatItem.h>          //                 |
#include <CGlomParameters.h>          //                 |
#include <CDataFormatItem.h>          //             ----+----

#include <CDataSink.h>
#include <CDataSinkFactory.h>

// Headers for other modules in this program:
#include "processor.h"
#include "AnalyzeFrame.h"

// standard run time headers:
#include <iostream>
#include <cstdlib>
#include <memory>
#include <vector>
#include <cstdint>

class GETDecoder
{

 private:

  static GETDecoder* m_pInstance;
  
  GETDecoder();
  virtual ~GETDecoder();
  
 public:

  static GETDecoder* getInstance();

  void SetUrls(std::string src);
  std::string GetSourceUrl();

  std::vector<NSCLGET::Hit> GetFrame();
  void SetHits(std::vector<NSCLGET::Hit>& hit);
  std::vector<NSCLGET::Hit> GetHits();

  unsigned GetCobo();
  unsigned GetAsAd();  
  unsigned GetAget();
  unsigned GetChannel();  
  std::vector<std::pair<unsigned, unsigned>> GetRawADC();

  CDataSource*              m_pDataSource;
  std::string               m_sourceUrl;
  std::vector<NSCLGET::Hit> m_pHits;
  
};

#endif
