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

#include "filecache.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QUrl>


FileCache* FileCache::s_self = nullptr;

FileCache::FileCache()
    : m_limit( 0 ),
      m_tempDir( 0L )
{
    m_files.setMaxCost( 100 ); // good default? ### make configurable?
}

FileCache::~FileCache()
{
    delete m_tempDir;
}

void FileCache::shutdown()
{
    if ( s_self )
    {
        delete s_self;
        s_self = 0L;
    }
}

FileCache * FileCache::self()
{
    if ( !s_self )
        s_self = new FileCache();
    return s_self;
}

KuickFile * FileCache::getFile( const QUrl& url )
{
    QString urlString = url.toDisplayString();
    KuickFile *file = m_files.object( urlString );
    if ( !file ) {
        file = new KuickFile( url );
        m_files.insert( urlString, file );
    }

    return file;
}

QString FileCache::tempDir()
{
    if ( !m_tempDir ) {
        m_tempDir = createTempDir();

        if ( !m_tempDir ) {
            qWarning("Unable to create temporary directory for KuickShow");
            return QString::null;
        }
    }

    return m_tempDir->path() + QLatin1Char('/');
}

QTemporaryFile* FileCache::createTempFile(const QString& suffix, const QString& prefix)
{
    QString nameTemplate = tempDir();
    if(nameTemplate.isEmpty()) return nullptr;

    nameTemplate += prefix + QStringLiteral("XXXXXX") + suffix;
    return new QTemporaryFile(nameTemplate);
}


QTemporaryDir* FileCache::createTempDir()
{
    QString nameTemplate = QStringLiteral("%1/%2_%3_XXXXXX")
            .arg(QDir::tempPath())
            .arg(QCoreApplication::applicationName())
            .arg(QCoreApplication::applicationPid());
    QTemporaryDir* dir = new QTemporaryDir(nameTemplate);

    if(!dir->isValid()) {
        delete dir;
        return 0L;
    }

    return dir;
}
