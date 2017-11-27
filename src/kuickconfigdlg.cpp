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

#include "kuickconfigdlg.h"

#include <KConfig>
#include <KGlobal>
#include <KLocale>
#include <KShortcutsDialog>
#include <KVBox>

#include <QPushButton>
#include <qnamespace.h>

#include "defaultswidget.h"
#include "generalwidget.h"
#include "imagewindow.h"
#include "kuickdata.h"
#include "slideshowwidget.h"


KuickConfigDialog::KuickConfigDialog( KActionCollection *_coll, QWidget *parent, bool modal )
    : KPageDialog( parent )
{
    setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Help);
    setModal( modal );
    setWindowTitle( i18n("Configure") );
    setFaceType( Tabbed );
    coll = _coll;
    KVBox *box = new KVBox();
    addPage( box, i18n("&General") );
    generalWidget = new GeneralWidget( box );
    generalWidget->setObjectName( QString::fromLatin1( "general widget" ) );

    box = new KVBox();
    addPage( box, i18n("&Modifications") );
    defaultsWidget = new DefaultsWidget( box );
    defaultsWidget->setObjectName( QString::fromLatin1( "defaults widget" ) );

    box = new KVBox();
    addPage( box, i18n("&Slideshow")  );
    slideshowWidget = new SlideShowWidget( box );
    slideshowWidget->setObjectName( QString::fromLatin1( "slideshow widget" ) );

    box = new KVBox();
    addPage( box, i18n("&Viewer Shortcuts") );

    imageWindow = new ImageWindow(); // just to get the accel...
    imageWindow->hide();

    imageKeyChooser = new KShortcutsEditor( imageWindow->actionCollection(), box );

    box = new KVBox();
    addPage( box, i18n("Bro&wser Shortcuts") );
    browserKeyChooser = new KShortcutsEditor( coll, box );

    connect(buttonBox()->button(QDialogButtonBox::RestoreDefaults), SIGNAL(clicked()), SLOT(resetDefaults()));
    connect(buttonBox()->button(QDialogButtonBox::Ok), SIGNAL(clicked()), SIGNAL(okClicked()));
    connect(buttonBox()->button(QDialogButtonBox::Apply), SIGNAL(clicked()), SIGNAL(applyClicked()));
}

KuickConfigDialog::~KuickConfigDialog()
{
    delete imageWindow;
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
	//TODO port it
    //imageKeyChooser->allDefault();
    //browserKeyChooser->allDefault();
}
