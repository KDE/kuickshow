/****************************************************************************
** $Id$
**
** Configuration dialog for KuickShow
**
** Created : 98
**
** Copyright (C) 1998-2001 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/

#ifndef KUICKCONFIGDLG_H
#define KUICKCONFIGDLG_H

#include <qevent.h>

#include <kkeydialog.h>

#include "logotabdialog.h"

class GeneralWidget;
class DefaultsWidget;
class SlideShowWidget;
class ImageWindow;

class KuickConfigDialog : public LogoTabDialog
{
    Q_OBJECT

public:
    KuickConfigDialog( KActionCollection *coll, QWidget *parent=0,
		       const char *name=0, bool modal=true);
    ~KuickConfigDialog();

    void 		applyConfig();

private slots:
    void 		resetDefaults();

private:
    DefaultsWidget   *defaultsWidget;
    GeneralWidget    *generalWidget;
    SlideShowWidget  *slideshowWidget;
    KKeyChooser      *imageKeyChooser, *browserKeyChooser;
    KActionCollection *coll;

    ImageWindow      *imageWindow;

};

#endif
