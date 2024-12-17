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

//#define DEBUG_TIMING
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
    if (idleCount==0) Q_EMIT sigBusy();
    ++idleCount;
}

void ImageCache::slotIdle()
{
    --idleCount;
    if (idleCount==0) Q_EMIT sigIdle();
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


KuickImage *ImageCache::loadImage(KuickFile *file, const ImageModifiers &mod)
{
	if (file==nullptr || !file->isAvailable()) return (nullptr);
        const QString fileName = file->localFile();

	slotBusy();
#ifdef DEBUG_TIMING
	qDebug() << "starting to load image";
	QElapsedTimer timer;
	timer.start();
#endif

	ImageHandle image = ImageLibrary::loadImage(fileName);
#ifdef DEBUG_TIMING
	qDebug() << "load took" << timer.elapsed() << "ms";
#endif
	slotIdle();

	if (!image) {
		qWarning("failed to load image %s", qPrintable(fileName));
		return nullptr;
	}

	ImageLibrary::setImageModifiers(image, mod);
	KuickImage *kuim = new KuickImage(file, image);
	connect(kuim, &KuickImage::startRendering, this, &ImageCache::slotBusy);
	connect(kuim, &KuickImage::stoppedRendering, this, &ImageCache::slotIdle);

	myCache.insert(file->url(), kuim, 1);
#ifdef DEBUG_CACHE
	qDebug() << "inserted" << file->url();
	dumpCache();
#endif
	return (kuim);
}
