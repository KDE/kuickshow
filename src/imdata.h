/****************************************************************************
** $Id$
**
** Definition of something or other
**
** Created : 98
**
** Copyright (C) 1998, 1999 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/

#ifndef IMBLIBCONFIG_H
#define IMBLIBCONFIG_H

class KConfig;
class ImData
{
public:
  ImData();
  ~ImData() {};

  void 		load( KConfig *kc );
  void 		save( KConfig *kc );

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
