#ifndef ALIMONITOR_H
#define ALIMONITOR_H
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

/* $Id$ */

#include <TObject.h>
#include "AliMonitorHisto.h"

class TFolder;
class TTree;
class AliRunLoader;
class AliRawReader;
class AliMonitorTrend;


class AliMonitor : public TObject {
public:
  AliMonitor();
  AliMonitor(const AliMonitor& monitor);
  AliMonitor& operator = (const AliMonitor& monitor);
  virtual ~AliMonitor() {};

  virtual void     CreateHistos(TFolder* folder) = 0;
  virtual void     CreateBranches(TTree* tree);
  virtual void     FillHistos(AliRunLoader* runLoader, 
			      AliRawReader* rawReader) = 0;

protected:
  TFolder*         fFolder;    // sub folder for monitor histograms

  AliMonitorHisto* CreateHisto1(const char* name, const char* title,
				Int_t xBins, Double_t xMin, Double_t xMax,
				const char* xTitle, const char* yTitle,
				AliMonitorHisto::ENorm norm);
  AliMonitorHisto* CreateHisto2(const char* name, const char* title,
				Int_t xBins, Double_t xMin, Double_t xMax,
				Int_t yBins, Double_t yMin, Double_t yMax,
				const char* xTitle, const char* yTitle,
				const char* zTitle,
				AliMonitorHisto::ENorm norm);
  AliMonitorTrend* CreateTrend(const char* name, const char* title,
			       const char* label, 
			       Double_t min = 0, Double_t max = 0);

  ClassDef(AliMonitor, 0)   // base class for the creation and filling of monitor histograms
};
 

#endif









