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

#ifndef KUICKDATA_H
#define KUICKDATA_H

#include <qcolor.h>
#include <qstring.h>

#include "imdata.h"

class KConfig;

// values are also used as combobox index defaultswidget.*
enum Rotation { ROT_0=0, ROT_90=1, ROT_180=2, ROT_270=3 };

// hmm, global declaration for now
enum FlipMode { FlipNone = 0, FlipHorizontal = 1, FlipVertical = 2 };

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
    bool        slideshowStartAtFirst :1;

    int 	brightnessSteps;
    int 	contrastSteps;
    int 	gammaSteps;

    int 	scrollSteps;
    float	zoomSteps;

    bool startInLastDir      :1;

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
    float 	maxZoomFactor;
    uint        maxCachedImages;
    Rotation 	rotation;

    QColor      backgroundColor;


};


extern KuickData* kdata;

#endif
