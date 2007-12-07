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

#include <qkeycode.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kcompletionbox.h>
#include <kurlcompletion.h>

#include "filefinder.h"

FileFinder::FileFinder( QWidget *parent, const char *name )
    : KLineEdit( parent, name )
{
    // make this widget just as large, as the font is + 8 Pixels
    int height = fontMetrics().height() + 8;
    setFixedSize( 150, height );
    setFrame( true );

    setHandleSignals( true ); // we want the completionbox signals
    completionBox()->setTabHandling( true );

    connect( completionBox(), SIGNAL( userCancelled(const QString&) ),
             SLOT( hide() ));
    connect( completionBox(), SIGNAL( activated( const QString& ) ),
             SLOT( slotAccept( const QString& )));
    connect( this, SIGNAL( returnPressed( const QString& )),
             SLOT( slotAccept( const QString& ) ));

    KURLCompletion *comp = new KURLCompletion();
    comp->setReplaceHome( true );
    comp->setReplaceEnv( true );
    setCompletionObject( comp, false );
    setAutoDeleteCompletionObject( true );
    setFocusPolicy( ClickFocus );

    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, "GeneralConfiguration" );
    setCompletionMode( (KGlobalSettings::Completion)
               config->readNumEntry( "FileFinderCompletionMode",
                                     KGlobalSettings::completionMode()));
}

FileFinder::~FileFinder()
{
    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, "GeneralConfiguration" );
    config->writeEntry( "FileFinderCompletionMode", completionMode() );
}

void FileFinder::focusOutEvent( QFocusEvent *e )
{
    if ( e->reason() != QFocusEvent::Popup )
        hide();
}

void FileFinder::keyPressEvent( QKeyEvent *e )
{
    int key = e->key();
    if ( key == Key_Escape ) {
        hide();
        e->accept();
    }

    else {
	KLineEdit::keyPressEvent( e );
    }
}

void FileFinder::hide()
{
    KLineEdit::hide();
    parentWidget()->setFocus();
}

void FileFinder::slotAccept( const QString& dir )
{
    hide();
    emit enterDir( dir );
}

#include "filefinder.moc"
