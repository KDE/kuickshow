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

bool KuickIO::deleteFile( const QString& path, bool dontAsk )
{
    KURL url;
    if ( path.at(0) == '/' )
        url.setPath( path );
    else
        url = path;

    deleteFile( url, dontAsk );
}

bool KuickIO::deleteFile( const KURL& url, bool dontAsk )
{
    if ( !dontAsk ) {
	QString tmp = i18n( "Really delete the file\n\n%1 ?\n" ).arg(url.prettyURL());
	int res = KMessageBox::questionYesNo( s_parent, tmp,
					     i18n("Delete file?"));
	if ( res == KMessageBox::No )
	    return false;
    }

    bool deleted = true;

    // first we try to unlink, then resort to KIO
    if ( url.isLocalFile() ) {
	if ( unlink( QFile::encodeName( url.path(-1)) ) < 0 ) {
	    deleted = KIO::NetAccess::del( url );
	    if ( !deleted ) {
		QString tmp = i18n( "Sorry, I can't delete the file\n\n%1").arg(url.prettyURL());
		KMessageBox::sorry( s_parent, tmp, i18n( "Delete failed" ) );
	    }
	}
    }
	
    else {
	deleted = KIO::NetAccess::del( url );
    }

    return deleted;
}

bool KuickIO::deleteFiles( const KURL::List& /*list*/, bool /*dontAsk*/ )
{
    return true;
}

#include "kuickio.moc"
