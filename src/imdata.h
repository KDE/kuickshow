#include <ksharedconfig.h>
/* This file is part of the KDE project
   Copyright (C) 1998,1999,2000,2001 Carsten Pfeiffer <pfeiffer@kde.org>

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

#ifndef IMBLIBCONFIG_H
#define IMBLIBCONFIG_H

class KConfigBase;
class ImData
{
public:
  ImData();
  ~ImData() {};

  void 		load( KSharedConfig::Ptr kc );
  void 		save( KSharedConfig::Ptr kc );

  // new stuff..........

  int		gamma;
  int 		brightness;
  int 		contrast;

  // -----------------------

  bool 		ownPalette;
  bool 		fastRemap;
  bool 		fastRender;
  bool		dither16bit;
  bool 		dither8bit;

  uint          gammaFactor;
  uint          brightnessFactor;
  uint          contrastFactor;

  uint          maxCache;

};


#endif
