// =================================================
//  GETFileChecker Class
//
//  Description:
//    Check if the file exists or not
//
//  Genie Jhang ( geniejhang@majimak.com )
//  Giordano Cerizza ( cerizza@nscl.msu.edu )
//
// =================================================

#ifndef GETFILECHECKER
#define GETFILECHECKER

#include "TString.h"

class GETFileChecker {
  public:
    static TString CheckFile(TString filename, Bool_t print = kTRUE);

  ClassDef(GETFileChecker, 1)
};

#endif
