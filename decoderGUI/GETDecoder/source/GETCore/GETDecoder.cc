// =================================================
//  GETDecoder Class
// 
//  Author:
//    Genie Jhang ( geniejhang@majimak.com )
//    Giordano Cerizza ( cerizza@nscl.msu.edu )  
//
// =================================================

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>
#include <arpa/inet.h>
#include <ext/stdio_filebuf.h>

#include "TString.h"
#include "TFile.h"
#include "TTree.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TSystem.h"

#include "GETDecoder.hh"
#include "GETFileChecker.hh"

#include "CDataSource.h"
#include "CDataSourceFactory.h"
#include "CRingItem.h"

ClassImp(GETDecoder);

GETDecoder::GETDecoder()
  :fFrameInfoArray(NULL), fFrameInfo(NULL), fHeaderBase(NULL), fBasicFrameHeader(NULL), fBasicFrame(NULL)
{
  Initialize();
}

GETDecoder::GETDecoder(TString filename)
  :fFrameInfoArray(NULL), fFrameInfo(NULL), fHeaderBase(NULL), fBasicFrameHeader(NULL), fBasicFrame(NULL)
{
  Initialize();
  SetDataUrl((std::string)filename);  
}

void GETDecoder::Initialize()
{
  fNumTbs = 512;
  
  fIsDoneAnalyzing = kFALSE;
  fIsDataInfo = kFALSE;

  fDataSize = 0;

  fFrameInfoIdx = 0;
  fTargetFrameInfoIdx = -1;
  
  if (    fFrameInfoArray == NULL) fFrameInfoArray = new TClonesArray("GETFrameInfo", 10000);
  fFrameInfoArray -> Clear("C");
  
  if (      fHeaderBase == NULL) fHeaderBase = new GETHeaderBase();
  else                           fHeaderBase -> Clear();
  
  if (fBasicFrameHeader == NULL) fBasicFrameHeader = new GETBasicFrameHeader();
  else                           fBasicFrameHeader -> Clear();
  
  if (      fBasicFrame == NULL) fBasicFrame = new GETBasicFrame();
  else                           fBasicFrame -> Clear();

  fPrevPosition = 0;
  startByte = 0;
  endByte = 0;  
}

void GETDecoder::Clear() {

  fIsDoneAnalyzing = kFALSE;
  fIsDataInfo = kFALSE;

  fDataSize = 0;

  fFrameInfoIdx = 0;
  fTargetFrameInfoIdx = -1;

  fFrameInfoArray -> Clear("C");
  fHeaderBase -> Clear();
  fBasicFrameHeader -> Clear();
  fBasicFrame -> Clear();
  
}

void GETDecoder::SetNumTbs(Int_t value) { fNumTbs = value; } 

Bool_t GETDecoder::SetDataUrl(std::string sourceUrl)
{
  std::cout << "== [GETDecoder] " << sourceUrl << " is opened!" << std::endl;

  CDataSource* pSource;
  try {
    std::vector<uint16_t> empty;
    pSource = CDataSourceFactory::makeSource(sourceUrl, empty, empty);
  }
  catch (...) {
    std::cout << "Exception caught making the data source" << std::endl;
    exit(0);
  }
  
  CRingItem* pItem;
  while(pItem = pSource->getItem()) {
    uint32_t type  = pItem->type();
    if (type == 30) {
      void* pData  = pItem->getBodyPointer();
      size_t nByte = pItem->getBodySize();
      oData.write((char*)pData, nByte);
    }
    delete pItem;
  }
  // check the size of the stringstream
  //  oData.seekg(0, ios::end);
  //  std::cout << "Size of stringstream: " << oData.tellg() << std::endl;
  oData.seekg(0, ios::beg);
  sData = oData.str();
  
  std::istream& Data = oData;
  fDataSize = Data.tellg();
  Data.seekg(0);
  
  if (!fIsDataInfo) {
    fHeaderBase -> Read(Data, kTRUE);
    
    int frametype = fHeaderBase -> GetFrameType();
    std::cout << "== [GETDecoder] Frame Type: " ;
    if (frametype == 1)
      std::cout << " Basic frame (partial readout mode)" << std::endl;
    else if (frametype == 2)
      std::cout << " Basic frame (full readout mode)" << std::endl;

    fIsDataInfo = kTRUE;
  }
  
  return kTRUE;

}

Int_t GETDecoder::GetNumTbs() { return fNumTbs; }

GETBasicFrame *GETDecoder::GetBasicFrame(Int_t frameID)
{
  std::stringstream pData(sData);
  // Size check to make sure we go through the whole file
  pData.seekg(0, ios::end);
  fDataSize = pData.tellg();
  pData.seekg(0, ios::beg);
  
  startByte=endByte; 
  pData.seekg(startByte);    
  std::istream& Data = pData;
  
  if (frameID == -1)
    fTargetFrameInfoIdx++;
  else
    fTargetFrameInfoIdx = frameID;

  while (kTRUE) {
    Data.clear();    
    
    if (fIsDoneAnalyzing)
      if (fTargetFrameInfoIdx > fFrameInfoArray -> GetLast())
        return NULL;

    if (fFrameInfoIdx > fTargetFrameInfoIdx)
      fFrameInfoIdx = fTargetFrameInfoIdx;
    
    fFrameInfo = (GETFrameInfo *) fFrameInfoArray -> ConstructedAt(fFrameInfoIdx);
    while (fFrameInfo -> IsFill()) {

#ifdef DEBUG
      cout << "fFrameInfoIdx: " << fFrameInfoIdx << " fTargetFrameInfoIdx: " << fTargetFrameInfoIdx << endl;
#endif

      if (fFrameInfoIdx == fTargetFrameInfoIdx) {
        BackupCurrentState(startByte);

	Data.seekg(fFrameInfo -> GetStartByte());	
        fBasicFrame -> Read(Data);	

#ifdef DEBUG
	cout << "Returned event ID: " << fBasicFrame -> GetEventID() << endl;
#endif
	
        return fBasicFrame;
      } else
        fFrameInfo = (GETFrameInfo *) fFrameInfoArray -> ConstructedAt(++fFrameInfoIdx);
    }
    
    fBasicFrameHeader -> Read(Data);
    Data.ignore(fBasicFrameHeader -> GetFrameSkip());
    
    endByte = startByte + fBasicFrameHeader -> GetFrameSize();    
    fFrameInfo -> SetStartByte(startByte);
    fFrameInfo -> SetEndByte(endByte);
    fFrameInfo -> SetEventID(fBasicFrameHeader -> GetEventID());

    CheckEndOfData();
  }
}

void GETDecoder::PrintFrameInfo(Int_t frameID) {
  if (frameID == -1) {
    for (Int_t iEntry = 0; iEntry < fFrameInfoArray -> GetEntriesFast(); iEntry++)
      ((GETFrameInfo *) fFrameInfoArray -> At(iEntry)) -> Print();
  } else
    ((GETFrameInfo *) fFrameInfoArray -> At(frameID)) -> Print();
}

void GETDecoder::CheckEndOfData() {
  if (fFrameInfo -> GetEndByte() == fDataSize)
    if (!fIsDoneAnalyzing) {
      fIsDoneAnalyzing = kTRUE;
    }
}

void GETDecoder::BackupCurrentState(ULong64_t current) {
  std::stringstream pData(sData);
  pData.seekg(current);
  std::istream& Data = pData;
  
  fPrevPosition = Data.tellg();  
}

