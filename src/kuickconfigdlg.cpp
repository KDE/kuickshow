/* This file is part of the KDE project
   Copyright (C) 1998-2002 Carsten Pfeiffer <pfeiffer@kde.org>

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
    : KDialogBase( Tabbed, i18n("Configure"),
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

    imageKeyChooser = new KKeyChooser( imageWindow->actionCollection(), box );

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

    imageKeyChooser->save();
    browserKeyChooser->save();

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
