/**
 * $Id:
 *
 * Copyright 1998, 1999 by Carsten Pfeiffer
 */

#include <qtooltip.h>

#include <kurl.h>
#include <krun.h>

#include "kurlwidget.h"

KURLWidget::KURLWidget(const QString& text, QWidget *parent, const char *name)
    : KURLLabel( parent, name )
{
    setText( text );
    connect( this, SIGNAL( leftClickedURL() ), SLOT( run() ));
    setUseTips( true );
}

void KURLWidget::run()
{
    KURL ku( url() );
    if ( !ku.isMalformed() ) {
	(void) new KRun( ku );
    }
}

#include "kurlwidget.moc"
