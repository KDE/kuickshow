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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
 */

#ifndef GENERALWIDGET_H
#define GENERALWIDGET_H

#include <qwidget.h>

#include "kuickdata.h"

class QCheckBox;
class KColorButton;
class KLineEdit;
class KIntNumInput;
class QSlider;

class GeneralWidget : public QWidget
{
    Q_OBJECT

public:
    GeneralWidget( QWidget *parent, const char *name );
    ~GeneralWidget();

    void 	loadSettings( const KuickData& data );
    void 	applySettings( KuickData& data );

private:
    QCheckBox 	*cbFullscreen, *cbActiveWindow, *cbPreload;
    QCheckBox   *cbFastRemap, *cbFastRender;
    QCheckBox 	*cbDither16bit, *cbDither8bit, *cbOwnPalette;

    QSlider	*quality;
    
    KLineEdit   	*editFilter;
    KIntNumInput 	*delaySpinBox, *maxCacheSpinBox;

    KIntNumInput 	*sbMaxWidth, *sbMaxHeight;
    KIntNumInput 	*sbZoomFactor;

    KColorButton        *colorButton;

private slots:
    void 	useOwnPalette();
    void slotURLClicked( const QString & );

};

#endif
