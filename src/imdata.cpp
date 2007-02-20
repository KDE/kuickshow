/* This file is part of the KDE project
   Copyright (C) 1998-2001 Carsten Pfeiffer <pfeiffer@kde.org>

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


void ImData::load( KConfigBase *kc )
{
  ImData def;

  kc->setGroup( "ImlibConfiguration" );

  ownPalette  = kc->readEntry( "UseOwnPalette", def.ownPalette );
  fastRemap   = kc->readEntry( "FastRemapping", def.fastRemap );
  fastRender  = kc->readEntry( "FastRendering", def.fastRender );
  dither16bit = kc->readEntry( "Dither16Bit", def.dither16bit );
  dither8bit  = kc->readEntry( "Dither8Bit", def.dither8bit );

  maxCache    = kc->readEntry( "MaxCacheSize", 10240 );

  gamma       = kc->readEntry( "GammaDefault", 0 );
  brightness  = kc->readEntry( "BrightnessDefault", 0 );
  contrast    = kc->readEntry( "ContrastDefault", 0 );

  gammaFactor      = abs( kc->readEntry( "GammaFactor", 10 ) );
  brightnessFactor = abs( kc->readEntry( "BrightnessFactor", 10 ) );
  contrastFactor   = abs( kc->readEntry( "ContrastFactor", 10 ) );
}


void ImData::save( KConfigBase *kc )
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
