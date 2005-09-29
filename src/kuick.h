/* This file is part of the KDE project
   Copyright (C) 2000,2001,2002 Carsten Pfeiffer <pfeiffer@kde.org>

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

#ifndef KUICKGLOBALS_H
#define KUICKGLOBALS_H

#include <qrect.h>
#include <qsize.h>

#include <kwin.h>
#include <kwinmodule.h>

class Kuick
{
public:
    static QRect workArea() {
	return self()->winModule.workArea();
    }

    static QSize frameSize( WId win = 0L ) {
	if ( win ) {
	    KWin::WindowInfo info = KWin::windowInfo(win, NET::WMKDEFrameStrut | NET::WMGeometry);
	    int wborder = info.frameGeometry().width() - info.geometry().width();
	    int hborder = info.frameGeometry().height() - info.geometry().height();
	
	    if ( wborder || hborder ) { // we get a 0,0 border when not shown
		s_frameSize.setWidth( wborder );
		s_frameSize.setHeight( hborder );
	    }
	}
	
	if ( !s_frameSize.isValid() )
	    return QSize( 0, 0 );

	return s_frameSize;
    }

    static Kuick * self() {
	if ( !s_self ) {
	    s_self = new Kuick;
	}
	return s_self;
    }

    KWinModule winModule;

private:
    Kuick() {}
    static Kuick * s_self;

    static QSize s_frameSize;
};


#endif // KUICKGLOBALS_H
