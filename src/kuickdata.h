/****************************************************************************
** $Id$
**
** KuickData, a class containing/loading/saving  all preferences of KuickShow
**
** Created : 98
**
** Copyright (C) 1998, 1999 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/

#ifndef KUICKDATA_H
#define KUICKDATA_H

#include <qcolor.h>
#include <qstring.h>

#include "imdata.h"

class KConfig;

class KuickData
{
public:
    KuickData();
    ~KuickData();

    void 	load();
    void 	save();


    QString 	fileFilter;
    uint 	slideDelay;
    bool 	preloadImage;
    bool 	showInOneWindow;
    ImData 	*idata;
    bool 	fullScreen;


    int 	brightnessSteps;
    int 	contrastSteps;
    int 	gammaSteps;

    int 	scrollSteps;
    float	zoomSteps;


    // default image modifications
    bool 	isModsEnabled;

    bool 	downScale;
    bool 	upScale;
    int 	maxUpScale;
    uint 	maxWidth, maxHeight;


    bool 	flipVertically;
    bool 	flipHorizontally;
    int 	rotation;

    QColor      backgroundColor;

};


extern KuickData* kdata;

#endif
