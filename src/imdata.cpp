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

#include "imdata.h"

#include <ksharedconfig.h>
#include <kconfiggroup.h>


ImData::ImData()
{
  ownPalette     = true;
  fastRemap      = true;
  fastRender  	 = true;
  dither16bit    = false;
  dither8bit     = true;
  smoothScale    = false;
  maxCache       = 10240;

  gamma          = 0;
  brightness     = 0;
  contrast       = 0;

  gammaFactor      = 10;
  brightnessFactor = 10;
  contrastFactor   = 10;
}


void ImData::load()
{
  KSharedConfig::Ptr kc = KSharedConfig::openConfig();
  KConfigGroup group( kc, "ImlibConfiguration" );
  ImData def;

  ownPalette  = group.readEntry( "UseOwnPalette", def.ownPalette );
  fastRemap   = group.readEntry( "FastRemapping", def.fastRemap );
  fastRender  = group.readEntry( "FastRendering", def.fastRender );
  dither16bit = group.readEntry( "Dither16Bit", def.dither16bit );
  dither8bit  = group.readEntry( "Dither8Bit", def.dither8bit );
  smoothScale = group.readEntry( "SmoothScaling", def.smoothScale );

  maxCache    = group.readEntry( "MaxCacheSize", def.maxCache);

  gamma       = group.readEntry( "GammaDefault", def.gamma);
  brightness  = group.readEntry( "BrightnessDefault", def.brightness);
  contrast    = group.readEntry( "ContrastDefault", def.contrast);

  gammaFactor      = group.readEntry( "GammaFactor", def.gammaFactor);
  brightnessFactor = group.readEntry( "BrightnessFactor", def.brightnessFactor);
  contrastFactor   = group.readEntry( "ContrastFactor", def.contrastFactor);
}


void ImData::save()
{
  KSharedConfig::Ptr kc = KSharedConfig::openConfig();
  KConfigGroup group( kc, "ImlibConfiguration" );

  group.writeEntry( "UseOwnPalette", ownPalette );
  group.writeEntry( "FastRemapping", fastRemap );
  group.writeEntry( "FastRendering", fastRender );
  group.writeEntry( "Dither16Bit", dither16bit );
  group.writeEntry( "Dither8Bit", dither8bit );
  group.writeEntry( "MaxCacheSize", maxCache );
  group.writeEntry( "SmoothScaling", smoothScale );

  group.writeEntry( "GammaDefault", gamma );
  group.writeEntry( "BrightnessDefault", brightness );
  group.writeEntry( "ContrastDefault", contrast );

  group.writeEntry( "GammaFactor", gammaFactor );
  group.writeEntry( "BrightnessFactor", brightnessFactor );
  group.writeEntry( "ContrastFactor", contrastFactor );

  kc->sync();
}
