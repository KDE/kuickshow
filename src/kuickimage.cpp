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
#undef USE_IMLIB_SCALING

#include "kuickimage.h"

#include <qimage.h>

#ifdef DEBUG_TIMING
#include <qelapsedtimer.h>
#include <qdebug.h>
#endif

#include "imagemods.h"


KuickImage::KuickImage( const KuickFile * file, ImlibImage *im, ImlibData *id)
    : QObject( 0L )
{
    myFile     = file;
    myOrigIm   = 0L;
    myIm       = im;
    myId       = id;
    myWidth    = im->rgb_width;
    myHeight   = im->rgb_height;
    setDirty(true);

    myOrigWidth  = myWidth;
    myOrigHeight = myHeight;
    myRotation   = ROT_0;
    myFlipMode   = FlipNone;
}

KuickImage::~KuickImage()
{
	if (isModified())
	{
		ImageMods::rememberFor( this );
	}

    if ( myOrigIm )
    {
    	Imlib_destroy_image( myId, myOrigIm );
	    Imlib_kill_image( myId, myIm ); // kill scaled image (### really? analyze!)
    }
    else
	    Imlib_destroy_image( myId, myIm );
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
    if ( rot == ROT_180 ) { 		// rotate 180 degrees
	Imlib_flip_image_horizontal( myId, myIm );
	Imlib_flip_image_vertical( myId, myIm );
    }

    else if ( rot == ROT_90 || rot == ROT_270 ) {
	qSwap( myWidth, myHeight );
	Imlib_rotate_image( myId, myIm, -1 );

	if ( rot == ROT_90 ) 		// rotate 90 degrees
	    Imlib_flip_image_horizontal( myId, myIm );
	else if ( rot == ROT_270 ) 		// rotate 270 degrees
	    Imlib_flip_image_vertical( myId, myIm );
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
    if ( flipMode & FlipHorizontal )
	Imlib_flip_image_horizontal( myId, myIm );
    if ( flipMode & FlipVertical )
	Imlib_flip_image_vertical( myId, myIm );

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
	Imlib_flip_image_horizontal( myId, myIm );
	changed = true;
    }

    if ( ((myFlipMode & FlipVertical) && !(mode & FlipVertical)) ||
	 (!(myFlipMode & FlipVertical) && (mode & FlipVertical)) ) {
	Imlib_flip_image_vertical( myId, myIm );
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
	if (myWidth == myOrigWidth && myHeight == myOrigHeight)
		return;

//	qDebug("-- restoreOriginalSize");

	if ( myOrigIm != 0L )
	{
		Imlib_destroy_image( myId, myIm );
		myIm = myOrigIm;
		myOrigIm = 0L;
	}

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


static ImlibImage *toImlibImage(ImlibData *id, const QImage &image)
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
	// Convert the QImage pixels to 24bpp (discard the alpha)
	const int NUM_BYTES_NEW = 3;

	const int w = image.width();
	const int h = image.height();
	const int numPixels = w*h;

	uchar *newImageData = new uchar[numPixels*NUM_BYTES_NEW];
	uchar *newData = newImageData;

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
			*(newData++) = qRed(pixel);
			*(newData++) = qGreen(pixel);
			*(newData++) = qBlue(pixel);
		}
	}

	ImlibImage *im = Imlib_create_image_from_data(id, newImageData, nullptr, w, h);
	delete [] newImageData;
#ifdef DEBUG_TIMING
	qDebug() << "convert took" << timer.elapsed() << "ms";
#endif
	return (im);
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
	emit startRendering();
#ifdef USE_IMLIB_SCALING
	ImlibImage *newIm = Imlib_clone_scaled_image(myId, myIm, newWidth, newHeight);
#else
	// This uses Qt for scaling instead of Imlib, because Imlib1's
	// scaling does not appear to have such good smoothing even
	// if the "Smooth scaling" option is turned on.  It is slower,
	// though (typically 33ms to scale a 800x600 image up one step,
	// as opposed to 5ms by Imlib1).
	//
	// Note from original: QImage::ScaleMin seems to have a bug
	// (off-by-one, sometimes results in width being 1 pixel too small)
	QImage scaledImage = toQImage().scaled(newWidth, newHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	ImlibImage *newIm = toImlibImage(myId, scaledImage);
#endif
	emit stoppedRendering();
#ifdef DEBUG_TIMING
	qDebug() << "resize took" << timer.elapsed() << "ms";
#endif
	if (newIm==nullptr) return (false);

	if (myOrigIm==nullptr) myOrigIm = myIm;
	else Imlib_destroy_image(myId, myIm);

	myIm = newIm;
	myWidth = newWidth;
	myHeight = newHeight;
	setDirty(true);
	return (true);
}


QImage KuickImage::toQImage() const
{
	emit startRendering();

	ImlibImage *im;
	if (myOrigIm != 0L && myRotation == ROT_0 && myFlipMode == FlipNone
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

	int w = im->rgb_width;
	int h = im->rgb_height;

#ifdef DEBUG_TIMING
	qDebug() << "starting to render image size" << QSize(w, h);
	QElapsedTimer timer;
	timer.start();
#endif
	// TODO: be able to detect whether the modifier is actually needed,
	// see comment in imlibwidget.h

	// Apply image modifications as set by ImlibWidget::setImageModifier()
	// by taking a temporary clone of the image and applying the colour
	// modifications to its RGB data before extracting it.  Originally
	// done implicitly by Imlib_render() in KuickImage::renderPixmap().
	ImlibImage *tempImage = Imlib_clone_image(myId, im);
	Imlib_apply_modifiers_to_rgb(myId, tempImage);
	im = tempImage;

	QImage image(w, h, QImage::Format_RGB32);
	uchar *rgb = im->rgb_data;

	int byteIndex = 0;
	int destLineIndex = 0;
	int destByteIndex = 0;
	for ( int pixel = 0; pixel < (w * h); pixel++ )
	{
		// TODO: similify this block using 2 nested loops,
		// see toImlibImage()
		if ( pixel != 0 && (pixel % w) == 0 )
		{
			destLineIndex++;
			destByteIndex = 0;
		}

		uchar r = rgb[byteIndex++];
		uchar g = rgb[byteIndex++];
		uchar b = rgb[byteIndex++];

		QRgb rgbPixel = qRgb( r, g, b );
		// TODO: use QImage::setPixel()
		// TODO: or even if that is slower, move the line below
		// up to the "destLineIndex++" block
		QRgb *destImageData = reinterpret_cast<QRgb*>(image.scanLine(destLineIndex));
		destImageData[destByteIndex++] = rgbPixel;
	}

	if (tempImage!=nullptr) Imlib_kill_image(myId, tempImage);
#ifdef DEBUG_TIMING
	qDebug() << "render took" << timer.elapsed() << "ms";
#endif
	emit stoppedRendering();
	return image;
}


#if 0
bool KuickImage::smoothResize( int newWidth, int newHeight )
{
	int numPixels = newWidth * newHeight;
	const int NUM_BYTES_NEW  = 3; // 24 bpp
	uchar *newImageData = new uchar[numPixels * NUM_BYTES_NEW];

	// ### endianness
	// myIm : old image, old size


	/////////////////////////////////////////////////
//	int w = myOrigWidth; //myViewport.width();
	//int h = myOrigHeight; //myViewport.height();

	//QImage dst(w, h, myIm->depth(), myIm->numColors(), myIm->bitOrder());

	//QRgb *scanline;

	int basis_ox, basis_oy, basis_xx, basis_yy;

	// ### we only scale with a fixed factor for x and y anyway
	double scalex = newWidth / (double) myOrigWidth;
	double scaley = newHeight / (double) myOrigHeight;

//	basis_ox=(int) (myViewport.left() * 4096.0 / scalex);
//	basis_oy=(int) (myViewport.top() * 4096.0 / scaley);
	basis_ox = 0;
	basis_oy = 0;
	basis_xx = (int) (4096.0 / scalex);
	basis_yy = (int) (4096.0 / scaley);

	//qDebug("Basis: (%d, %d), (%d, 0), (0, %d)", basis_ox, basis_oy, basis_xx, basis_yy);

	int x2, y2;

	int max_x2 = (myOrigWidth << 12);
	int max_y2 = (myOrigHeight << 12);

//	QRgb background = idata->backgroundColor.rgb();

//	QRgb **imdata = (QRgb **) myIm->jumpTable();
//	QRgb *imdata = reinterpret_cast<QRgb*>( myIm->rgb_data );
	uchar *imdata = myIm->rgb_data;


	int y = 0;


//	for (;;)	//fill the top of the target pixmap with the background color
//	{
//		y2 = basis_oy + y * basis_yy;
//
//		if ((y2 >= 0  && (y2 >> 12) < myIm->height()) || y >= h)
//			break;
//
//		scanline = (QRgb*) dst.scanLine(y);
//		for (int i = 0; i < w; i++)
//			*(scanline++) = background; //qRgb(0,255,0);
//		y++;
//	}

	for (; y < newHeight; y++)
	{
//		scanline = (QRgb*) dst.scanLine(y);

		x2 = basis_ox;
		y2 = basis_oy + y * basis_yy;

		if (y2 >= max_y2)
			break;

		int x = 0;

//		while  ((x2 < 0 || (x2 >> 12) >= myIm->width()) && x < w)	//fill the left of the target pixmap with the background color
//		{
//			*(scanline++) = background; //qRgb(0,0,255);
//			x2 += basis_xx;
//			x++;
//		}

		int top = y2 >> 12;
		int bottom = top + 1;
		if (bottom >= myOrigHeight)
			bottom--;

//		for (; x < w; x++)
		for (; x < newWidth; x++) // ### myOrigWidth orig
		{
			int left = x2 >> 12;
			int right = left + 1;

			if (right >= myOrigWidth)
				right = myOrigWidth - 1;

			unsigned int wx = x2  & 0xfff; //12 bits of precision for reasons which will become clear
			unsigned int wy = y2 & 0xfff; //12 bits of precision

			unsigned int iwx = 0xfff - wx;
			unsigned int iwy = 0xfff - wy;

			QRgb tl = 0, tr = 0, bl = 0, br = 0;
			int ind = 0;
			ind = (left + top * myOrigWidth) * 3;
			tl  = (imdata[ind] << 16);
			tl |= (imdata[ind + 1] << 8);
			tl |= (imdata[ind + 2] << 0);
			int bar = imdata[ind + 2] << 8;
			bar = qBlue(bar);

			ind = (right + top * myOrigWidth) * 3;
			tr  = (imdata[ind] << 16);
			tr |= (imdata[ind + 1] << 8);
			tr |= (imdata[ind + 2] << 0);
			bar = imdata[ind + 2] << 8;

			ind = (left + bottom * myOrigWidth) * 3;
			bl  = (imdata[ind] << 16);
			bl |= (imdata[ind + 1] << 8);
			bl |= (imdata[ind + 2] << 0);
			bar = imdata[ind + 2] << 8;

			ind = (right + bottom * myOrigWidth) * 3;
			br  = (imdata[ind] << 16);
			br |= (imdata[ind + 1] << 8);
			br |= (imdata[ind + 2] << 0);
//			tl=imdata[top][left];
//			tr=imdata[top][right];
//			bl=imdata[bottom][left];
//			br=imdata[bottom][right];

			/*
			tl=getValidPixel(myIm, left, top, x, y);		//these calls are expensive
			tr=getValidPixel(myIm, right, top, x, y);		//use them to debug segfaults in this function
			bl=getValidPixel(myIm, left, bottom, x, y);
			br=getValidPixel(myIm, right, bottom, x, y);
			*/

			unsigned int r = (unsigned int) (qRed(tl) * iwx * iwy + qRed(tr) * wx* iwy + qRed(bl) * iwx * wy + qRed(br) * wx * wy); // NB 12+12+8 == 32
			unsigned int g = (unsigned int) (qGreen(tl) * iwx * iwy + qGreen(tr) * wx * iwy + qGreen(bl) * iwx * wy + qGreen(br) * wx * wy);
			unsigned int b = (unsigned int) (qBlue(tl) * iwx * iwy + qBlue(tr) * wx * iwy + qBlue(bl) * iwx * wy + qBlue(br) * wx * wy);

			// ### endianness
			//we're actually off by one in 255 here! (254 instead of 255)
			int foo = r >> 24;
			foo = g >> 24;
			foo = b >> 24;
			newImageData[(y * newWidth * 3) + (x * 3) + 0] = (r >> 24);
			newImageData[(y * newWidth * 3) + (x * 3) + 1] = (g >> 24);
			newImageData[(y * newWidth * 3) + (x * 3) + 2] = (b >> 24);
//			*(scanline++) = qRgb(r >> 24, g >> 24, b >> 24); //we're actually off by one in 255 here

			x2 += basis_xx;

			if (x2 > max_x2)
			{
				x++;
				break;
			}

		}

//		while  (x < w)	//fill the right of each scanline with the background colour
//		{
//			*(scanline++) = background; //qRgb(255,0,0);
//			x++;
//		}
	}

//	for (;;)	//fill the bottom of the target pixmap with the background color
//	{
//		y2 = basis_oy + y * basis_yy;
//
//		if (y >= h)
//			break;
//
//		scanline = (QRgb*) dst.scanLine(y);
//		for (int i = 0; i < w; i++)
//			*(scanline++) = background; //qRgb(255,255,0);
//		y++;
//	}

	// ### keep orig image somewhere but delete all scaled images!
	ImlibImage *newIm = Imlib_create_image_from_data( myId, newImageData, NULL,
                                                      newWidth, newHeight );
    delete[] newImageData;

    if ( newIm )
    {
    	myScaledIm = newIm;
    	myIsDirty = true;
    	myWidth = newWidth;
    	myHeight = newHeight;
    }

    return myIm != 0L;
//	return dst.copy();
}
#endif
