//
//  AliEveSaveViews.cxx
//
//
//  Created by Jeremi Niedziela on 16/03/15.
//
//

#include "AliEveSaveViews.h"
#include "AliEveMultiView.h"
#include "AliEveInit.h"

#include <TGFileDialog.h>
#include <TGLViewer.h>
#include <TEveViewer.h>
#include <TEveManager.h>
#include <TEveBrowser.h>
#include <TMath.h>
#include <TTimeStamp.h>
#include <TEnv.h>
#include <TSQLServer.h>
#include <TSQLResult.h>
#include <TSQLRow.h>
#include <TSystem.h>
#include <TObjArray.h>
#include <TObjString.h>

#include <AliRawReader.h>
#include <AliRawEventHeaderBase.h>
#include <AliEveEventManager.h>
#include <AliDAQ.h>

#include <iostream>
using namespace std;


AliEveSaveViews::AliEveSaveViews(int width,int height) :
fHeightInfoBar(0.0722*height),
fWidth(width),
fHeight(height)
{
  fRunNumber=-1;
  fCurrentFileNumber=0;
  fNumberOfClusters=-1;
  fMaxFiles=100;
  fCompositeImgFileName="";

  for (int i=0; i<3; i++) {
    fTriggerClasses[i]="";
  }
  fClustersInfo="";
}


AliEveSaveViews::~AliEveSaveViews()
{

}

void AliEveSaveViews::ChangeRun()
{
  // read crededentials to logbook from config file:
  TEnv settings;
  AliEveInit::GetConfig(&settings);
  const char *dbHost = settings.GetValue("logbook.host", "");
  Int_t   dbPort =  settings.GetValue("logbook.port", 0);
  const char *dbName =  settings.GetValue("logbook.db", "");
  const char *user =  settings.GetValue("logbook.user", "");
  const char *password = settings.GetValue("logbook.pass", "");

  // get entry from logbook:
  cout<<"creating sql server...";
  TSQLServer* server = TSQLServer::Connect(Form("mysql://%s:%d/%s", dbHost, dbPort, dbName), user, password);
  if(!server){
    cout<<"Server could not be created"<<endl;
    return;
  }
  else{
    cout<<"created"<<endl;
  }
  TString sqlQuery;
  sqlQuery.Form("SELECT * FROM logbook_trigger_clusters WHERE run = %d",fRunNumber);

  TSQLResult* result = server->Query(sqlQuery);
  if (!result){
    Printf("ERROR: Can't execute query <%s>!", sqlQuery.Data());
    return;
  }

  if (result->GetRowCount() == 0){
    Printf("ERROR: Run %d not found", fRunNumber);
    delete result;
    return;
  }

  // read values from logbook entry:
  TSQLRow* row;
  int i=0;
  fCluster.clear();

  while((row=result->Next()))
  {
    fCluster.push_back(atoi(row->GetField(2)));
    fInputDetectorMask.push_back(atol(row->GetField(4)));
    i++;
  }
  fNumberOfClusters=i;
}

void AliEveSaveViews::SaveForAmore(){
  gEve->DoRedraw3D();

  fESDEvent = AliEveEventManager::Instance()->GetESD();

  if(!fESDEvent) {
    cout<<"AliEveSaveViews -- event manager has no esd file"<<endl;
    return;
  }

  // update trigger info if run changed
  if(fESDEvent->GetRunNumber()!=fRunNumber) {
    fRunNumber = fESDEvent->GetRunNumber();
    ChangeRun();
  }

  // get visualisation screenshot
  TASImage *compositeImg = GetPicture(fWidth, fHeight+1.33*fHeightInfoBar, fHeight);
  if(!compositeImg){
    cout<<"AliEveSaveViews -- couldn't produce visualisation screenshot!"<<endl;
    return;
  }


  // draw ALICE LIVE bar
  TTimeStamp ts;
  UInt_t year,month,day;
  UInt_t hour,minute,second;

  ts.GetDate(kFALSE, 0, &year, &month,&day);
  ts.GetTime(kFALSE, 0, &hour, &minute,&second);

  TString gmtNow = TString::Format("%u-%.2u-%.2u %.2u:%.2u:%.2u",year,month,day,hour,minute,second);

  compositeImg->Gradient( 90, "#EAEAEA #D2D2D2 #FFFFFF", 0, 0.03*fWidth, 0.000*fHeight, 0.20*fWidth, 0.10*fHeight);
  compositeImg->Gradient( 90, "#D6D6D6 #242424 #000000", 0, 0.08*fWidth, 0.066*fHeight, 0.15*fWidth, 0.03*fHeight);
  compositeImg->BeginPaint();
  compositeImg->DrawRectangle(0.01*fWidth,0*fHeight, 0.23*fWidth, 0.1*fHeight);
  compositeImg->DrawText(0.11*fWidth, 0.005*fHeight, "LIVE", 0.08*fHeight, "#FF2D00", "FreeSansBold.otf");
  compositeImg->DrawText(0.09*fWidth, 0.073*fHeight, gmtNow, 0.02*fHeight, "#FFFFFF", "arial.ttf");
  compositeImg->DrawText(0.20*fWidth, 0.073*fHeight, "Geneva", 0.01*fHeight, "#FFFFFF", "arial.ttf");
  compositeImg->DrawText(0.20*fWidth, 0.083*fHeight, "time", 0.01*fHeight, "#FFFFFF", "arial.ttf");
  compositeImg->EndPaint();

  //include ALICE Logo
  TASImage *aliceLogo = new TASImage(Form("%s/EVE/resources/alice_logo.png",gSystem->Getenv("ALICE_ROOT")));
  if(aliceLogo) {
    aliceLogo->Scale(0.037*fWidth,319./236.*0.037*fWidth);
    compositeImg->Merge(aliceLogo, "alphablend", 0.034*fWidth, 0.004*fHeight);
    delete aliceLogo;aliceLogo=0;
  }

  // draw information bar
  BuildEventInfoString();
  BuildTriggerClassesStrings();
  BuildClustersInfoString();

  // put event's info in blue bar on the bottom:
  compositeImg->Gradient( 90, "#1B58BF #1D5CDF #0194FF", 0, 0,fHeight, fWidth, fHeightInfoBar);
  compositeImg->BeginPaint();
  compositeImg->DrawText(0.007*fWidth, 1.0166*fHeight, fEventInfo, 0.03*fHeight, "#FFFFFF", "FreeSansBold.otf");
  // put trigger classes in blue bar on the bottom:
  compositeImg->DrawText(0.52 *fWidth, 1.0044*fHeight, fTriggerClasses[0], 0.0177*fHeight, "#FFFFFF", "FreeSansBold.otf");
  compositeImg->DrawText(0.52 *fWidth, 1.0244*fHeight, fTriggerClasses[1], 0.0177*fHeight, "#FFFFFF", "FreeSansBold.otf");
  compositeImg->DrawText(0.52 *fWidth, 1.0488*fHeight, fTriggerClasses[2], 0.0177*fHeight, "#FFFFFF", "FreeSansBold.otf");
  compositeImg->DrawText(0.007*fWidth, 0.9333*fHeight, "Preliminary reconstruction", 0.03*fHeight, "#60FFFFFF", "FreeSansBold.otf");
  compositeImg->DrawText(0.007*fWidth, 0.9666*fHeight, "(not for publication)", 0.03*fHeight, "#60FFFFFF", "FreeSansBold.otf");
  compositeImg->EndPaint();
  // put clusters description in green bar on the bottom:
  compositeImg->Gradient( 90, "#1BDD1B #1DDD1D #01DD01", 0, 0, fHeight+fHeightInfoBar, fWidth, 0.33*fHeightInfoBar);
  compositeImg->BeginPaint();
  compositeImg->DrawText(0.007*fWidth,fHeight+fHeightInfoBar+2,fClustersInfo, 0.0177*fHeight, "#000000", "FreeSansBold.otf");
  compositeImg->EndPaint();

  // write composite image to disk
  fCompositeImgFileName = Form("online-viz-%03d", fCurrentFileNumber);
  compositeImg->WriteImage(Form("%s.png", fCompositeImgFileName.Data()));
  if (++fCurrentFileNumber >= fMaxFiles) fCurrentFileNumber = 0;

  // trick to refresh the window
  TEveBrowser *browser = gEve->GetBrowser();
  browser->MoveResize(browser->GetX(), browser->GetY(), browser->GetWidth(),browser->GetHeight());
  gSystem->ProcessEvents();

  delete compositeImg;
}

void AliEveSaveViews::Save(bool withDialog,const char* filename){
  gEve->DoRedraw3D();

  fESDEvent = AliEveEventManager::Instance()->GetESD();

  if(!fESDEvent) {
    cout<<"AliEveSaveViews -- event manager has no esd file"<<endl;
    return;
  }

  // read eve config
  TEnv settings;
  AliEveInit::GetConfig(&settings);

  // get visualisation screenshot
  Int_t fWidthSave        = settings.GetValue("screenshot.width",2000);
  Int_t fHeightSave       = settings.GetValue("screenshot.height",1500);
  TASImage *compositeImg  = GetPicture(fWidthSave,fHeightSave,fHeightSave,settings.GetValue("screenshot.projections.draw",true));
  if(!compositeImg){
    cout<<"AliEveSaveViews -- couldn't produce visualisation screenshot!"<<endl;
    return;
  }

  TASImage *aliceLogo;
  //draw ALICE Logo
  if(settings.GetValue("screenshot.logo.draw",true)) {
    TString nameLogo = Form("%s/EVE/resources/alice_logo_big.png",gSystem->Getenv("ALICE_ROOT"));
    if (settings.GetValue("background.color",1) == 0){
      nameLogo = Form("%s/EVE/resources/alice_logo_big_black.png",gSystem->Getenv("ALICE_ROOT"));
    }
    aliceLogo = new TASImage(nameLogo.Data());
    if(aliceLogo) {
      double ratio = 1434./1939.; // ratio of the ALICE logo as in the alice_logo_big.png file
      aliceLogo->Scale(0.08*fWidthSave,0.08*fWidthSave/ratio);
      compositeImg->Merge(aliceLogo, "alphablend", 20, 20);
    }
  }
  TString runNumber       = "";
  TString timeStamp       = "";
  TString system          = "";
  TString energy          = "";
  TString color           = "#BBBBBB";
  TString colorBG         = "#000000";
  TString additionalInfo  = settings.GetValue("screenshot.additional.info","");
  int infoOffset          = 0;
  int fontSize            = 0.015*fHeightSave;

  //draw info
  if(settings.GetValue("screenshot.info.draw",true)) {
    TTimeStamp ts(fESDEvent->GetTimeStamp());
    runNumber = Form("Run:%d",fESDEvent->GetRunNumber());
    timeStamp = Form("Timestamp:%s(UTC)",ts.AsString("s"));
    system;

    cout<<"Beam:"<<fESDEvent->GetBeamType()<<endl;
    cout<<"Energy:"<<fESDEvent->GetBeamEnergy()<<endl;

    const char *systemLabel= settings.GetValue("screenshot.force.system","System: unknown");
    if(strcmp(systemLabel,"")==0){
      if(strcmp(fESDEvent->GetBeamType(),"")!=0){ // if not empty
        if(strcmp(fESDEvent->GetBeamType(),"A-A")==0){
          system = "Colliding system: Pb-Pb";
        } else if(strcmp(fESDEvent->GetBeamType(),"p-A")==0){
          system = "Colliding system: p-Pb";
        } else if(strcmp(fESDEvent->GetBeamType(),"A-p")==0){
          system = "Colliding system: Pb-p";
        } else {
          system = Form("Colliding system:%s",fESDEvent->GetBeamType());
        }
      } else { // if beam info empty
        system = "System: unknown";;
      }
    } else {
      system = systemLabel;
    }

    Double_t beamEnergy = fESDEvent->GetBeamEnergy();
    const char *energyLabel= settings.GetValue("screenshot.force.energy","Energy: unknown");
    if(strcmp(energyLabel,"")==0){
      if(beamEnergy>=0.0000001){
        energy = Form("Energy:%.2f TeV",2*beamEnergy/1000.);
      } else {
        energy = "Energy: unknown";
      }
    } else {
      energy = energyLabel;
    }

    if(strcmp(additionalInfo,"") != 0){ // if there is some additional info
      infoOffset = -5-1*fontSize;
    }

    cout << "BG color" << settings.GetValue("background.color",1) << endl;
    if (settings.GetValue("background.color",1) == 0){
      color     = "#000000";
      colorBG   = "#FFFFFF";
      fontSize  = fontSize*1.25;
    } else if (settings.GetValue("background.color",1) == 2){
      color     = "#FFFFFF";
      fontSize  = fontSize*1.25;
    }

    compositeImg->BeginPaint();
    compositeImg->DrawText(10,fHeightSave-25-4*fontSize+infoOffset, runNumber.Data(), fontSize,color.Data(),"FreeSansBold.otf");
    compositeImg->DrawText(10,fHeightSave-20-3*fontSize+infoOffset, timeStamp.Data(), fontSize,color.Data(),"FreeSansBold.otf");
    compositeImg->DrawText(10,fHeightSave-15-2*fontSize+infoOffset, system.Data(),    fontSize,color.Data(),"FreeSansBold.otf");
    compositeImg->DrawText(10,fHeightSave-10-1*fontSize+infoOffset, energy.Data(),    fontSize,color.Data(),"FreeSansBold.otf");
    compositeImg->DrawText(10,fHeightSave-10-1*fontSize      , additionalInfo.Data(), fontSize,color.Data(),"FreeSansBold.otf");
    compositeImg->EndPaint();
  }

  // Save screenshot to file
  TString fileNameOut = "";
  if(withDialog) {
    TGFileInfo fileinfo;
    const char *filetypes[] = {"All types", "*", 0, 0};
    fileinfo.fFileTypes = filetypes;
    fileinfo.fIniDir = StrDup(".");
    new TGFileDialog(gClient->GetDefaultRoot(), gClient->GetDefaultRoot(),kFDOpen, &fileinfo);
    if(!fileinfo.fFilename){
      cout<<"AliEveSaveViews::SaveWithDialog() -- couldn't get path from dialog window!!!"<<endl;
      return;
    }
    fileNameOut = fileinfo.fFilename;
    compositeImg->WriteImage(fileinfo.fFilename);
  } else {
    compositeImg->WriteImage(filename);
    fileNameOut = filename;
  }

  if (settings.GetValue("screenshot.single.draw",true)){
    TString namesSubWindows[3]  = {"3D View MV", "RPhi View","RhoZ View"};
    TString namesSubName[3]     = {"3D", "RPhi","RhoZ"};
    for (Int_t i = 0; i<3; i++){
      TASImage *singleImg  = GetPictureSingle(fWidthSave,fHeightSave,namesSubWindows[i]);
      if(!singleImg){
        cout<<"AliEveSaveViews -- couldn't produce visualisation screenshot!"<<endl;
        return;
      }
      if (i > 0){
        singleImg->Merge(aliceLogo, "alphablend", settings.GetValue("screenshot.logo.single.x",20), settings.GetValue("screenshot.logo.single.y",20));
        singleImg->BeginPaint();
        singleImg->DrawText(settings.GetValue("screenshot.label.single.x",10),fHeightSave-settings.GetValue("screenshot.label.single.y",0)-25-4*fontSize+infoOffset, runNumber.Data(), fontSize,color.Data(),"FreeSansBold.otf");
        singleImg->DrawText(settings.GetValue("screenshot.label.single.x",10),fHeightSave-settings.GetValue("screenshot.label.single.y",0)-20-3*fontSize+infoOffset, timeStamp.Data(), fontSize,color.Data(),"FreeSansBold.otf");
        singleImg->DrawText(settings.GetValue("screenshot.label.single.x",10),fHeightSave-settings.GetValue("screenshot.label.single.y",0)-15-2*fontSize+infoOffset, system.Data(),    fontSize,color.Data(),"FreeSansBold.otf");
        singleImg->DrawText(settings.GetValue("screenshot.label.single.x",10),fHeightSave-settings.GetValue("screenshot.label.single.y",0)-10-1*fontSize+infoOffset, energy.Data(),    fontSize,color.Data(),"FreeSansBold.otf");
        singleImg->DrawText(settings.GetValue("screenshot.label.single.x",10),fHeightSave-settings.GetValue("screenshot.label.single.y",0)-10-1*fontSize      , additionalInfo.Data(), fontSize,color.Data(),"FreeSansBold.otf");
        singleImg->EndPaint();
      } else {
        singleImg->Merge(aliceLogo, "alphablend", 20, 20);
        singleImg->BeginPaint();
        singleImg->DrawText(10,fHeightSave-25-4*fontSize+infoOffset, runNumber.Data(), fontSize,color.Data(),"FreeSansBold.otf");
        singleImg->DrawText(10,fHeightSave-20-3*fontSize+infoOffset, timeStamp.Data(), fontSize,color.Data(),"FreeSansBold.otf");
        singleImg->DrawText(10,fHeightSave-15-2*fontSize+infoOffset, system.Data(),    fontSize,color.Data(),"FreeSansBold.otf");
        singleImg->DrawText(10,fHeightSave-10-1*fontSize+infoOffset, energy.Data(),    fontSize,color.Data(),"FreeSansBold.otf");
        singleImg->DrawText(10,fHeightSave-10-1*fontSize      , additionalInfo.Data(), fontSize,color.Data(),"FreeSansBold.otf");
        singleImg->EndPaint();
      }

      Printf("%s\n",fileNameOut.Data());
      TObjArray *sep = fileNameOut.Tokenize(".");
      TString fileNameCurr = "";
      for(Int_t i = 0; i<sep->GetEntries()-1 ; i++){
        TObjString* temp = (TObjString*) sep->At(i);
        TString tempStr = temp->GetString();
        fileNameCurr.Append(tempStr);
      }
      fileNameCurr.Append(namesSubName[i].Data());
      TObjString* temp = (TObjString*) sep->At(sep->GetEntries()-1);
      TString tempStr = temp->GetString();
      fileNameCurr.Append(".");
      fileNameCurr.Append(tempStr);
      Printf("Writing %s\n", fileNameCurr.Data());

      singleImg->WriteImage(fileNameCurr);
      delete singleImg;
    }
  }
  // trick to refresh the window
  TEveBrowser *browser = gEve->GetBrowser();
  browser->MoveResize(browser->GetX(), browser->GetY(), browser->GetWidth(),browser->GetHeight());
  gSystem->ProcessEvents();
  if (aliceLogo) delete aliceLogo;
  delete compositeImg;
}

TASImage* AliEveSaveViews::GetPicture(int width, int heigth, int height3DView, bool projections){
  TASImage *compositeImg = new TASImage(width, heigth);

  TEveViewerList* viewers = gEve->GetViewers();
  int Nviewers = viewers->NumChildren()-2; // remark: 3D view is counted twice

  // 3D View size
  int width3DView = projections ? TMath::FloorNint(2.*width/3.) : width;  // the width of the 3D view
  float aspectRatio = (float)width3DView/(float)height3DView;             // 3D View aspect ratio

  // Children View Size
  int heightChildView = TMath::FloorNint((float)height3DView/Nviewers);
  int widthChildView  = TMath::FloorNint((float)width/3.);
  float childAspectRatio = (float)widthChildView/(float)heightChildView; // 3D View aspect ratio

  int index=0;            // iteration counter
  int x = width3DView;    // x position of the child view
  int y = 0;              // y position of the child view

  for(TEveElement::List_i i = viewers->BeginChildren(); i != viewers->EndChildren(); i++){
    TEveViewer* view = ((TEveViewer*)*i);

    if((strcmp(view->GetName(),"3D View MV")!=0) &&
      (strcmp(view->GetName(),"RPhi View")!=0) &&
      (strcmp(view->GetName(),"RhoZ View")!=0)){
      continue;
      }

      TASImage *viewImg;

    // We can use either get picture using FBO or BB
    // FBO improves the quality of pictures in some specific cases
    // but there were some problems with it:
    // - moving mouse over views makes them disappear on new event being loaded
    // - scale of the view can be broken after getting the picture
    // - scale of the resulting image can be incorrect

    // viewImg = (TASImage*)view->GetGLViewer()->GetPictureUsingBB();

    if(index==0){
      viewImg = (TASImage*)view->GetGLViewer()->GetPictureUsingFBO(width3DView, height3DView);
    } else {
      viewImg = (TASImage*)view->GetGLViewer()->GetPictureUsingFBO(widthChildView, heightChildView);
    }

    if(viewImg){
      // copy view image in the composite image
      int currentWidth = viewImg->GetWidth();
      int currentHeight = viewImg->GetHeight();

      if(index==0){
        if(currentWidth < aspectRatio*currentHeight) {
          viewImg->Crop(0,(currentHeight-currentWidth/aspectRatio)*0.5,currentWidth,currentWidth/aspectRatio);
        } else {
          viewImg->Crop((currentWidth-currentHeight*aspectRatio)*0.5,0,currentHeight*aspectRatio,currentHeight);
        }
        viewImg->Scale(width3DView,height3DView);
        viewImg->CopyArea(compositeImg, 0,0, width3DView, height3DView);
      } else {
        if(currentWidth < aspectRatio*currentHeight) {
          viewImg->Crop(0,(currentHeight-currentWidth/childAspectRatio)*0.5,currentWidth,currentWidth/childAspectRatio);
        } else {
          viewImg->Crop((currentWidth-currentHeight*childAspectRatio)*0.5,0,currentHeight*childAspectRatio,currentHeight);
        }
        viewImg->Scale(widthChildView,heightChildView);
        viewImg->CopyArea(compositeImg,0,0, widthChildView, heightChildView, x,y);
        compositeImg->DrawRectangle(x,y, widthChildView, heightChildView, "#C0C0C0"); // draw a border around child views
      }
      delete viewImg;viewImg=0;
    }
    if(index>0){ // skip 3D View
      y+=heightChildView;
    }
    index++;
    if(!projections){
      break;
    }
  }

  return compositeImg;
}

TASImage* AliEveSaveViews::GetPictureSingle(int width, int heigth, TString subViewName ){

  TASImage *viewImg;

  TEveViewerList* viewers = gEve->GetViewers();
  int Nviewers = viewers->NumChildren()-2; // remark: 3D view is counted twice

  float aspectRatio = (float)width/(float)heigth;             // 3D View aspect ratio

  for(TEveElement::List_i i = viewers->BeginChildren(); i != viewers->EndChildren(); i++){
    TEveViewer* view = ((TEveViewer*)*i);

    if( strcmp(view->GetName(),subViewName.Data())!=0 ){
      continue;
    }

    // We can use either get picture using FBO or BB
    // FBO improves the quality of pictures in some specific cases
    // but there were some problems with it:
    // - moving mouse over views makes them disappear on new event being loaded
    // - scale of the view can be broken after getting the picture
    // - scale of the resulting image can be incorrect

    // viewImg = (TASImage*)view->GetGLViewer()->GetPictureUsingBB();
    viewImg = (TASImage*)view->GetGLViewer()->GetPictureUsingFBO(width, heigth);

    if(viewImg){
      // copy view image in the composite image
      int currentWidth = viewImg->GetWidth();
      int currentHeight = viewImg->GetHeight();

      if(currentWidth < aspectRatio*currentHeight) {
        viewImg->Crop(0,(currentHeight-currentWidth/aspectRatio)*0.5,currentWidth,currentWidth/aspectRatio);
      } else {
        viewImg->Crop((currentWidth-currentHeight*aspectRatio)*0.5,0,currentHeight*aspectRatio,currentHeight);
      }
      viewImg->Scale(width,heigth);
    }
  }
  return viewImg;
}

void AliEveSaveViews::BuildEventInfoString(){

  AliRawReader* rawReader = AliEveEventManager::AssertRawReader();
  if(rawReader){
    fEventInfo.Form("Run: %d  Event#: %d (%s)",
                    rawReader->GetRunNumber(),
                    AliEveEventManager::Instance()->GetEventId(),
                    AliRawEventHeaderBase::GetTypeName(rawReader->GetType())
    );
    return;
  } else {
    AliESDEvent* esd =  AliEveEventManager::Instance()->AssertESD();
    if(esd) {
      fEventInfo.Form("Colliding: %s Run: %d  Event: %d (%s)",
                      esd->GetESDRun()->GetBeamType(),
                      esd->GetRunNumber(),
                      esd->GetEventNumberInFile(),
                      AliRawEventHeaderBase::GetTypeName(esd->GetEventType())
      );
      return;
    } else {
      fEventInfo="";
    }
  }
}

void AliEveSaveViews::BuildTriggerClassesStrings()
{
  ULong64_t mask = 1;
  int sw=0;

  ULong64_t triggerMask = fESDEvent->GetTriggerMask();
  ULong64_t triggerMaskNext50 = fESDEvent->GetTriggerMaskNext50();

  fTriggerClasses[0]="";
  fTriggerClasses[1]="";
  fTriggerClasses[2]="";

  for(int clusterIter=0;clusterIter<fNumberOfClusters;clusterIter++)//loop over all clusters in run
  {
    // get trigger classes for given cluster
    mask=1;
    for(int i=0;i<50;i++)
    {
      if(mask & triggerMask)
      {
        fTriggerClasses[sw]+=fESDEvent->GetESDRun()->GetTriggerClass(i);
        fTriggerClasses[sw]+=Form("(%d)",fCluster[clusterIter]);
        fTriggerClasses[sw]+="   ";

        if(sw==0)sw=1;
        else if(sw==1)sw=2;
        else if(sw==2)sw=0;
      }
      if(mask & triggerMaskNext50)
      {
        fTriggerClasses[sw]+=fESDEvent->GetESDRun()->GetTriggerClass(i+50);
        fTriggerClasses[sw]+=Form("(%d)",fCluster[clusterIter]);
        fTriggerClasses[sw]+="   ";

        if(sw==0)sw=1;
        else if(sw==1)sw=2;
        else if(sw==2)sw=0;
      }
      mask = mask<<1;
    }
  }
}

void AliEveSaveViews::BuildClustersInfoString()
{
  vector<TString> clustersDescription;
  ULong64_t mask = 1;

  for(int clusterIter=0;clusterIter<fNumberOfClusters;clusterIter++)//loop over all clusters in run
  {
    string clustersInfo="";
    mask=1;
    for(int i=0;i<22;i++)
    {
      if(fInputDetectorMask[clusterIter] & mask)
      {
        clustersInfo+=AliDAQ::DetectorName(i);
        clustersInfo+=", ";
      }

      mask=mask<<1;
    }
    clustersInfo = clustersInfo.substr(0, clustersInfo.size()-2);

    clustersDescription.push_back(TString(clustersInfo));
  }

  fClustersInfo = "";

  for (int i=0;i<clustersDescription.size();i++) {
    fClustersInfo+="Cluster ";
    fClustersInfo+=fCluster[i];
    fClustersInfo+=":";
    fClustersInfo+=clustersDescription[i];
    fClustersInfo+="   ";
  }
}

int AliEveSaveViews::SendToAmore()
{
  return gSystem->Exec(Form("SendImageToAmore %s %s.png %d",fCompositeImgFileName.Data(),fCompositeImgFileName.Data(),fRunNumber));
}
