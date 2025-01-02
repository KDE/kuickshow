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
#ifndef FILECACHE_H
#define FILECACHE_H

#include <QCache>

class QString;
class QTemporaryDir;
class QTemporaryFile;
class QUrl;
class KuickFile;


class FileCache
{
public:
    static FileCache * self();
    static void shutdown();

    KuickFile * getFile( const QUrl& url );
    int getLimit() const { return m_limit; }

    /**
     * @return the temporary directory or QString() if none available
     */
    QString tempDir();

    /**
     * Creates a new QTemporaryFile in this cache's temp dir and returns it unopened.
     * It is the responsibility of the caller to delete the object when it is no longer needed.
     * @return the QTemporaryFile object, or nullptr on error
     */
    QTemporaryFile* createTempFile(const QString& suffix, const QString& prefix = QString());

private:
    static FileCache *s_self;
    FileCache();
    ~FileCache();

    QTemporaryDir* createTempDir();
    QCache<QString,KuickFile> m_files;

    int m_limit;
    QTemporaryDir* m_tempDir;
};

#endif // FILECACHE_H
