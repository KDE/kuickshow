/**
 * $Id$
 *
 * Copyright 1998, 1999, 2000, 20001 by Carsten Pfeiffer
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
#if KDE_VERSION >= 220
    completionBox()->setTabHandling( true );
#endif
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

void FileFinder::slotAccept( const QString& dir )
{
    hide();
    emit enterDir( dir );
}

#include "filefinder.moc"
