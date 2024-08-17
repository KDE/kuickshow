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

#define DEBUG_TIMING

#include "kuickimage.h"

#include <qimage.h>
#ifdef DEBUG_TIMING
#include <qelapsedtimer.h>
#include <qdebug.h>
#endif

#include "imagemods.h"


KuickImage::KuickImage(const KuickFile *file, ImageHandle im)
    : QObject(nullptr)
{
    myFile     = file;
    myIm       = im;

    const QSize imageSize = ImageLibrary::getImageSize(im);
    myWidth = imageSize.width();
    myHeight = imageSize.height();

    setDirty(true);

    myOrigWidth  = myWidth;
    myOrigHeight = myHeight;
    myRotation   = ROT_0;
    myFlipMode   = FlipNone;
}

KuickImage::~KuickImage()
{
    if (isModified()) ImageMods::rememberFor(this);
}


bool KuickImage::isModified() const
{
	bool modified = myWidth != myOrigWidth || myHeight != myOrigHeight;
	modified |= (myFlipMode != FlipNone);
	modified |= (myRotation != ROT_0);
	return modified;
}


void KuickImage::rotate( Rotation rot )
{
    ImageLibrary::rotateImage(myIm, rot);
    const QSize imageSize = ImageLibrary::getImageSize(myIm);
    myWidth = imageSize.width();
    myHeight = imageSize.height();

    myRotation = static_cast<Rotation>((myRotation + rot) % 4);
    setDirty(true);
}


bool KuickImage::rotateAbs( Rotation rot )
{
    if ( myRotation == rot )
	return false;

    int diff = rot - myRotation;
    bool clockWise = (diff > 0);

    switch( abs(diff) ) {
    case ROT_90:
        rotate( clockWise ? ROT_90 : ROT_270 );
	break;
    case ROT_180:
	rotate( ROT_180 );
	break;
    case ROT_270:
        rotate( clockWise ? ROT_270 : ROT_90 );
	break;
    }

    return true;
}

void KuickImage::flip( FlipMode flipMode )
{
    ImageLibrary::flipImage(myIm, flipMode);
    myFlipMode = static_cast<FlipMode>(myFlipMode ^ flipMode);
    setDirty(true);
}

bool KuickImage::flipAbs( int mode )
{
    if ( myFlipMode == mode )
	return false;

    bool changed = false;

    if ( ((myFlipMode & FlipHorizontal) && !(mode & FlipHorizontal)) ||
	 (!(myFlipMode & FlipHorizontal) && (mode & FlipHorizontal)) ) {
	ImageLibrary::flipImage(myIm, FlipHorizontal);
	changed = true;
    }

    if ( ((myFlipMode & FlipVertical) && !(mode & FlipVertical)) ||
	 (!(myFlipMode & FlipVertical) && (mode & FlipVertical)) ) {
	ImageLibrary::flipImage(myIm, FlipVertical);
	changed = true;
    }

    if ( changed ) {
        myFlipMode = static_cast<FlipMode>(mode);
        setDirty(true);
        return true;
    }

    return false;
}


void KuickImage::restoreOriginalSize()
{
	if (myWidth == myOrigWidth && myHeight == myOrigHeight) return;

	myIm = myOrigIm;
	myOrigIm.reset();

    myWidth   = myOrigWidth;
    myHeight  = myOrigHeight;
    setDirty(true);

    if ( myRotation == ROT_90 || myRotation == ROT_270 )
        qSwap( myWidth, myHeight );
}

void KuickImage::resize( int width, int height, KuickImage::ResizeMode mode )
{
	if ( myWidth == width && myHeight == height )
		return;

	if ( mode == KuickImage::SMOOTH )
	{
		if ( !smoothResize( width, height ) )
			fastResize( width, height );
	}
	else
	{
		fastResize( width, height );
	}
}

void KuickImage::fastResize( int width, int height )
{
//      qDebug("-- fastResize: %i x %i", width, height);
	smoothResize(width, height);
}

bool KuickImage::smoothResize( int newWidth, int newHeight )
{
//	qDebug("-- smoothResize: %i x %i", newWidth, newHeight);
#ifdef DEBUG_TIMING
	qDebug() << "starting to resize image size" << QSize(myWidth, myHeight);
	QElapsedTimer timer;
	timer.start();
#endif
	Q_EMIT startRendering();

	// resize a copy of the current image
	ImageHandle newIm = ImageLibrary::copyImage(myIm);
	if (KuickConfig::get().smoothScale) {
		ImageLibrary::resizeImageSmooth(newIm, newWidth, newHeight);
	} else {
		ImageLibrary::resizeImageFast(newIm, newWidth, newHeight);
	}

	Q_EMIT stoppedRendering();
#ifdef DEBUG_TIMING
	qDebug() << "resize took" << timer.elapsed() << "ms";
#endif

	if (!newIm) return false;
	if (!myOrigIm) myOrigIm = myIm;
	myIm = newIm;
	myWidth = newWidth;
	myHeight = newHeight;
	setDirty(true);
	return (true);
}


QImage KuickImage::toQImage() const
{
	Q_EMIT startRendering();

	ImageHandle im;
	if (myRotation == ROT_0 && myFlipMode == FlipNone && myOrigIm &&
	    myWidth == myOrigWidth && myHeight == myOrigHeight)
	{
		// use original image if no other modifications have been applied
		// ### use orig image always and reapply mods?
		im = myOrigIm;
	}
	else
	{
		im = myIm;
	}
	QImage image = ImageLibrary::toQImage(im);

	Q_EMIT stoppedRendering();
	return image;
}
