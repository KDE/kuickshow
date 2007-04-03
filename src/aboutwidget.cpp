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
#include <q3groupbox.h>
#include <qlabel.h>
//Added by qt3to4:
#include <QMouseEvent>

#include <kwm.h>
#include <kstandarddirs.h>

#include "imlibwidget.h"
#include "kurlwidget.h"
#include "version.h"

#include "aboutwidget.h"

AboutWidget::AboutWidget( QWidget *parent, const char *name )
    : Q3VBox( parent, name )
{
    KWM::setType( winId(), NET::Override );
    KWM::setState( winId(), NET::StaysOnTop | NET::SkipTaskbar );

    setFrameStyle( WinPanel | Raised );

    Q3GroupBox *gBox = new Q3GroupBox( 1, Qt::Horizontal, this);
    gBox->setGeometry( 10, 10, width()-20, height()-20 );
    gBox->setAlignment( Qt::AlignHCenter );
    gBox->installEventFilter( this );

    gBox->setPalette( QPalette( QColor( Qt::white ) ) );
    gBox->setBackgroundMode( Qt::PaletteBackground );

    int hour = QTime::currentTime().hour();
    QString file;

    if ( hour >= 10 && hour < 16 )
	file = KStandardDirs::locate("appdata", "pics/kuickshow-day.jpg");
    else
	file = KStandardDirs::locate("appdata", "pics/kuickshow-night.jpg");

    QLabel *authors = new QLabel("Kuickshow " KUICKSHOWVERSION
				 " was brought to you by", gBox);
    authors->setAlignment( Qt::AlignCenter );

    m_homepage = new KURLWidget("Carsten Pfeiffer", gBox);
    m_homepage->setUrl( "http://devel-home.kde.org/~pfeiffer/kuickshow/" );
    m_homepage->setAlignment( Qt::AlignCenter );

    QLabel *copy = new QLabel("(C) 1998-2004", gBox);
    copy->setAlignment( Qt::AlignCenter );

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
            delete this;
            return true;
        }
    }

    return Q3VBox::eventFilter( o, e );
}
#include "aboutwidget.moc"
