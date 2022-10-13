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
#undef DEBUG_CACHE

#include "imagecache.h"

#include <qfile.h>
#include <qimage.h>
#include <qdebug.h>
#ifdef DEBUG_TIMING
#include <qelapsedtimer.h>
#endif

#include <assert.h>

#include "kuickfile.h"
#include "kuickimage.h"
#include "imlibparams.h"


ImageCache::ImageCache(int maxImages)
{
    idleCount   = 0;
    setMaxImages(maxImages);
}


#ifdef DEBUG_CACHE
void ImageCache::dumpCache() const
{
    qDebug() << "cache total" << myCache.totalCost();
    for (const QUrl &u : myCache.keys())
    {
        qDebug() << "  " << u.toDisplayString();
    }
}
#endif


void ImageCache::setMaxImages(int maxImages)
{
    // The 'cost' of an item to be inserted in the cache is always
    // set to be 1.  This means that the maximum cost of the cache
    // is the number of images set here, as before.
    myCache.setMaxCost(maxImages);
#ifdef DEBUG_CACHE
    qDebug() << "cache size set to" << myCache.maxCost() << "images";
#endif
}

int ImageCache::maxImages() const
{
    return (myCache.maxCost());
}


void ImageCache::slotBusy()
{
    if (idleCount==0) emit sigBusy();
    ++idleCount;
}

void ImageCache::slotIdle()
{
    --idleCount;
    if (idleCount==0) emit sigIdle();
}


KuickImage *ImageCache::getKuimage(KuickFile *file)
{
    if (file==nullptr) return (nullptr);

    assert(file->isAvailable());			// debug build only
    if (file->waitForDownload(nullptr)!=KuickFile::OK) return (nullptr);

#ifdef DEBUG_CACHE
    dumpCache();
    qDebug() << "requesting" << file->url();
#endif
    return (myCache.object(file->url()));
}


#ifndef HAVE_QTONLY
// Note: the returned image's filename will not be the real filename (which it usually
// isn't anyway, according to Imlib's sources).
static IMLIBIMAGE loadImageWithQt(const QString &fileName)
{
    qDebug() << "loading" << fileName;

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
	return (nullptr);
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
            return (nullptr);
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

    IMLIBIMAGE im = Imlib_create_image_from_data(myId, newImageData, nullptr, w, h);
    delete[] newImageData;
#ifdef DEBUG_TIMING
    qDebug() << "create took" << timer.elapsed() << "ms";
#endif
    return (im);
}
#endif // HAVE_QTONLY


KuickImage *ImageCache::loadImage(KuickFile *file, const ImlibColorModifier &mod)
{
	if (file==nullptr || !file->isAvailable()) return (nullptr);
        const QString fileName = file->localFile();
        qDebug() << "loading" << fileName;

	slotBusy();
#ifdef DEBUG_TIMING
	qDebug() << "starting to load image";
	QElapsedTimer timer;
	timer.start();
#endif

#ifdef HAVE_QTONLY
        QImage im(fileName);
        if (!im.isNull())
        {
            if (im.depth()!=32) im = im.convertToFormat(QImage::Format_RGB32);
        }
#else // HAVE_QTONLY
	IMLIBIMAGE im = Imlib_load_image(ImlibParams::imlibData(), QFile::encodeName(fileName).data());
        if (im==nullptr)
        {
            qWarning() << "image loading failed";
            return (nullptr);
        }
#endif // HAVE_QTONLY
#ifdef DEBUG_TIMING
	qDebug() << "load took" << timer.elapsed() << "ms";
#endif
	slotIdle();

#ifndef HAVE_QTONLY
	if (im==nullptr)				// failed to load via imlib,
	{						// fall back to loading via Qt
		slotBusy();
		im = loadImageWithQt(fileName);
		slotIdle();
		if (im==nullptr) return (nullptr);
	}

        // TODO: for Qt only
	Imlib_set_image_modifier(ImlibParams::imlibData(), im, const_cast<ImlibColorModifier *>(&mod));
#endif // HAVE_QTONLY
	KuickImage *kuim = new KuickImage(file, im);
	connect( kuim, SIGNAL( startRendering() ),   SLOT( slotBusy() ));
	connect( kuim, SIGNAL( stoppedRendering() ), SLOT( slotIdle() ));

	myCache.insert(file->url(), kuim, 1);
#ifdef DEBUG_CACHE
	qDebug() << "inserted" << file->url();
	dumpCache();
#endif
	return (kuim);
}
