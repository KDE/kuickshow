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
#include "kuickdata.h"


KuickConfigDialog::KuickConfigDialog( KAccel *browserAccel, QWidget *parent,
				      const char *name, bool modal )
    : LogoTabDialog( KJanusWidget::Tabbed, i18n("KuickShow Configuration"),
		     Help | Default | Ok | Apply | Cancel, Ok,
		     parent, name, modal )
{
    m_browserAccel = browserAccel;
    QVBox *box = addVBoxPage( i18n("General") );
    generalWidget = new GeneralWidget( box, "general widget" );

    box = addVBoxPage( i18n("Modifications") );
    defaultsWidget = new DefaultsWidget( box, "defaults widget" );

    box = addVBoxPage( i18n("Viewer Shortcuts") );

    imageWindow = new ImageWindow(); // just to get the accel...
    imageWindow->hide();

    //m_imageKeys = imageWindow->accel()->keyDict();
    //imageKeyChooser = new KKeyChooser( &m_imageKeys, box );
    imageKeyChooser = new KKeyChooser( imageWindow->accel()->actions(), box );

    box = addVBoxPage( i18n("Browser Shortcuts") );
    //m_browserKeys = browserAccel->keyDict();
    //browserKeyChooser = new KKeyChooser( &m_browserKeys, box );
    browserKeyChooser = new KKeyChooser( browserAccel->actions(), box );

    connect( this, SIGNAL( defaultClicked() ), SLOT( resetDefaults() ));
}

KuickConfigDialog::~KuickConfigDialog()
{
    imageWindow->close( true );
}

void KuickConfigDialog::applyConfig()
{
    generalWidget->applySettings();
    defaultsWidget->applySettings();

    //KAccel *accel = imageWindow->accel();
    //accel->setKeyDict( m_imageKeys );
    imageKeyChooser->commitChanges();
    imageWindow->accel()->writeSettings();

    //m_browserAccel->setKeyDict( m_browserKeys );
    browserKeyChooser->commitChanges();
    m_browserAccel->writeSettings();

    KGlobal::config()->sync();
}


void KuickConfigDialog::resetDefaults()
{
    generalWidget->resetDefaults();
    defaultsWidget->resetDefaults();
    imageKeyChooser->allDefault();
    browserKeyChooser->allDefault();
}

#include "kuickconfigdlg.moc"
