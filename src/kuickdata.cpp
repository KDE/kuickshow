/* This file is part of the KDE project
   Copyright (C) 1998-2003 Carsten Pfeiffer <pfeiffer@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <stdlib.h>

#include <qcolor.h>
#include <kconfig.h>
#include <kglobal.h>

#include "kuickdata.h"


KuickData::KuickData()
{
  fileFilter  = "*.jpeg *.jpg *.gif *.xpm *.ppm *.pgm *.pbm *.pnm *.png *.bmp *.psd *.eim *.tif *.tiff *.xcf";// *.mng";
  slideDelay       = 3000;
  slideshowCycles  = 1;
  slideshowFullscreen = true;
  slideshowStartAtFirst = true;

  preloadImage     = true;

  isModsEnabled    = true;
  fullScreen       = false;
  autoRotation     = true;
  downScale 	   = true;
  upScale 	   = false;
  flipVertically   = false;
  flipHorizontally = false;

  maxUpScale       = 3;
  rotation         = ROT_0;

  brightnessSteps = 1;
  contrastSteps   = 1;
  gammaSteps      = 1;
  scrollSteps     = 1;
  zoomSteps       = 1.5;

  maxWidth 	  = 8192;
  maxHeight 	  = 8192;

  maxCachedImages = 4;
  backgroundColor = Qt::black;

  startInLastDir = true;

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
  slideDelay   = kc->readEntry( "SlideShowDelay", def.slideDelay );
  slideshowCycles = kc->readEntry( "SlideshowCycles", 1 );
  slideshowFullscreen = kc->readEntry( "SlideshowFullscreen", true );
  slideshowStartAtFirst = kc->readEntry("SlideshowStartAtFirst", true );

  preloadImage = kc->readEntry( "PreloadNextImage", def.preloadImage );

  fullScreen = kc->readEntry( "Fullscreen", def.fullScreen);
  autoRotation = kc->readEntry( "AutoRotation", def.autoRotation);
  downScale  = kc->readEntry( "ShrinkToScreenSize", def.downScale );
  upScale    = kc->readEntry( "ZoomToScreenSize", def.upScale );
  flipVertically   = kc->readEntry( "FlipVertically", def.flipVertically );
  flipHorizontally = kc->readEntry( "FlipHorizontally",
					def.flipHorizontally );
  maxUpScale       = kc->readEntry( "MaxUpscale Factor", def.maxUpScale );
  rotation         = (Rotation) kc->readEntry( "Rotation", int(def.rotation) );

  isModsEnabled    = kc->readEntry( "ApplyDefaultModifications",
					def.isModsEnabled );

  brightnessSteps = kc->readEntry("BrightnessStepSize",def.brightnessSteps);
  contrastSteps   = kc->readEntry("ContrastStepSize", def.contrastSteps);
  gammaSteps      = kc->readEntry("GammaStepSize", def.gammaSteps);
  scrollSteps     = kc->readEntry("ScrollingStepSize", def.scrollSteps);
  zoomSteps       = kc->readEntry("ZoomStepSize", (double)def.zoomSteps);


  maxWidth 	= abs( kc->readEntry( "MaximumImageWidth", def.maxWidth ) );
  maxHeight 	= abs( kc->readEntry( "MaximumImageHeight", def.maxHeight));

  maxCachedImages = kc->readEntry( "MaxCachedImages",
                                              def.maxCachedImages );
  QColor _col(Qt::black);
  backgroundColor = kc->readEntry( "BackgroundColor", _col );

  startInLastDir = kc->readEntry( "StartInLastDir", true);

  idata->load( kc );

  // compatibility with KuickShow <= 0.8.3
  switch ( rotation )
  {
      case 90:
          rotation = ROT_90;
          break;
      case 180:
          rotation = ROT_180;
          break;
      case 270:
          rotation = ROT_270;
          break;
      default:
          if ( (rotation < ROT_0) || (rotation > ROT_270) )
              rotation = ROT_0;
          break;
  }
}


void KuickData::save()
{
  KConfig *kc = KGlobal::config();
  kc->setGroup( "GeneralConfiguration" );

  kc->writeEntry( "FileFilter", fileFilter );
  kc->writeEntry( "SlideShowDelay", slideDelay );
  kc->writeEntry( "SlideshowCycles", slideshowCycles );
  kc->writeEntry( "SlideshowFullscreen", slideshowFullscreen );
  kc->writeEntry( "SlideshowStartAtFirst", slideshowStartAtFirst );

  kc->writeEntry( "PreloadNextImage", preloadImage );

  kc->writeEntry( "Fullscreen", fullScreen  );
  kc->writeEntry( "AutoRotation", autoRotation  );
  kc->writeEntry( "ShrinkToScreenSize", downScale );
  kc->writeEntry( "ZoomToScreenSize", upScale );
  kc->writeEntry( "FlipVertically", flipVertically );
  kc->writeEntry( "FlipHorizontally", flipHorizontally );
  kc->writeEntry( "MaxUpscale Factor", maxUpScale );
  kc->writeEntry( "Rotation", int(rotation) );

  kc->writeEntry( "ApplyDefaultModifications", isModsEnabled );


  kc->writeEntry( "BrightnessStepSize", brightnessSteps );
  kc->writeEntry( "ContrastStepSize", contrastSteps );
  kc->writeEntry( "GammaStepSize", gammaSteps );

  kc->writeEntry( "ScrollingStepSize", scrollSteps );
  kc->writeEntry( "ZoomStepSize", int(zoomSteps) );

  kc->writeEntry( "MaximumImageWidth", maxWidth );
  kc->writeEntry( "MaximumImageHeight", maxHeight );

  kc->writeEntry( "MaxCachedImages", maxCachedImages );
  kc->writeEntry( "BackgroundColor", backgroundColor );

  kc->writeEntry( "StartInLastDir", startInLastDir );

  idata->save( kc );

  kc->sync();
}
