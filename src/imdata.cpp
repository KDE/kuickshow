/****************************************************************************
** $Id$
**
** ImData, a class containing/loading/saving some preferences of KuickShow
**
** Created : 98
**
** Copyright (C) 1998-2001 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/

#include <stdlib.h>

#include <kconfig.h>

#include "imdata.h"


ImData::ImData()
{
  ownPalette     = true;
  fastRemap      = true;
  fastRender  	 = true;
  dither16bit    = false;
  dither8bit     = true;
  maxCache       = 10240;

  gamma          = 0;
  brightness     = 0;
  contrast       = 0;

  gammaFactor      = 10;
  brightnessFactor = 10;
  contrastFactor   = 10;
}


void ImData::load( KConfig *kc )
{
  ImData def;

  kc->setGroup( "ImlibConfiguration" );

  ownPalette  = kc->readBoolEntry( "UseOwnPalette", def.ownPalette );
  fastRemap   = kc->readBoolEntry( "FastRemapping", def.fastRemap );
  fastRender  = kc->readBoolEntry( "FastRendering", def.fastRender );
  dither16bit = kc->readBoolEntry( "Dither16Bit", def.dither16bit );
  dither8bit  = kc->readBoolEntry( "Dither8Bit", def.dither8bit );

  maxCache    = kc->readNumEntry( "MaxCacheSize", 10240 );

  gamma       = kc->readNumEntry( "GammaDefault", 0 );
  brightness  = kc->readNumEntry( "BrightnessDefault", 0 );
  contrast    = kc->readNumEntry( "ContrastDefault", 0 );

  gammaFactor      = abs( kc->readNumEntry( "GammaFactor", 10 ) );
  brightnessFactor = abs( kc->readNumEntry( "BrightnessFactor", 10 ) );
  contrastFactor   = abs( kc->readNumEntry( "ContrastFactor", 10 ) );
}


void ImData::save( KConfig *kc )
{
  kc->setGroup( "ImlibConfiguration" );

  kc->writeEntry( "UseOwnPalette", ownPalette );
  kc->writeEntry( "FastRemapping", fastRemap );
  kc->writeEntry( "FastRendering", fastRender );
  kc->writeEntry( "Dither16Bit", dither16bit );
  kc->writeEntry( "Dither8Bit", dither8bit );
  kc->writeEntry( "MaxCacheSize", maxCache );

  kc->writeEntry( "GammaDefault", gamma );
  kc->writeEntry( "BrightnessDefault", brightness );
  kc->writeEntry( "ContrastDefault", contrast );

  kc->writeEntry( "GammaFactor", gammaFactor );
  kc->writeEntry( "BrightnessFactor", brightnessFactor );
  kc->writeEntry( "ContrastFactor", contrastFactor );

  kc->sync();
}
