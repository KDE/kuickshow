/****************************************************************************
** $Id$
**
** KuickShow - a fast and comfortable image viewer based on Rasterman's Imlib
**
** Created : 98
**
** Copyright (C) 1998 - 2001 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/

#include <qdatetime.h>
#include <qglobal.h>
#include <qgroupbox.h>
#include <qlabel.h>

#include <kwin.h>
#include <kstandarddirs.h>

#include "imlibwidget.h"
#include "kurlwidget.h"
#include "version.h"

#include "aboutwidget.h"

AboutWidget::AboutWidget( QWidget *parent, const char *name )
    : QVBox( parent, name )
{
    KWin::setType( winId(), NET::Override );
    KWin::setState( winId(), NET::StaysOnTop | NET::SkipTaskbar );

    setFrameStyle( WinPanel | Raised );

    QGroupBox *gBox = new QGroupBox( 1, Horizontal, this);
    gBox->setGeometry( 10, 10, width()-20, height()-20 );
    gBox->setAlignment( AlignHCenter );
    gBox->installEventFilter( this );

    gBox->setPalette( QPalette( QColor( white ) ) );
    gBox->setBackgroundMode( PaletteBackground );

    int hour = QTime::currentTime().hour();
    QString file;

    if ( hour >= 10 && hour < 16 )
	file = locate("appdata", "pics/kuickshow-day.jpg");
    else
	file = locate("appdata", "pics/kuickshow-night.jpg");

    QLabel *authors = new QLabel("Kuickshow " KUICKSHOWVERSION
				 " was brought to you by", gBox);
    authors->setAlignment( AlignCenter );
    authors->installEventFilter( this );

    KURLWidget *homepage = new KURLWidget("Carsten Pfeiffer", gBox);
    homepage->setURL( "http://devel-home.kde.org/~pfeiffer/kuickshow/" );
    homepage->setAlignment( AlignCenter );

    QLabel *copy = new QLabel("(C) 1998-2001", gBox);
    copy->setAlignment( AlignCenter );
    copy->installEventFilter( this );

    ImlibWidget *im = new ImlibWidget( 0L, gBox, "KuickShow Logo" );
    if ( im->loadImage( file ) ) {
	im->setFixedSize( im->width(), im->height() );
	im->installEventFilter( this );
    }
    else {
	delete im;
	im = 0L;
	qWarning( "KuickShow: about-image not found/unreadable." );
    }
}

AboutWidget::~AboutWidget()
{
}

bool AboutWidget::eventFilter( QObject *o, QEvent *e )
{
    if ( e->type() == QEvent::MouseButtonPress ) {
	delete this;
	return true;
    }

    return QVBox::eventFilter( o, e );
}
#include "aboutwidget.moc"
