#include <qpixmap.h>
#include <qtooltip.h>

#include <kiconloader.h>
#include <krun.h>

#include "logo.h"


Logo::Logo(const QString& filename, QWidget *parent, const char *name)
    : QLabel(parent, name)
{
    setLineWidth(1);
    setFrameStyle(Panel|Sunken);

    QPixmap pix = UserIcon( filename );
    setPixmap( pix );
    adjustSize();

    QToolTip::add( this, "http://devel-home.kde.org/~pfeiffer/kuickshow/" );
}

void Logo::mousePressEvent( QMouseEvent *e )
{
    if ( e->button() == LeftButton )
	openHomepage();
}

void Logo::openHomepage()
{
    (void) new KRun( "http://devel-home.kde.org/~pfeiffer/kuickshow/" );
}
