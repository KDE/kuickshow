#include <unistd.h>

#include <qfile.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>

#include "kuickio.h"

KuickIO * KuickIO::s_self = 0L;
QWidget * KuickIO::s_parent = 0L;

KuickIO * KuickIO::self( QWidget *parent )
{
    if ( !s_self )
	s_self = new KuickIO();

    s_self->s_parent = parent;
    return s_self;
}


#include "kuickio.moc"
