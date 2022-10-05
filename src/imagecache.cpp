/* This file is part of the KDE project
   Copyright (C) 1998-2002 Carsten Pfeiffer <pfeiffer@kde.org>

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

#define DEBUG_TIMING

#include "imagecache.h"

#include <qfile.h>
#include <qimage.h>
#ifdef DEBUG_TIMING
#include <qelapsedtimer.h>
#include <qdebug.h>
#endif

#include <assert.h>

#include "kuickfile.h"
#include "kuickimage.h"


// TODO: maybe use QCache to simplify and avoid the below

// uhh ugly, we have two lists to map from filename to KuickImage :-/
ImageCache::ImageCache( ImlibData *id, int maxImages )
{
    myId        = id;
    idleCount   = 0;
    myMaxImages = maxImages;
}


ImageCache::~ImageCache()
{
    while ( !kuickList.isEmpty() ) {
        delete kuickList.takeFirst();
    }
    fileList.clear();
}


void ImageCache::setMaxImages( int maxImages )
{
    myMaxImages = maxImages;
    int count   = kuickList.count();
    while ( count > myMaxImages ) {
	delete kuickList.takeLast();
	fileList.removeLast();
	count--;
    }
}

void ImageCache::slotBusy()
{
    if ( idleCount == 0 )
	emit sigBusy();

    idleCount++;
}

void ImageCache::slotIdle()
{
    idleCount--;

    if ( idleCount == 0 )
	emit sigIdle();
}


KuickImage * ImageCache::getKuimage( KuickFile * file )
{
    if ( !file )
	return 0L;

    assert( file->isAvailable() ); // debug build
    if ( file->waitForDownload( 0L ) != KuickFile::OK ) // and for users
        return 0L;

    KuickImage *kuim = 0L;
    int index = fileList.indexOf( file );
    if ( index != -1 )
    {
        if ( index == 0 )
            kuim = kuickList.at( 0 );

        // need to reorder the lists, otherwise we might delete the current
        // image when a new one is cached and the current one is the last!
        else {
            kuim = kuickList.takeAt( index );
            kuickList.insert( 0, kuim );
            fileList.removeAll( file );
            fileList.prepend( file );
        }

        return kuim;
    }

    return 0L;
}

// TODO: 'mod' by const ref
KuickImage * ImageCache::loadImage( KuickFile * file, ImlibColorModifier mod)
{
	KuickImage *kuim = 0L;
	if ( !file || !file->isAvailable() )
		return 0L;

	slotBusy();
#ifdef DEBUG_TIMING
	qDebug() << "starting to load image";
	QElapsedTimer timer;
	timer.start();
#endif
	ImlibImage *im = Imlib_load_image( myId, QFile::encodeName( file->localFile() ).data() );
#ifdef DEBUG_TIMING
	qDebug() << "load took" << timer.elapsed() << "ms, ok" << (im!=nullptr);
#endif
	slotIdle();

	if ( !im ) {
        slotBusy();
		im = loadImageWithQt( file->localFile() );
		slotIdle();
		if ( !im )
			return 0L;
	}

	Imlib_set_image_modifier( myId, im, &mod );
	kuim = new KuickImage( file, im, myId );
	connect( kuim, SIGNAL( startRendering() ),   SLOT( slotBusy() ));
	connect( kuim, SIGNAL( stoppedRendering() ), SLOT( slotIdle() ));

	kuickList.insert( 0, kuim );
	fileList.prepend( file );

	if ( kuickList.count() > myMaxImages ) {
		//         qDebug(":::: now removing from cache: %s", (*fileList.fromLast()).toLatin1());
		delete kuickList.takeLast();
		fileList.removeLast();
	}

	return kuim;
}


// Note: the returned image's filename will not be the real filename (which it usually
// isn't anyway, according to Imlib's sources).
// TODO: can be made file-static once myId is a singleton
ImlibImage * ImageCache::loadImageWithQt( const QString& fileName ) const
{
    qDebug() << "Trying to load" << fileName;

#ifdef DEBUG_TIMING
    QElapsedTimer timer;
    qDebug() << "starting to load image";
    timer.start();
#endif
    QImage image( fileName );
#ifdef DEBUG_TIMING
    qDebug() << "load took" << timer.elapsed() << "ms, ok" << !image.isNull();
#endif
    if (image.isNull())
    {
        qDebug() << "loading failed";
	return 0L;
    }

    if (image.depth()!=32)
    {
#ifdef DEBUG_TIMING
	qDebug() << "starting to convert image from depth" << image.depth();
	timer.start();
#endif
	image = image.convertToFormat(QImage::Format_RGB32);
#ifdef DEBUG_TIMING
        qDebug() << "convert took" << timer.elapsed() << "ms, ok" << !image.isNull();
#endif
        if (image.isNull())
        {
            qDebug() << "converting to Format_RGB32 failed";
            return 0L;
        }
    }

    // convert to 24 bpp (discard alpha)
#ifdef DEBUG_TIMING
    qDebug() << "starting to create image";
    timer.start();
#endif
    int numPixels = image.width() * image.height();
    const int NUM_BYTES_NEW  = 3; // 24 bpp
    // TODO: use a QByteArray
    uchar *newImageData = new uchar[numPixels * NUM_BYTES_NEW];
    uchar *newData = newImageData;

    int w = image.width();
    int h = image.height();

    for (int y = 0; y < h; y++) {
	QRgb *scanLine = reinterpret_cast<QRgb *>( image.scanLine(y) );
	for (int x = 0; x < w; x++) {
	    const QRgb& pixel = scanLine[x];
	    *(newData++) = qRed(pixel);
	    *(newData++) = qGreen(pixel);
	    *(newData++) = qBlue(pixel);
	}
    }

    ImlibImage *im = Imlib_create_image_from_data( myId, newImageData, NULL,
                                                   image.width(), image.height() );

    delete[] newImageData;
#ifdef DEBUG_TIMING
    qDebug() << "create took" << timer.elapsed() << "ms";
#endif
    return im;
}
