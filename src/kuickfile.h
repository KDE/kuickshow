/****************************************************************************
** $Id: .emacs,v 1.3 2002/02/20 15:06:53 gis Exp $
**
** Created : 2006
**
** Copyright (C) 2006 Carsten Pfeiffer <pfeiffer@kde.org>
**
****************************************************************************/

#ifndef KUICKFILE_H
#define KUICKFILE_H

#include <qobject.h>
#include <qstring.h>

#include <kurl.h>
#include <kprogress.h>

namespace KIO {
    class Job;
    class FileCopyJob;
}

class KuickFile : public QObject
{
    Q_OBJECT

public:
	enum DownloadStatus
	{
		OK = 1,
		CANCELED,
		ERROR	
	};

    KuickFile(const KURL& url);

    /**
     * Cleans up resources and removes any temporary file, if available.
     */
    ~KuickFile();

    const KURL& url() const { return m_url; }


    QString localFile() const;

    bool download();

    /**
     * @return true if download is in progress
     */
    bool isDownloading() const { return m_job != 0L; }

    /**
     * @return true if a local file is available, that is,
     * @ref #localFile will return a non-empty name
     * ### HERE ADD mostlylocal thing!
     */
    bool isAvailable() const { return !localFile().isEmpty(); }

    /**
     * @return true if @ref #isAvailable() returns true AND @ref #url() is a remote URL,
     * i.e. the file really has been downloaded.
     */
    bool hasDownloaded() const;

    /**
     * Opens a modal dialog window, blocking user interaction until the download
     * has finished. If the file is already available, this function will return true
     * immediately.
     * @return true when the download has finished or false when the user aborted the dialog
     */
    KuickFile::DownloadStatus waitForDownload( QWidget *parent );

//    bool needsDownload();

signals:
    /**
     * Signals that download has finished for that file. Will only be emitted for non-local files!
     */
    void downloaded( KuickFile * );

private slots:
    void slotResult( KIO::Job *job );
    void slotProgress( KIO::Job *job, unsigned long percent );

private:
    KURL m_url;
    QString m_localFile;
    KIO::FileCopyJob *m_job;
    KProgress *m_progress;
    int m_currentProgress;

};

bool operator==( const KuickFile& first, const KuickFile& second );

#endif // KUICKFILE_H
