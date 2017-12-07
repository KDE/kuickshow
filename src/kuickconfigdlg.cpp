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

#include <QDialogButtonBox>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KShortcutsDialog>

#include <QPushButton>

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

    generalWidget = new GeneralWidget( this );
    generalWidget->setObjectName( QString::fromLatin1( "general widget" ) );
    addPage( generalWidget, i18n("&General") );

    defaultsWidget = new DefaultsWidget( this );
    defaultsWidget->setObjectName( QString::fromLatin1( "defaults widget" ) );
    addPage( defaultsWidget, i18n("&Modifications") );

    slideshowWidget = new SlideShowWidget( this );
    slideshowWidget->setObjectName( QString::fromLatin1( "slideshow widget" ) );
    addPage( slideshowWidget, i18n("&Slideshow")  );

    imageWindow = new ImageWindow(); // just to get the accel...
    imageWindow->hide();
    imageKeyChooser = new KShortcutsEditor( imageWindow->actionCollection(), this );
    addPage( imageKeyChooser, i18n("&Viewer Shortcuts") );

    browserKeyChooser = new KShortcutsEditor( coll, this );
    addPage( browserKeyChooser, i18n("Bro&wser Shortcuts") );

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

    KSharedConfig::openConfig()->sync();
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
