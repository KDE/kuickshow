#include <unistd.h>

#include <qstring.h>

#include <kcomponentdata.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <ktempdir.h>

#include "filecache.h"

FileCache * FileCache::s_self;

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

KuickFile * FileCache::getFile( const KUrl& url )
{
    QString urlString = url.prettyUrl();
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
            kWarning() << "Unable to create temporary directory for KuickShow" << endl;
            return QString::null;
        }
    }

    return m_tempDir->name();
}


KTempDir * FileCache::createTempDir()
{
    QString tmpName = KGlobal::mainComponent().componentName();
    tmpName.append( QString::number( getpid() ) );
    QString dirName = KStandardDirs::locateLocal( "tmp", tmpName );
    KTempDir *dir = new KTempDir( dirName );
    dir->setAutoRemove( true );
    if ( dir->status() != 0L )
    {
        delete dir;
        return 0L;
    }

    return dir;
}
