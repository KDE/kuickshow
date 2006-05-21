/****************************************************************************
** $Id: .emacs,v 1.3 2006/02/20 15:06:53 gis Exp $
**
** Created : 2006
**
** Copyright (C) 2006 Carsten Pfeiffer <pfeiffer@kde.org>
**
****************************************************************************/

#ifndef FILECACHE_H
#define FILECACHE_H

#include <qcache.h>

#include "kuickfile.h"

class KTempDir;

class FileCache
{
public:
    static FileCache * self();
    static void shutdown();

    KuickFile * getFile( const KURL& url );
    void setLimit( int numFiles );
    int getLimit() const { return m_limit; }

    /**
     * @return the temporary directory or QString::null if none available
     */
    QString tempDir();

private:
    static FileCache *s_self;
    FileCache();
    ~FileCache();

    KTempDir * createTempDir();
    QCache<KuickFile> m_files;

    int m_limit;
    KTempDir *m_tempDir;

};

#endif // FILECACHE_H
