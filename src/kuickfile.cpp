#include <qfile.h>

#include <kdebug.h>
#include <kdeversion.h>
#include <kinstance.h>
#include <klocale.h>
#include <kprogress.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <ktempfile.h>

#include "filecache.h"
#include "kuickfile.h"

KuickFile::KuickFile(const KURL& url)
    : QObject(),
      m_url( url ),
      m_job( 0L ),
      m_progress( 0L ),
      m_currentProgress( 0 )
{
    if ( m_url.isLocalFile())
        m_localFile = m_url.path();
    else {
    	const KURL& mostLocal = KIO::NetAccess::mostLocalURL( m_url, 0L );
    	if ( mostLocal.isValid() && mostLocal.isLocalFile() )
    		m_localFile = mostLocal.path();
    }
}

KuickFile::~KuickFile()
{
    delete m_job;

    if ( hasDownloaded() )
        QFile::remove( m_localFile );
}

QString KuickFile::localFile() const
{
    // Note: never call isAvailable() from here, directly or indirectly

    if ( isDownloading() )
        return QString::null;

    return m_localFile;
}

bool KuickFile::hasDownloaded() const
{
    return !m_url.isLocalFile() && isAvailable() && m_job != 0L;
}

// ### need an API for refreshing the file?
bool KuickFile::download()
{
    if ( m_url.isLocalFile() || isAvailable() )
        return true;

    if ( isDownloading() )
        return true;

    // reinitialize
    m_localFile = QString::null;
    m_currentProgress = 0;
    

    QString ext;
    QString fileName = m_url.fileName();
    int extIndex = fileName.findRev('.');
    if ( extIndex > 0 )
        ext = fileName.mid( extIndex + 1 );

    QString tempDir = FileCache::self()->tempDir();
    KTempFile tempFile( tempDir, ext );
    tempFile.setAutoDelete( tempDir.isNull() ); // in case there is no proper tempdir, make sure to delete those files!
    if ( tempFile.status() != 0 )
        return false;

    tempFile.close();
    if ( tempFile.status() != 0 )
        return false;

    KURL destURL;
    destURL.setPath( tempFile.name() );

    m_job = KIO::file_copy( m_url, destURL, -1, true, false, false ); // handling progress ourselves
    m_job->setAutoErrorHandlingEnabled( true );
    connect( m_job, SIGNAL( result( KIO::Job * )), SLOT( slotResult( KIO::Job * ) ));
    connect( m_job, SIGNAL( percent( KIO::Job *, unsigned long )), SLOT( slotProgress( KIO::Job *, unsigned long ) ));

    // TODO: generify background/foreground downloading?

    return m_job != 0L;
}

KuickFile::DownloadStatus KuickFile::waitForDownload( QWidget *parent )
{
    if ( isAvailable() )
        return OK;

    if ( !isDownloading() ) {
        if ( !download() )
            return ERROR;
    }

    KProgressDialog *dialog = new KProgressDialog( parent );
    dialog->setModal( true );
    dialog->setCaption( i18n("Downloading %1...").arg( m_url.fileName() ) );
    dialog->setLabel( i18n("Please wait while downloading\n%1").arg( m_url.prettyURL() ));
    dialog->setAllowCancel( true );
    dialog->setAutoClose( true );

    m_progress = dialog->progressBar();
    m_progress->setTotalSteps( 100 ); // percent
    m_progress->setProgress( m_currentProgress );
    dialog->exec();
    bool canceled = dialog->wasCancelled();
    delete dialog;
    m_progress = 0L;

	if ( canceled && m_job ) {
		m_job->kill();
		m_job = 0L;
		m_currentProgress = 0;
	}
    // ### when aborted, remove KuickImage from FileCache?

    if ( canceled )
        return CANCELED;

    if ( !isAvailable() )
        return ERROR;
    
    // ### use custom progress dialog with OK, SKIP, CANCEL?
     return OK;
}

void KuickFile::slotResult( KIO::Job *job )
{
    if (job != m_job) { // huh?
        return;
    }

    m_job = 0L;

    if ( job->error() != 0 ) {
    	m_currentProgress = 0;

        if ( job->error() != KIO::ERR_USER_CANCELED )
            kdWarning() << "ERROR: KuickFile::slotResult: " << job->errorString() << endl;

        QString canceledFile = static_cast<KIO::FileCopyJob*>(job)->destURL().path();
        QFile::remove( canceledFile );
        m_progress->topLevelWidget()->hide();
    }
    else {
	    m_localFile = static_cast<KIO::FileCopyJob*>(job)->destURL().path();
	    emit downloaded( this ); // before closing the progress dialog

	    if ( m_progress ) {
	        m_progress->setProgress( 100 );
#define BUGGY_VERSION KDE_MAKE_VERSION(3,5,2)
	        if ( KDE::version() <= BUGGY_VERSION ) {
	            m_progress->topLevelWidget()->hide(); // ### workaround broken KProgressDialog
	        }
	    }
    }
}

void KuickFile::slotProgress( KIO::Job *job, unsigned long percent )
{
    if (job != m_job) { // huh?
        return;
    }

    m_currentProgress = percent;
    
    if ( !m_progress )
        return;

    // only set 100% in slotResult. Otherwise, the progress dialog would be closed
    // before slotResult() is called.
    if ( percent >= 100 )
        percent = 99;

    m_progress->setProgress( (int) percent );
}

bool operator==( const KuickFile& first, const KuickFile& second ) {
    return first.url().equals( second.url() );
}

#include "kuickfile.moc"
