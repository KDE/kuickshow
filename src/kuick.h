/****************************************************************************
** $Id$
**
** Definition of something or other
**
** Created : 2000
**
** Copyright (C) 2000 by Carsten Pfeiffer
**
****************************************************************************/

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
	    KWin::Info info = KWin::info( win );
	    int wborder = info.frameGeometry.width() - info.geometry.width();
	    int hborder = info.frameGeometry.height() - info.geometry.height();
	
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
