/* This file is part of the KDE project
   Copyright (C) 2003,2009 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include "kuickfile.h"
#include "kuickshow_debug.h"

#include <KIO/FileCopyJob>
#include <KIO/StatJob>
#include <KLocalizedString>

#include <QFile>
#include <QProgressDialog>
#include <QScopedPointer>
#include <QTemporaryFile>

#include "filecache.h"


KuickFile::KuickFile(const QUrl& url)
    : QObject(),
      m_url( url ),
      m_job( 0L ),
      m_progress( 0L ),
      m_currentProgress( 0 )
{
    if ( m_url.isLocalFile())
        m_localFile = m_url.path();
    else {
        QUrl mostLocal;
        KIO::StatJob* job = KIO::mostLocalUrl(m_url);
        connect(job, &KIO::StatJob::result, [job, &mostLocal]() {
            if(!job->error()) mostLocal = job->mostLocalUrl();
        });
        job->exec();

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
        return QString();

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
    m_localFile.clear();
    m_currentProgress = 0;


    QString ext;
    QString fileName = m_url.fileName();
    int extIndex = fileName.lastIndexOf('.');
    if ( extIndex > 0 )
        ext = fileName.mid( extIndex );

    QScopedPointer<QTemporaryFile> tempFilePtr(FileCache::self()->createTempFile(ext));
    if(tempFilePtr.isNull() || !tempFilePtr->open()) return false;

    QUrl destURL = QUrl::fromLocalFile(tempFilePtr->fileName());

    // we don't need the actual temp file, just its unique name
    tempFilePtr.reset();

    m_job = KIO::file_copy( m_url, destURL, -1, KIO::HideProgressInfo | KIO::Overwrite ); // handling progress ourselves
//    m_job->setAutoErrorHandlingEnabled( true );
    connect( m_job, SIGNAL( result( KJob * )), SLOT( slotResult( KJob * ) ));
    connect( m_job, SIGNAL( percent( KJob *, unsigned long )), SLOT( slotProgress( KJob *, unsigned long ) ));

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

    QProgressDialog *dialog = new QProgressDialog( parent );
    dialog->setWindowTitle( i18n("Downloading %1...", m_url.fileName() ) );
    dialog->setLabelText( i18n("Please wait while downloading\n%1", m_url.toDisplayString() ));
    dialog->setAutoClose( true );

    dialog->setMaximum( 100 ); // percent
    dialog->setValue( m_currentProgress );

    m_progress = dialog;
    dialog->exec();
    m_progress = nullptr;

    bool canceled = dialog->wasCanceled();
    delete dialog;

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

void KuickFile::slotResult( KJob *job )
{
    if (job != m_job) { // huh?
        return;
    }

    m_job = 0L;

    if ( job->error() != 0 ) {
    	m_currentProgress = 0;

        if ( job->error() != KIO::ERR_USER_CANCELED )
            qWarning("ERROR: KuickFile::slotResult: %s", qUtf8Printable(job->errorString()));

        QString canceledFile = static_cast<KIO::FileCopyJob*>(job)->destUrl().path();
        QFile::remove( canceledFile );
        m_progress->hide();
    }
    else {
	    m_localFile = static_cast<KIO::FileCopyJob*>(job)->destUrl().path();
	    emit downloaded( this ); // before closing the progress dialog

	    if ( m_progress ) {
	        m_progress->setValue( 100 );
	    }
    }
}

void KuickFile::slotProgress( KJob *job, unsigned long percent )
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

    m_progress->setValue(static_cast<int>(percent));
}

bool operator==( const KuickFile& first, const KuickFile& second ) {
    return first.url() == second.url();
}
