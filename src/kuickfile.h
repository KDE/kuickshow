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

#ifndef KUICKFILE_H
#define KUICKFILE_H

#include <QObject>
#include <QString>
#include <QUrl>

class KJob;
class QProgressBar;

namespace KIO {
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

    KuickFile(const QUrl& url);

    /**
     * Cleans up resources and removes any temporary file, if available.
     */
    ~KuickFile();

    const QUrl& url() const { return m_url; }


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
    void slotResult( KJob *job );
    void slotProgress( KJob *job, unsigned long percent );

private:
    QUrl m_url;
    QString m_localFile;
    KIO::FileCopyJob *m_job;
    QProgressBar *m_progress;
    int m_currentProgress;

};

bool operator==( const KuickFile& first, const KuickFile& second );

#endif // KUICKFILE_H
