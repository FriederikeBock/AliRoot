/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/*
$Log$
*/

//======================================================================
//  AliGenPHOSlib class contains parameterizations of the
//  pion, kaon, eta, omega, etaprime, phi and baryon (proton, 
//  antiproton, neutron and anti-neutron) particles for the 
//  study of the neutral background in PHOS detector. 
//  These parameterizations are used by the 
//  AliGenParam  class:
//  AliGenParam(npar, param,  AliGenPHOSlib::GetPt(param),
//                            AliGenPHOSlib::GetY(param),
//                            AliGenPHOSlib::GetIp(param) )
//  param represents the particle to be simulated : 
//  Pion, Kaon, Eta, Omega, Etaprime, Phi or Baryon    
//  Pt distributions are calculated from the transverse mass scaling 
//  with Pions, using the PtScal fonction taken from AliGenMUONlib 
//  version aliroot 3.01
//
//     Gines MARTINEZ GPS @ SUBATECH,  Nantes , France
//     http://www-subatech.in2p3.fr/~photons/subatech
//     martinez@subatech.in2p3.fr
//======================================================================

#include "AliGenPHOSlib.h"
#include "AliRun.h"

ClassImp(AliGenPHOSlib)

//======================================================================
//    P  I  O  N  S
//    (From GetPt, GetY and GetIp as param = Pion)
//    Transverse momentum distribution" PtPion 
//    Rapidity distribution YPion
//    Particle distribution IdPion  111, 211 and -211 (pi0, pi+ and pi-)
//
 Double_t AliGenPHOSlib::PtPion(Double_t *px, Double_t *)
{
//     Pion transverse momentum distribtuion taken 
//     from AliGenMUONlib class, version 3.01 of alirrot
//     PT-PARAMETERIZATION CDF, PRL 61(88) 1819
//     POWER LAW FOR PT > 500 MEV
//     MT SCALING BELOW (T=160 MEV)
//
  const Double_t p0 = 1.3;
  const Double_t xn = 8.28;
  const Double_t xlim=0.5;
  const Double_t t=0.160;
  const Double_t xmpi=0.139;
  const Double_t b=1.;
  Double_t y, y1, xmpi2, ynorm, a;
  Double_t x=*px;
  //
  y1=TMath::Power(p0/(p0+xlim),xn);
  xmpi2=xmpi*xmpi;
  ynorm=b*(TMath::Exp(-sqrt(xlim*xlim+xmpi2)/t));
  a=ynorm/y1;
  if (x > xlim)
    y=a*TMath::Power(p0/(p0+x),xn);
  else
    y=b*TMath::Exp(-sqrt(x*x+xmpi2)/t);
  return y*x;
}
//
// y-distribution
//
 Double_t AliGenPHOSlib::YPion( Double_t *py, Double_t *)
{
  const Double_t a    = 7000.;   
  const Double_t dy   = 4.;

  Double_t y=TMath::Abs(*py);
  //
  Double_t ex = y*y/(2*dy*dy);
  return a*TMath::Exp(-ex);
}
//                 particle composition pi+, pi0, pi-
//
 Int_t AliGenPHOSlib::IpPion()
{
    Float_t random[1];
    gMC->Rndm(random,1);

    if ( (3.*random[0])  < 1. ) 
    {
          return 211 ;
    } 
    else
    {  
      if ( (3.*random[0]) >= 2.)
      {
         return -211 ;
      }
      else 
      {
        return 111  ;
      }
    }
}
// End Pions
//============================================================= 
//
// Mt-scaling
// Fonction for the calculation of the Pt distribution for a 
// given particle np, from the pion Pt distribution using the 
// mt scaling. This function was taken from AliGenMUONlib 
// aliroot version 3.01, and was extended for baryons
// np = 1=>Pions 2=>Kaons 3=>Etas 4=>Omegas 5=>ETA' 6=>PHI
//      7=>BARYONS-BARYONBARS
 Double_t AliGenPHOSlib::PtScal(Double_t pt, Int_t np)
{
  //    SCALING EN MASSE PAR RAPPORT A PTPI
  //    MASS                1=>PI,  2=>K, 3=>ETA, 4=>OMEGA,  5=>ETA',6=>PHI
  const Double_t hm[10] = {0.1396, 0.494,  0.547,    0.782,   0.957,   1.02, 
  //    MASS               7=>BARYON-BARYONBAR  
                                         0.938, 0. , 0., 0.};
  //     VALUE MESON/PI AT 5 GEV
  const Double_t fmax[10]={1., 1., 1., 1., 1., 1., 1., 1., 1., 1.};
  np--;
  Double_t f5=TMath::Power(((sqrt(100.018215)+2.)/(sqrt(100.+hm[np]*hm[np])+2.0)),12.3);
  Double_t fmax2=f5/fmax[np];
  // PIONS
  Double_t ptpion=100.*PtPion(&pt, (Double_t*) 0);
  Double_t fmtscal=TMath::Power(((sqrt(pt*pt+0.018215)+2.)/
                                 (sqrt(pt*pt+hm[np]*hm[np])+2.0)),12.3)/ fmax2;
  return fmtscal*ptpion;
}
// End Scaling
//============================================================================
//    K  A  O  N  S
// kaon
//                pt-distribution
//____________________________________________________________
 Double_t AliGenPHOSlib::PtKaon( Double_t *px, Double_t *)
{
  return PtScal(*px,2);  //  2==> Kaon in the PtScal function
}

// y-distribution
//____________________________________________________________
 Double_t AliGenPHOSlib::YKaon( Double_t *py, Double_t *)
{
  const Double_t a    = 1000.;
  const Double_t dy   = 4.;


  Double_t y=TMath::Abs(*py);
  //
  Double_t ex = y*y/(2*dy*dy);
  return a*TMath::Exp(-ex);
}

//                 particle composition
//
 Int_t AliGenPHOSlib::IpKaon()
{
    Float_t random[1],random2[1];
    gMC->Rndm(random,1);
    gMC->Rndm(random2,1);
    if (random2[0] < 0.5) 
    {
      if (random[0] < 0.5) {       
        return  321;   //   K+
      } else {
        return -321;   // K-
      }
    }
    else
    {  
      if (random[0] < 0.5) {       
        return  311;   // K^0
      } else {  
        return -311;   // K^0 bar
      }
    }
}
// End Kaons
//============================================================================
//============================================================================
//   E  T  A  S
// etas
//                pt-distribution
//____________________________________________________________
 Double_t AliGenPHOSlib::PtEta( Double_t *px, Double_t *)
{
  return PtScal(*px,3);  //  3==> Eta in the PtScal function
}

// y-distribution
//____________________________________________________________
 Double_t AliGenPHOSlib::YEta( Double_t *py, Double_t *)
{
  const Double_t a    = 1000.;
  const Double_t dy   = 4.;


  Double_t y=TMath::Abs(*py);
  //
  Double_t ex = y*y/(2*dy*dy);
  return a*TMath::Exp(-ex);
}

//                 particle composition
//
 Int_t AliGenPHOSlib::IpEta()
{
        return  221;   //   eta
}
// End Etas
//============================================================================
//============================================================================
//    O  M  E  G  A  S
// omegas
//                pt-distribution
//____________________________________________________________
 Double_t AliGenPHOSlib::PtOmega( Double_t *px, Double_t *)
{
  return PtScal(*px,4);  //  4==> Omega in the PtScal function
}

// y-distribution
//____________________________________________________________
 Double_t AliGenPHOSlib::YOmega( Double_t *py, Double_t *)
{
  const Double_t a    = 1000.;
  const Double_t dy   = 4.;


  Double_t y=TMath::Abs(*py);
  //
  Double_t ex = y*y/(2*dy*dy);
  return a*TMath::Exp(-ex);
}

//                 particle composition
//
 Int_t AliGenPHOSlib::IpOmega()
{
        return  223;   // Omega
}
// End Omega
//============================================================================
//============================================================================
//    E  T  A  P  R  I  M  E
// etaprime
//                pt-distribution
//____________________________________________________________
 Double_t AliGenPHOSlib::PtEtaprime( Double_t *px, Double_t *)
{
  return PtScal(*px,5);  //  5==> Etaprime in the PtScal function
}

// y-distribution
//____________________________________________________________
 Double_t AliGenPHOSlib::YEtaprime( Double_t *py, Double_t *)
{
  const Double_t a    = 1000.;
  const Double_t dy   = 4.;


  Double_t y=TMath::Abs(*py);
  //
  Double_t ex = y*y/(2*dy*dy);
  return a*TMath::Exp(-ex);
}

//                 particle composition
//
 Int_t AliGenPHOSlib::IpEtaprime()
{
        return  331;   //   Etaprime
}
// End EtaPrime
//===================================================================
//============================================================================
//    P  H  I   S
// phi
//                pt-distribution
//____________________________________________________________
 Double_t AliGenPHOSlib::PtPhi( Double_t *px, Double_t *)
{
  return PtScal(*px,6);  //  6==> Phi in the PtScal function
}

// y-distribution
//____________________________________________________________
 Double_t AliGenPHOSlib::YPhi( Double_t *py, Double_t *)
{
  const Double_t a    = 1000.;
  const Double_t dy   = 4.;


  Double_t y=TMath::Abs(*py);
  //
  Double_t ex = y*y/(2*dy*dy);
  return a*TMath::Exp(-ex);
}

//                 particle composition
//
 Int_t AliGenPHOSlib::IpPhi()
{    
        return  333;   //   Phi      
}
// End Phis
//===================================================================
//============================================================================
//    B  A  R  Y  O  N  S  == protons, protonsbar, neutrons, and neutronsbars
// baryons
//                pt-distribution
//____________________________________________________________
 Double_t AliGenPHOSlib::PtBaryon( Double_t *px, Double_t *)
{
  return PtScal(*px,7);  //  7==> Baryon in the PtScal function
}

// y-distribution
//____________________________________________________________
 Double_t AliGenPHOSlib::YBaryon( Double_t *py, Double_t *)
{
  const Double_t a    = 1000.;
  const Double_t dy   = 4.;


  Double_t y=TMath::Abs(*py);
  //
  Double_t ex = y*y/(2*dy*dy);
  return a*TMath::Exp(-ex);
}

//                 particle composition
//
 Int_t AliGenPHOSlib::IpBaryon()
{
    Float_t random[1],random2[1];
    gMC->Rndm(random,1);
    gMC->Rndm(random2,1);
    if (random2[0] < 0.5) 
    {
      if (random[0] < 0.5) {       
        return  2212;   //   p
      } else {
        return -2212;   // pbar
      }
    }
    else
    {  
      if (random[0] < 0.5) {       
        return  2112;   // n
      } else {  
        return -2112;   // n bar
      }
    }
}
// End Baryons
//===================================================================


typedef Double_t (*GenFunc) (Double_t*,  Double_t*);
 GenFunc AliGenPHOSlib::GetPt(Param_t param)
{
    GenFunc func;
    
    switch (param)
    {
    case Pion:     
        func=PtPion;
        break;
    case Kaon:
        func=PtKaon;
        break;
    case Eta:
        func=PtEta;
        break;
    case Omega:
        func=PtOmega;
        break;
    case Etaprime:
        func=PtEtaprime;
        break;
    case Baryon:
        func=PtBaryon;
        break;
    default:
        func=0;
        printf("<AliGenPHOSlib::GetPt> unknown parametrisationn");
    }
    return func;
}

 GenFunc AliGenPHOSlib::GetY(Param_t param)
{
    GenFunc func;
    switch (param)
    {
    case Pion:
        func=YPion;
        break;
    case Kaon:
        func=YKaon;
        break;
    case Eta:
        func=YEta;
        break;
    case Omega:
        func=YOmega;
        break;
    case Etaprime:
        func=YEtaprime;
        break;
    case Phi:
        func=YPhi;
        break;
    case Baryon:
        func=YBaryon;
        break;
    default:
        func=0;
        printf("<AliGenPHOSlib::GetY> unknown parametrisationn");
    }
    return func;
}
typedef Int_t (*GenFuncIp) ();
 GenFuncIp AliGenPHOSlib::GetIp(Param_t param)
{
    GenFuncIp func;
    switch (param)
    {
    case Pion:
        
        func=IpPion;
        break;
    case Kaon:
        func=IpKaon;
        break;
    case Eta:
        func=IpEta;
        break;
    case Omega:
        func=IpOmega;
        break;
    case Etaprime:
        func=IpEtaprime;
        break;
    case Phi:
        func=IpPhi;
        break;
    case Baryon:
        func=IpBaryon;
        break;
    default:
        func=0;
        printf("<AliGenPHOSlib::GetIp> unknown parametrisationn");
    }
    return func;
}

