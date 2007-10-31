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
#include <ksharedconfig.h>
#include <kglobal.h>
#include <kconfiggroup.h>

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
  KSharedConfig::Ptr kc = KGlobal::config();

  KuickData def;

  KConfigGroup generalGroup( kc, "GeneralConfiguration" );
  fileFilter   = generalGroup.readEntry( "FileFilter", def.fileFilter );
  slideDelay   = generalGroup.readEntry( "SlideShowDelay", def.slideDelay );
  slideshowCycles = generalGroup.readEntry( "SlideshowCycles", 1 );
  slideshowFullscreen = generalGroup.readEntry( "SlideshowFullscreen", true );
  slideshowStartAtFirst = generalGroup.readEntry("SlideshowStartAtFirst", true );

  preloadImage = generalGroup.readEntry( "PreloadNextImage", def.preloadImage );

  fullScreen = generalGroup.readEntry( "Fullscreen", def.fullScreen);
  autoRotation = generalGroup.readEntry( "AutoRotation", def.autoRotation);
  downScale  = generalGroup.readEntry( "ShrinkToScreenSize", def.downScale );
  upScale    = generalGroup.readEntry( "ZoomToScreenSize", def.upScale );
  flipVertically   = generalGroup.readEntry( "FlipVertically", def.flipVertically );
  flipHorizontally = generalGroup.readEntry( "FlipHorizontally",
					def.flipHorizontally );
  maxUpScale       = generalGroup.readEntry( "MaxUpscale Factor", def.maxUpScale );
  rotation         = (Rotation) generalGroup.readEntry( "Rotation", int(def.rotation) );

  isModsEnabled    = generalGroup.readEntry( "ApplyDefaultModifications",
					def.isModsEnabled );

  brightnessSteps = generalGroup.readEntry("BrightnessStepSize",def.brightnessSteps);
  contrastSteps   = generalGroup.readEntry("ContrastStepSize", def.contrastSteps);
  gammaSteps      = generalGroup.readEntry("GammaStepSize", def.gammaSteps);
  scrollSteps     = generalGroup.readEntry("ScrollingStepSize", def.scrollSteps);
  zoomSteps       = generalGroup.readEntry("ZoomStepSize", (double)def.zoomSteps);


  maxWidth 	= abs( generalGroup.readEntry( "MaximumImageWidth", def.maxWidth ) );
  maxHeight 	= abs( generalGroup.readEntry( "MaximumImageHeight", def.maxHeight));

  maxCachedImages = generalGroup.readEntry( "MaxCachedImages",
                                              def.maxCachedImages );
  QColor _col(Qt::black);
  backgroundColor = generalGroup.readEntry( "BackgroundColor", _col );

  startInLastDir = generalGroup.readEntry( "StartInLastDir", true);

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
  KSharedConfig::Ptr kc = KGlobal::config();

  KConfigGroup generalGroup(kc, "GeneralConfiguration");

  generalGroup.writeEntry( "FileFilter", fileFilter );
  generalGroup.writeEntry( "SlideShowDelay", slideDelay );
  generalGroup.writeEntry( "SlideshowCycles", slideshowCycles );
  generalGroup.writeEntry( "SlideshowFullscreen", slideshowFullscreen );
  generalGroup.writeEntry( "SlideshowStartAtFirst", slideshowStartAtFirst );

  generalGroup.writeEntry( "PreloadNextImage", preloadImage );

  generalGroup.writeEntry( "Fullscreen", fullScreen  );
  generalGroup.writeEntry( "AutoRotation", autoRotation  );
  generalGroup.writeEntry( "ShrinkToScreenSize", downScale );
  generalGroup.writeEntry( "ZoomToScreenSize", upScale );
  generalGroup.writeEntry( "FlipVertically", flipVertically );
  generalGroup.writeEntry( "FlipHorizontally", flipHorizontally );
  generalGroup.writeEntry( "MaxUpscale Factor", maxUpScale );
  generalGroup.writeEntry( "Rotation", int(rotation) );

  generalGroup.writeEntry( "ApplyDefaultModifications", isModsEnabled );


  generalGroup.writeEntry( "BrightnessStepSize", brightnessSteps );
  generalGroup.writeEntry( "ContrastStepSize", contrastSteps );
  generalGroup.writeEntry( "GammaStepSize", gammaSteps );

  generalGroup.writeEntry( "ScrollingStepSize", scrollSteps );
  generalGroup.writeEntry( "ZoomStepSize", int(zoomSteps) );

  generalGroup.writeEntry( "MaximumImageWidth", maxWidth );
  generalGroup.writeEntry( "MaximumImageHeight", maxHeight );

  generalGroup.writeEntry( "MaxCachedImages", maxCachedImages );
  generalGroup.writeEntry( "BackgroundColor", backgroundColor );

  generalGroup.writeEntry( "StartInLastDir", startInLastDir );

  idata->save( kc );

  kc->sync();
}
