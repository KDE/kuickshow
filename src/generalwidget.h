/**
 * $Id$
 *
 * Copyright 1998-2001 by Carsten Pfeiffer <pfeiffer@kde.org>
 */

#ifndef GENERALWIDGET_H
#define GENERALWIDGET_H

class QCheckBox;
class KColorButton;
class KLineEdit;
class KIntNumInput;

#include "basewidget.h"

class KuickData;

class GeneralWidget : public BaseWidget
{
    Q_OBJECT

public:
    GeneralWidget( QWidget *parent, const char *name );
    ~GeneralWidget();

    void 	loadSettings();
    void 	applySettings();
    void 	resetDefaults();

private:
    void 	init( const KuickData& );

    QCheckBox 	*cbFullscreen, *cbActiveWindow, *cbPreload;
    QCheckBox   *cbFastRemap, *cbFastRender;
    QCheckBox 	*cbDither16bit, *cbDither8bit, *cbOwnPalette;

    KLineEdit   	*editFilter;
    KIntNumInput 	*delaySpinBox, *maxCacheSpinBox;

    KIntNumInput 	*sbMaxWidth, *sbMaxHeight;
    KIntNumInput 	*sbZoomFactor;

    KColorButton        *colorButton;
    
private slots:
    void 	useOwnPalette();

};

#endif
