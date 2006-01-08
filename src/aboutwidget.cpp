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

#include <qdatetime.h>
#include <qevent.h>
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
    : QVBox( parent, name, Qt::WShowModal )
{
    KWin::setType( winId(), NET::Override );
    KWin::setState( winId(), NET::SkipTaskbar );

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

    m_homepage = new KURLWidget("Carsten Pfeiffer", gBox);
    m_homepage->setURL( "http://devel-home.kde.org/~pfeiffer/kuickshow/" );
    m_homepage->setAlignment( AlignCenter );

    QLabel *copy = new QLabel("(C) 1998-2006", gBox);
    copy->setAlignment( AlignCenter );

    ImlibWidget *im = new ImlibWidget( 0L, gBox, "KuickShow Logo" );
    if ( im->loadImage( file ) )
	im->setFixedSize( im->width(), im->height() );
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
        QMouseEvent *ev = static_cast<QMouseEvent*>( e );
        if ( !m_homepage->geometry().contains( ev->pos() ) ) {
            deleteLater();
            return true;
        }
    }

    return QVBox::eventFilter( o, e );
}
#include "aboutwidget.moc"
