/**
 * $Id$
 *
 * Copyright 1998-2001 by Carsten Pfeiffer
 */

#ifdef index
#undef index
#endif
#include "kuickconfigdlg.h"

#include <qkeycode.h>
#include <qvbox.h>

#include <kaccel.h>
#include <kconfig.h>
#include <kglobal.h>
#include <klocale.h>

#include "imagewindow.h"
#include "defaultswidget.h"
#include "generalwidget.h"
#include "slideshowwidget.h"

#include "kuickdata.h"


KuickConfigDialog::KuickConfigDialog( KActionCollection *_coll, QWidget *parent,
				      const char *name, bool modal )
    : LogoTabDialog( KJanusWidget::Tabbed, i18n("KuickShow Configuration"),
		     Help | Default | Ok | Apply | Cancel, Ok,
		     parent, name, modal )
{
    coll = _coll;
    QVBox *box = addVBoxPage( i18n("&General") );
    generalWidget = new GeneralWidget( box, "general widget" );

    box = addVBoxPage( i18n("&Modifications") );
    defaultsWidget = new DefaultsWidget( box, "defaults widget" );

    box = addVBoxPage( i18n("&Slideshow") );
    slideshowWidget = new SlideShowWidget( box, "slideshow widget" );

    box = addVBoxPage( i18n("&Viewer Shortcuts") );

    imageWindow = new ImageWindow(); // just to get the accel...
    imageWindow->hide();

    imageKeyChooser = new KKeyChooser( imageWindow->accel(), box );

    box = addVBoxPage( i18n("Bro&wser Shortcuts") );
    browserKeyChooser = new KKeyChooser( coll, box );

    connect( this, SIGNAL( defaultClicked() ), SLOT( resetDefaults() ));
}

KuickConfigDialog::~KuickConfigDialog()
{
    imageWindow->close( true );
}

void KuickConfigDialog::applyConfig()
{
    generalWidget->applySettings( *kdata );
    defaultsWidget->applySettings( *kdata );
    slideshowWidget->applySettings( *kdata );

    imageKeyChooser->commitChanges();
    imageWindow->accel()->writeSettings();

    browserKeyChooser->commitChanges();
    coll->writeShortcutSettings();

    KGlobal::config()->sync();
}


void KuickConfigDialog::resetDefaults()
{
    KuickData data;
    generalWidget->loadSettings( data );
    defaultsWidget->loadSettings( data );
    slideshowWidget->loadSettings( data );

    imageKeyChooser->allDefault();
    browserKeyChooser->allDefault();
}

#include "kuickconfigdlg.moc"
