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

// values are also used as combobox index defaultswidget.*
enum Rotation { ROT_0=0, ROT_90=1, ROT_180=2, ROT_270=3 };

class KuickData
{
public:
    KuickData();
    ~KuickData();

    void 	load();
    void 	save();


    ImData 	*idata;

    QString 	fileFilter;
    uint 	slideDelay;
    uint        slideshowCycles;
    bool        slideshowFullscreen :1;

    int 	brightnessSteps;
    int 	contrastSteps;
    int 	gammaSteps;

    int 	scrollSteps;
    float	zoomSteps;

    bool 	preloadImage     :1;
    bool 	autoRotation     :1;
    bool 	fullScreen       :1;

    // default image modifications
    bool 	isModsEnabled :1;

    bool 	flipVertically   :1;
    bool 	flipHorizontally :1;
    bool 	downScale        :1;
    bool 	upScale          :1;
    int 	maxUpScale;
    uint 	maxWidth, maxHeight;
    uint        maxCachedImages;
    Rotation 	rotation;

    QColor      backgroundColor;

};


extern KuickData* kdata;

#endif
