/**
 * $Id$
 *
 * Copyright 1998-2001 by Carsten Pfeiffer <pfeiffer@kde.org>
 */

#ifndef GENERALWIDGET_H
#define GENERALWIDGET_H

#include <qwidget.h>

#include "kuickdata.h"

class QCheckBox;
class KColorButton;
class KLineEdit;
class KIntNumInput;


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
