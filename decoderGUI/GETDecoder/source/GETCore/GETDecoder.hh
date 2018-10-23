// =================================================
//  GETDecoder Class
// 
// 
//  Author:
//    Genie Jhang ( geniejhang@majimak.com )
//    Giordano Cerizza ( cerizza@nscl.msu.edu )  
//
// =================================================

#ifndef GETDECODER
#define GETDECODER

#include "GETHeaderBase.hh"
#include "GETBasicFrameHeader.hh"
#include "GETBasicFrame.hh"
#include "GETFrameInfo.hh"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "TROOT.h"
#include "TString.h"
#include "TClonesArray.h"

/** Read the raw file from GET electronics and process it into GETFrame class **/
class GETDecoder
{
public:
  //! Constructor
  GETDecoder();
  //! Constructor
  GETDecoder(TString filename /*!< GRAW filename including path */);
  
  void Clear(); ///< Clear data information
  
  //! Setting the number of time buckets.
  void SetNumTbs(Int_t value = 512);
  //! Set the data file to the class.
  Bool_t SetDataUrl(std::string sourceUrl);  
  //! Return the number of time buckets.
  Int_t GetNumTbs();
  //! Return specific frame of the given frame number. If **frameID** is -1, this method returns next frame.
  GETBasicFrame *GetBasicFrame(Int_t frameID = -1);
  
  void PrintFrameInfo(Int_t frameID = -1);

  
private:
  //! Initialize variables used in the class.
  void Initialize();
  
  //! Check the end of file
  void CheckEndOfData();
  
  //! Store current frame position
  void BackupCurrentState(ULong64_t current);
  
  GETHeaderBase *fHeaderBase;
  GETBasicFrameHeader *fBasicFrameHeader;
  GETBasicFrame *fBasicFrame;
  
  TClonesArray *fFrameInfoArray;
  GETFrameInfo *fFrameInfo;
  
  Int_t fNumTbs; /// the number of time buckets. It's determined when taking data and should be changed manually by user. (Default: 512)
  
  Bool_t fIsDataInfo;             ///< Flag for data information existance
  Bool_t fIsDoneAnalyzing;        ///< Flag for all the frame info are read

  ULong64_t startByte;            ///< Start bytes to traverse the stringstream buffer
  ULong64_t endByte;              ///< End bytes to traverse the stringstream buffer
  
  std::stringstream oData;        ///< Current file data stream
  std::string sData;              ///< String dump of the stringstream
  ULong64_t fDataSize;            ///< Current file size
  
  Int_t fFrameInfoIdx;                ///< Current frame index
  Int_t fTargetFrameInfoIdx;          ///< Target frame or cobo frame index to return
  
  ULong64_t fPrevPosition;  ///< Byte number for going back to original data

  ClassDef(GETDecoder, 1); /// added for making dictionary by ROOT
};

#endif
