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

#ifdef HAVE_IMLIB2
#define USE_IMLIB_SCALING
#endif // HAVE_IMLIB2
#ifdef HAVE_QTONLY
#undef USE_IMLIB_SCALING
#endif // HAVE_QTONLY

#include "kuickimage.h"

#include <qimage.h>
#ifdef DEBUG_TIMING
#include <qelapsedtimer.h>
#include <qdebug.h>
#endif

#include "imagemods.h"
#include "imlibparams.h"
#include "imdata.h"


KuickImage::KuickImage(const KuickFile *file, IMLIBIMAGE &im)
    : QObject(nullptr)
{
    myFile     = file;
    myIm       = im;
#ifndef HAVE_QTONLY
    myOrigIm   = nullptr;
#endif // HAVE_QTONLY

#ifdef HAVE_IMLIB1
    myWidth    = im->rgb_width;
    myHeight   = im->rgb_height;
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
    imlib_context_set_image(im);
    myWidth    = imlib_image_get_width();
    myHeight   = imlib_image_get_height();
#endif // HAVE_IMLIB2
#ifdef HAVE_QTONLY
    myWidth    = im.width();
    myHeight   = im.height();
#endif // HAVE_QTONLY

    setDirty(true);

    myOrigWidth  = myWidth;
    myOrigHeight = myHeight;
    myRotation   = ROT_0;
    myFlipMode   = FlipNone;
}

KuickImage::~KuickImage()
{
    if (isModified()) ImageMods::rememberFor(this);

#ifndef HAVE_QTONLY
    if ( myOrigIm )
    {
#ifdef HAVE_IMLIB1
	Imlib_destroy_image( ImlibParams::imlibData(), myOrigIm );
	Imlib_kill_image( ImlibParams::imlibData(), myIm );			// kill scaled image (### really? analyze!)
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
	imlib_context_set_image(myOrigIm);
	imlib_free_image();
	imlib_context_set_image(myIm);
	imlib_free_image();
#endif // HAVE_IMLIB2
    }
    else
    {
#ifdef HAVE_IMLIB1
	Imlib_destroy_image(ImlibParams::imlibData(), myIm);
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
	imlib_context_set_image(myIm);
	imlib_free_image();
#endif // HAVE_IMLIB2
    }
#endif // HAVE_QTONLY
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
#ifdef HAVE_IMLIB2
    imlib_context_set_image(myIm);
#endif // HAVE_IMLIB2

    if ( rot == ROT_180 ) { 		// rotate 180 degrees
#ifdef HAVE_QTONLY
	myIm = myIm.mirrored(true, true);
#else // HAVE_QTONLY
	Imlib_flip_image_horizontal( ImlibParams::imlibData(), myIm );
	Imlib_flip_image_vertical( ImlibParams::imlibData(), myIm );
#endif // HAVE_QTONLY
    }
    else if ( rot == ROT_90 || rot == ROT_270 ) {
	qSwap( myWidth, myHeight );
#ifdef HAVE_QTONLY
        QTransform t;
        t.rotate(rot==ROT_90 ? 90 : -90);
	myIm = myIm.transformed(t);
#else // HAVE_QTONLY
	Imlib_rotate_image( ImlibParams::imlibData(), myIm, -1 );

	if ( rot == ROT_90 ) 		// rotate 90 degrees
	    Imlib_flip_image_horizontal( ImlibParams::imlibData(), myIm );
	else if ( rot == ROT_270 ) 		// rotate 270 degrees
	    Imlib_flip_image_vertical( ImlibParams::imlibData(), myIm );
#endif // HAVE_QTONLY
    }

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
#ifdef HAVE_IMLIB2
    imlib_context_set_image(myIm);
#endif // HAVE_IMLIB2

    if ( flipMode & FlipHorizontal )
#ifdef HAVE_QTONLY
	    myIm = myIm.mirrored(true, false);
#else // HAVE_QTONLY
	Imlib_flip_image_horizontal( ImlibParams::imlibData(), myIm );
#endif // HAVE_QTONLY
    if ( flipMode & FlipVertical )
#ifdef HAVE_QTONLY
	    myIm = myIm.mirrored(false, true);
#else // HAVE_QTONLY
	Imlib_flip_image_vertical( ImlibParams::imlibData(), myIm );
#endif // HAVE_QTONLY

    myFlipMode = static_cast<FlipMode>(myFlipMode ^ flipMode);
    setDirty(true);
}

bool KuickImage::flipAbs( int mode )
{
    if ( myFlipMode == mode )
	return false;

    bool changed = false;

#ifdef HAVE_IMLIB2
    imlib_context_set_image(myIm);
#endif // HAVE_IMLIB2

    if ( ((myFlipMode & FlipHorizontal) && !(mode & FlipHorizontal)) ||
	 (!(myFlipMode & FlipHorizontal) && (mode & FlipHorizontal)) ) {
#ifdef HAVE_QTONLY
	myIm = myIm.mirrored(true, false);
#else // HAVE_QTONLY
	Imlib_flip_image_horizontal( ImlibParams::imlibData(), myIm );
#endif // HAVE_QTONLY
	changed = true;
    }

    if ( ((myFlipMode & FlipVertical) && !(mode & FlipVertical)) ||
	 (!(myFlipMode & FlipVertical) && (mode & FlipVertical)) ) {
#ifdef HAVE_QTONLY
	myIm = myIm.mirrored(false, true);
#else // HAVE_QTONLY
	Imlib_flip_image_vertical( ImlibParams::imlibData(), myIm );
#endif // HAVE_QTONLY
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

#ifdef HAVE_QTONLY
	if (!myOrigIm.isNull())
	{
		myIm = myOrigIm;
		myOrigIm = QImage();
	}
#else // HAVE_QTONLY
	if (myOrigIm!=nullptr)
	{
#ifdef HAVE_IMLIB1
		Imlib_destroy_image(ImlibParams::imlibData(), myIm);
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
		imlib_context_set_image(myIm);
		imlib_free_image();
#endif // HAVE_IMLIB2
		myIm = myOrigIm;
		myOrigIm = nullptr;
	}
#endif // HAVE_QTONLY

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


#ifndef HAVE_QTONLY
#ifndef USE_IMLIB_SCALING
static IMLIBIMAGE toImlibImage(ImlibData *id, const QImage &image)
{
	if (image.isNull()) return (nullptr);
	// The image depth should always be 32, because toImlibImage()
	// is only called from smoothResize() on the scaled result of
	// a toQImage() which always returns a 32bpp image.
	if (image.depth()!=32) return (nullptr);

#ifdef DEBUG_TIMING
	qDebug() << "starting to convert image size" << image.size();
	QElapsedTimer timer;
	timer.start();
#endif

	const int w = image.width();
	const int h = image.height();
	const int numPixels = w*h;

#ifdef HAVE_IMLIB1
	// Convert the QImage pixels to 24bpp (discard the alpha)
	const int NUM_BYTES_NEW = 3;
	uchar *newImageData = new uchar[numPixels*NUM_BYTES_NEW];
	uchar *newData = newImageData;
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
	// Convert the QImage pixels to 32bpp ARGB
	DATA32 *newImageData = new DATA32[numPixels];
	DATA32 *newData = newImageData;
#endif // HAVE_IMLIB2

	for (int y = 0; y<h; ++y)
	{
		const QRgb *scanLine = reinterpret_cast<const QRgb *>(image.scanLine(y));
		for (int x = 0; x<w; ++x)
		{
			// It would be possible to use QImage::pixel() here, instead
			// of setting scanLine above and then indexing through it.
			// However, that is slower (typically 15ms for a 1200x900
			// image, as opposed to 7ms this way).
			const QRgb &pixel = scanLine[x];

			uchar r = qRed(pixel);
			uchar g = qGreen(pixel);
			uchar b = qBlue(pixel);
#ifdef HAVE_IMLIB1
			*(newData++) = r;
			*(newData++) = g;
			*(newData++) = b;
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
			DATA32 rgb = (r << 16)|(g << 8)|b;
			*(newData++) = rgb;
#endif // HAVE_IMLIB2
		}
	}

	IMLIBIMAGE im = Imlib_create_image_from_data(id, newImageData, nullptr, w, h);
	delete [] newImageData;
#ifdef DEBUG_TIMING
	qDebug() << "convert took" << timer.elapsed() << "ms";
#endif
	return (im);
}
#endif // USE_IMLIB_SCALING
#endif // HAVE_QTONLY


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
	emit startRendering();

	Qt::TransformationMode mode = ImlibParams::imlibConfig()->smoothScale ? Qt::SmoothTransformation :
		                                                                Qt::FastTransformation;
#ifdef USE_IMLIB_SCALING
#ifdef HAVE_IMLIB1
	IMLIBIMAGE newIm = Imlib_clone_scaled_image(ImlibParams::imlibData(), myIm, newWidth, newHeight);
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
	imlib_context_set_image(myIm);
	// TODO: use the "Smooth scaling" user setting
	imlib_context_set_anti_alias(1);
	IMLIBIMAGE newIm = imlib_create_cropped_scaled_image(0, 0, myWidth, myHeight, newWidth, newHeight);
#endif // HAVE_IMLIB2
#else // USE_IMLIB_SCALING
#ifdef HAVE_QTONLY
	QImage newIm = myIm.scaled(newWidth, newHeight, Qt::IgnoreAspectRatio, mode);
#else // HAVE_QTONLY
	// This uses Qt for scaling instead of Imlib, because Imlib1's
	// scaling does not appear to have such good smoothing even
	// if the "Smooth scaling" option is turned on.  It is slower,
	// though (typically 33ms to scale a 800x600 image up one step,
	// as opposed to 5ms by Imlib1).
	//
	// Note from original: QImage::ScaleMin seems to have a bug
	// (off-by-one, sometimes results in width being 1 pixel too small)
	QImage scaledImage = toQImage().scaled(newWidth, newHeight, Qt::IgnoreAspectRatio, mode);
	IMLIBIMAGE newIm = toImlibImage(ImlibParams::imlibData(), scaledImage);
#endif // HAVE_QTONLY
#endif // USE_IMLIB_SCALING

	emit stoppedRendering();
#ifdef DEBUG_TIMING
	qDebug() << "resize took" << timer.elapsed() << "ms";
#endif
#ifdef HAVE_QTONLY
	if (newIm.isNull()) return (false);
	if (myOrigIm.isNull()) myOrigIm = myIm;
#else // HAVE_QTONLY
	if (newIm==nullptr) return (false);
	if (myOrigIm==nullptr) myOrigIm = myIm;
#endif // HAVE_QTONLY
	else
	{
#ifdef HAVE_IMLIB1
		Imlib_destroy_image(ImlibParams::imlibData(), myIm);
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
		imlib_context_set_image(myIm);
		imlib_free_image();
#endif // HAVE_IMLIB2
	}

	myIm = newIm;
	myWidth = newWidth;
	myHeight = newHeight;
	setDirty(true);
	return (true);
}


QImage KuickImage::toQImage() const
{
	emit startRendering();

	IMLIBIMAGE im;
	if (myRotation == ROT_0 && myFlipMode == FlipNone
#ifdef HAVE_QTONLY
	    && !myOrigIm.isNull()
#else // HAVE_QTONLY
	    && myOrigIm!=nullptr
#endif // HAVE_QTONLY
	    && myWidth == myOrigWidth && myHeight == myOrigHeight)
	{
		// use original image if no other modifications have been applied
		// ### use orig image always and reapply mods?
		im = myOrigIm;
	}
	else
	{
		im = myIm;
	}

#ifdef HAVE_IMLIB1
	const int w = im->rgb_width;
	const int h = im->rgb_height;
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
	imlib_context_set_image(im);
	const int w = imlib_image_get_width();
	const int h = imlib_image_get_height();
#endif // HAVE_IMLIB2

#ifndef HAVE_QTONLY
#ifdef DEBUG_TIMING
	qDebug() << "starting to render image size" << QSize(w, h);
	QElapsedTimer timer;
	timer.start();
#endif
#endif // HAVE_QTONLY

	// TODO: be able to detect whether the modifier is actually needed,
	// see comment in imlibwidget.h

	// Apply image modifications as set by ImlibWidget::setImageModifier()
	// by taking a temporary clone of the image and applying the colour
	// modifications to its RGB data before extracting it.  Originally
	// done implicitly by Imlib_render() in KuickImage::renderPixmap().

#ifdef HAVE_IMLIB1
	ImlibColorModifier mod;
	Imlib_get_image_modifier(ImlibParams::imlibData(), im, &mod);

	IMLIBIMAGE tempImage = Imlib_clone_image(ImlibParams::imlibData(), im);
	Imlib_set_image_modifier(ImlibParams::imlibData(), tempImage, &mod);
	Imlib_apply_modifiers_to_rgb(ImlibParams::imlibData(), tempImage);
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
	// The colour modifier context will already have been set
	// by ImlibWidget::setImageModifier().
	imlib_context_set_image(im);
	IMLIBIMAGE tempImage = imlib_clone_image();
	imlib_context_set_image(tempImage);
	imlib_apply_color_modifier();
#endif // HAVE_IMLIB2

#ifndef HAVE_QTONLY
	im = tempImage;
	QImage image(w, h, QImage::Format_RGB32);	// destination image

#ifdef HAVE_IMLIB1
	const uchar *rgb = im->rgb_data;
	int byteIndex = 0;
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
	imlib_context_set_image(im);
	const DATA32 *rgb = imlib_image_get_data_for_reading_only();
	int wordIndex = 0;
#endif // HAVE_IMLIB2

	for (int y = 0; y<h; ++y)
	{
		QRgb *destImageData = reinterpret_cast<QRgb *>(image.scanLine(y));
		for (int x = 0; x<w; ++x)
		{
#ifdef HAVE_IMLIB1
			uchar r = rgb[byteIndex++];
			uchar g = rgb[byteIndex++];
			uchar b = rgb[byteIndex++];
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
			DATA32 word = rgb[wordIndex++];
			uchar r = (word & 0x00FF0000)>>16;
			uchar g = (word & 0x0000FF00)>>8;
			uchar b = (word & 0x000000FF);
#endif // HAVE_IMLIB2
			QRgb rgbPixel = qRgb(r, g, b);
			destImageData[x] = rgbPixel;
		}
	}

	if (tempImage!=nullptr)
	{
#ifdef HAVE_IMLIB1
		Imlib_destroy_image(ImlibParams::imlibData(), tempImage);
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
		imlib_context_set_image(tempImage);
		imlib_free_image();
#endif // HAVE_IMLIB2
	}
#ifdef DEBUG_TIMING
	qDebug() << "render took" << timer.elapsed() << "ms";
#endif
#else // HAVE_QTONLY
	QImage &image = im;
#endif // HAVE_QTONLY
	emit stoppedRendering();
	return image;
}
