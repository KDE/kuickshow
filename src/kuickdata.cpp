/****************************************************************************
** $Id$
**
** KuickData, a class containing/loading/saving  all preferences of KuickShow
**
** Created : 98
**
** Copyright (C) 1998-2001 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/

#include <stdlib.h>

#include <qcolor.h>
#include <kconfig.h>
#include <kglobal.h>

#include "kuickdata.h"


KuickData::KuickData()
{
  fileFilter  = "*.jpeg *.jpg *.gif *.xpm *.ppm *.pgm *.png *.bmp *.psd *.eim *.tiff *.xcf";// *.mng";
  slideDelay       = 3000;
  preloadImage     = true;

  isModsEnabled    = true;
  fullScreen       = false;
  downScale 	   = true;
  upScale 	   = false;
  flipVertically   = false;
  flipHorizontally = false;

  maxUpScale       = 3;
  rotation         = 0;

  brightnessSteps = 1;
  contrastSteps   = 1;
  gammaSteps      = 1;
  scrollSteps     = 1;
  zoomSteps       = 1.5;

  maxWidth 	  = 8192;
  maxHeight 	  = 8192;

  backgroundColor = Qt::black;

  idata = new ImData;
}

KuickData::~KuickData()
{
  delete idata;
}


void KuickData::load()
{
  KConfig *kc = KGlobal::config();

  KuickData def;

  kc->setGroup( "GeneralConfiguration" );
  fileFilter   = kc->readEntry( "FileFilter", def.fileFilter );
  slideDelay   = kc->readNumEntry( "SlideShowDelay", def.slideDelay );
  preloadImage = kc->readBoolEntry( "PreloadNextImage", def.preloadImage );

  fullScreen = kc->readBoolEntry( "Fullscreen", def.fullScreen);
  downScale  = kc->readBoolEntry( "ShrinkToScreenSize", def.downScale );
  upScale    = kc->readBoolEntry( "ZoomToScreenSize", def.upScale );
  flipVertically   = kc->readBoolEntry( "FlipVertically", def.flipVertically );
  flipHorizontally = kc->readBoolEntry( "FlipHorizontally",
					def.flipHorizontally );
  maxUpScale       = kc->readNumEntry( "MaxUpscale Factor", def.maxUpScale );
  rotation         = kc->readNumEntry( "Rotation", def.rotation );

  isModsEnabled    = kc->readBoolEntry( "ApplyDefaultModifications",
					def.isModsEnabled );

  brightnessSteps = kc->readNumEntry("BrightnessStepSize",def.brightnessSteps);
  contrastSteps   = kc->readNumEntry("ContrastStepSize", def.contrastSteps);
  gammaSteps      = kc->readNumEntry("GammaStepSize", def.gammaSteps);
  scrollSteps     = kc->readNumEntry("ScrollingStepSize", def.scrollSteps);
  zoomSteps       = kc->readDoubleNumEntry("ZoomStepSize", def.zoomSteps);


  maxWidth 	= abs( kc->readNumEntry( "MaximumImageWidth", def.maxWidth ) );
  maxHeight 	= abs( kc->readNumEntry( "MaximumImageHeight", def.maxHeight));

  backgroundColor = kc->readColorEntry( "BackgroundColor", &Qt::black );

  idata->load( kc );
}


void KuickData::save()
{
  KConfig *kc = KGlobal::config();
  kc->setGroup( "GeneralConfiguration" );

  kc->writeEntry( "FileFilter", fileFilter );
  kc->writeEntry( "SlideShowDelay", slideDelay );
  kc->writeEntry( "PreloadNextImage", preloadImage ? "yes" : "no" );

  kc->writeEntry( "Fullscreen", fullScreen ? "yes" : "no" );
  kc->writeEntry( "ShrinkToScreenSize", downScale ? "yes" : "no" );
  kc->writeEntry( "ZoomToScreenSize", upScale ? "yes" : "no" );
  kc->writeEntry( "FlipVertically", flipVertically );
  kc->writeEntry( "FlipHorizontally", flipHorizontally );
  kc->writeEntry( "MaxUpscale Factor", maxUpScale );
  kc->writeEntry( "Rotation", rotation );

  kc->writeEntry( "ApplyDefaultModifications", isModsEnabled ? "yes" : "no" );


  kc->writeEntry( "BrightnessStepSize", brightnessSteps );
  kc->writeEntry( "ContrastStepSize", contrastSteps );
  kc->writeEntry( "GammaStepSize", gammaSteps );

  kc->writeEntry( "ScrollingStepSize", scrollSteps );
  kc->writeEntry( "ZoomStepSize", zoomSteps );

  kc->writeEntry( "MaximumImageWidth", maxWidth );
  kc->writeEntry( "MaximumImageHeight", maxHeight );

  kc->writeEntry( "BackgroundColor", backgroundColor );
  
  idata->save( kc );

  kc->sync();
}
