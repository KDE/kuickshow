/* This file is part of the KDE project
   Copyright (C) 1998-2003 Carsten Pfeiffer <pfeiffer@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
 */

#include "kuickdata.h"

#include <sys/time.h>
#include <unistd.h>

#include <stdlib.h>

#include <qcolor.h>
#include <qfile.h>
#include <qglobal.h>
#include <qobject.h>
#include <qpalette.h>
#include <qpainter.h>

#include <math.h>

#include <kcursor.h>
#include <kdebug.h>
#include <kfilemetainfo.h>
#include <kimageio.h>
#include <kpixmapio.h>

#include "imlibwidget.h"

ImlibWidget::ImlibWidget( ImData *_idata, QWidget *parent,
			  const char *name )
    : QWidget( parent, name, WDestructiveClose )
{
    idata           = _idata;
    deleteImData    = false;
    needNewCacheImage = false;

    if ( !idata ) {
	idata = new ImData;
	deleteImData = true;
    }

    init();
}

void ImlibWidget::init()
{
    myBackgroundColor = Qt::black;
    m_kuim              = 0L;

    xpos=0; ypos=0;
    
    setAutoRender( true );

    setPalette( QPalette( myBackgroundColor ));
    setBackgroundMode( NoBackground );

    imageCache = new ImageCache( 4 ); // cache 4 images (FIXME?)
    connect( imageCache, SIGNAL( sigBusy() ), SLOT( setBusyCursor() ));
    connect( imageCache, SIGNAL( sigIdle() ), SLOT( restoreCursor() ));
}

ImlibWidget::~ImlibWidget()
{
    delete imageCache;
    if ( deleteImData ) delete idata;
}

KURL ImlibWidget::url() const
{
    KURL url;
    if ( m_filename.at(0) == '/' )
        url.setPath( m_filename );
    else
        url = m_filename;

    return url;
}

// tries to load "filename" and returns the according KuickImage *
// or 0L if unsuccessful
KuickImage * ImlibWidget::loadImageInternal( const QString& filename )
{
    // apply default image modifications
    mod_brightness = idata->brightness;
    mod_contrast = idata->contrast;
    mod_gamma = idata->gamma;

    KuickImage *kuim = imageCache->getKuimage( filename , idata );
    if ( !kuim ) {// couldn't load file, maybe corrupt or wrong format
	kdWarning() << "ImlibWidget: can't load image " << filename << endl;
	return 0L;
    }

    loaded( kuim ); // maybe upscale/downscale/rotate in subclasses

    return kuim;
}

void ImlibWidget::paintEvent(QPaintEvent *e)
{
	QPainter *paint=new QPainter(this);

	e=e;
	
	m_kuim->setViewportSize(size());
	m_kuim->setViewportPosition(QPoint(-xpos, -ypos));
		
	paint->drawPixmap(QPoint(0,0), m_kuim->getQPixmap());
	
	delete paint;
	
	if (needNewCacheImage)
	{
		loadImageInternal(nextImage);
		needNewCacheImage=false;
	}
}

// overridden in subclass
void ImlibWidget::loaded( KuickImage * )
{
}

bool ImlibWidget::loadImage( const QString& filename )
{
    KuickImage *kuim = loadImageInternal( filename );
    // FIXME - check everywhere if we have a kuim or not!

    if ( kuim ) {
	m_kuim = kuim;
	m_kuim->setViewportSize(size());
	autoUpdate( true ); // -> updateWidget() -> updateGeometry()
	m_filename = filename;
        return true;
    }

    return false;
}


bool ImlibWidget::cacheImage( const QString& filename )
{
    nextImage=filename;
    needNewCacheImage=true;
    return true;
}

void ImlibWidget::showImage()
{
	update();
}


// -256..256
void ImlibWidget::setBrightness( int factor )
{
    mod_brightness = factor;
    setImageModifier();

    autoUpdate();
}


// -256..256
void ImlibWidget::setContrast( int factor )
{
    mod_contrast = factor;
    setImageModifier();

    autoUpdate();
}


// -256..256
void ImlibWidget::setGamma( int factor )
{
    mod_gamma = factor;
    setImageModifier();

    autoUpdate();
}


void ImlibWidget::zoomImage( float factor )
{
    if ( factor == 1 || factor == 0 || !m_kuim )
	return;

    float wf, hf;

    wf = (float) m_kuim->width() * factor;
    hf = (float) m_kuim->height() * factor;

    if ( wf <= 2.0 || hf <= 2.0 ) // minimum size for an image is 2x2 pixels
	return;

    m_kuim->resize( (int) wf, (int) hf );
    autoUpdate( true );
}


void ImlibWidget::showImageOriginalSize()
{
    if ( !m_kuim )
	return;

    m_kuim->restoreOriginalSize();
    autoUpdate( true );

}

bool ImlibWidget::autoRotate( KuickImage *kuim )
{
    KFileMetaInfo metadatas( kuim->filename() );
    if ( !metadatas.isValid() )
        return false;

    KFileMetaInfoItem metaitem = metadatas.item("Orientation");
    if ( !metaitem.isValid()
#if QT_VERSION >= 0x030100
        || metaitem.value().isNull()
#endif
        )
        return false;


    switch ( metaitem.value().toInt() )
    {
        //  Orientation:
        //  1:      normal
        //  2:      flipped horizontally
        //  3:      ROT 180
        //  4:      flipped vertically
        //  5:      ROT 90 -> flip horizontally
        //  6:      ROT 90
        //  7:      ROT 90 -> flip vertically
        //  8:      ROT 270

        case 1:
        default:
            kuim->rotateAbs( ROT_0 );
            break;
        case 2:
            kuim->flipAbs( FlipHorizontal );
            break;
        case 3:
            kuim->rotateAbs( ROT_180 );
            break;
        case 4:
            kuim->flipAbs( FlipVertical );
            break;
        case 5:
            kuim->rotateAbs( ROT_90 );
            kuim->flipAbs( FlipHorizontal );
            break;
        case 6:
            kuim->rotateAbs( ROT_90 );
            break;
        case 7:
            kuim->rotateAbs( ROT_90 );
            kuim->flipAbs( FlipVertical );
            break;
        case 8:
            kuim->rotateAbs( ROT_270 );
            break;
    }

    return true;
}


void ImlibWidget::setRotation( Rotation rot )
{
    if ( m_kuim )
    {
        if ( m_kuim->rotateAbs( rot ) )
            autoUpdate( true );
    }
}


// slots connected to Accels and popupmenu
void ImlibWidget::rotate90()
{
    if ( !m_kuim )
	return;

    m_kuim->rotate( ROT_90 );
    autoUpdate( true );
}

void ImlibWidget::rotate180()
{
    if ( !m_kuim )
	return;

    m_kuim->rotate( ROT_180 );
    autoUpdate();
}

void ImlibWidget::rotate270()
{
    if ( !m_kuim )
	return;

    m_kuim->rotate( ROT_270 );
    autoUpdate( true );
}


// should this go into a subclass?
void ImlibWidget::flipHoriz()
{
    if ( !m_kuim )
	return;

    m_kuim->flip( FlipHorizontal );
    autoUpdate();
}

void ImlibWidget::flipVert()
{
    if ( !m_kuim )
	return;

    m_kuim->flip( FlipVertical );
    autoUpdate();
}
// end slots


void ImlibWidget::setFlipMode( int mode )
{
    if ( !m_kuim )
        return;

    if ( m_kuim->flipAbs( mode ) )
	autoUpdate();
}


void ImlibWidget::updateWidget( bool geometryUpdate )
{
    if ( !m_kuim )
	return;
	
    if ( geometryUpdate )
    {
	updateGeometry( m_kuim->width(), m_kuim->height() );
    }
    else showImage();
}

void ImlibWidget::resize(int w, int h)
{
	QWidget::resize(w, h);
	
	if (m_kuim)
	{
		m_kuim->setViewportSize(QSize(w, h));
	}
}

// here we just use the size of m_kuim, may be overridden in subclass
void ImlibWidget::updateGeometry( int w, int h )
{
    bool show=(w == width() && h == height());
    resize( w, h );
    if (show)
    	showImage();
}


void ImlibWidget::closeEvent( QCloseEvent *e )
{
    e->accept();
    QWidget::closeEvent( e );
}


void ImlibWidget::setBackgroundColor( const QColor& color )
{
    myBackgroundColor = color;
    setPalette( QPalette( myBackgroundColor ));
    repaint( false); // FIXME - false? necessary at all?
}

const QColor& ImlibWidget::backgroundColor() const
{
    return myBackgroundColor;
}


void ImlibWidget::setImageModifier()
{
    if ( !m_kuim )
	return;

    m_kuim->setColourTransform(mod_brightness, mod_contrast, mod_gamma);
}

int ImlibWidget::imageWidth() const
{
    return m_kuim ? m_kuim->width() : 0;
}

int ImlibWidget::imageHeight() const
{
    return m_kuim ? m_kuim->height() : 0;
}

void ImlibWidget::setBusyCursor()
{
    if ( ownCursor() )
        m_oldCursor = cursor();
    else
        m_oldCursor = QCursor();

    setCursor( KCursor::waitCursor() );
}

void ImlibWidget::restoreCursor()
{
    setCursor( m_oldCursor );
}

//----------



KuickImage::KuickImage( const QString& filename, QImage* im, ImData * idata)
    : QObject( 0L, 0L )
{
    myFilename = filename;
    myWidth    = im->width();
    myHeight   = im->height();
    myIsDirty  = true;
    
    this->idata=idata;

    /* convert the image to 32-bit which we'll use internally */
    QImage *newim=new QImage();
    
    *newim=im->convertDepth(32);

    mySourceIm  = newim;
    myIm	= new QImage();
    *myIm	= newim->copy();
        
    if (newim != im)
    {
    	delete im;
    }
    
    fastXform(&myIm, FlipNone, ROT_0, idata->brightness, idata->contrast, idata->gamma);
    
    myViewport=QRect(0, 0, myWidth, myHeight);
    
    myOrigWidth  = myWidth;
    myOrigHeight = myHeight;
    myRotation   = ROT_0;
    myFlipMode   = FlipNone;
}

KuickImage::~KuickImage()
{
    delete myIm;
    delete mySourceIm;
}


void KuickImage::resize( int width, int height )
{
    if ( myWidth == width && myHeight == height )
	return;

    myWidth   = width;
    myHeight  = height;
    myIsDirty = true;
}


void KuickImage::restoreOriginalSize()
{
    if (myWidth == myOrigWidth && myHeight == myOrigHeight)
	return;

    myWidth   = myOrigWidth;
    myHeight  = myOrigHeight;
    myIsDirty = true;

    if ( myRotation == ROT_90 || myRotation == ROT_270 )
        qSwap( myWidth, myHeight );
}


QPixmap& KuickImage::getQPixmap()
{
    if ( myIsDirty )
	renderPixmap();

    return myPixmap;
}


void KuickImage::renderPixmap()
{
    if ( !myIsDirty )
	return;

   //  qDebug("### rendering: %s", myFilename.latin1());

    emit startRendering();

#ifndef NDEBUG
     struct timeval tms1, tms2;
     gettimeofday( &tms1, NULL );
#endif
    
    QImage scaled;
    
    //slow, nice way
    //scaled=myIm->smoothScale(myWidth, myHeight);
    
    /* rescale QImage*/
    
    if (myWidth != myOrigWidth || myHeight != myOrigHeight)
    {
    	if (idata->renderQuality >= 1)
    		scaled=smoothTransform();
	else	scaled=fastTransform();
    }
    else scaled=myIm->copy();
    
    
    
    KPixmapIO io;
    
    if (QPixmap::defaultDepth() == scaled.depth())
    	myPixmap=io.convertToPixmap(scaled);
    else
    {
    	switch (idata->renderQuality)
	{
		case 0:
    			myPixmap.convertFromImage(scaled, ThresholdDither);
			break;
			
		case 1:
    			myPixmap.convertFromImage(scaled, OrderedDither);
			break;
			
		default:
		case 2:
    			myPixmap.convertFromImage(scaled, DiffuseDither);
			break;
	}
    }

#ifndef NDEBUG
     gettimeofday( &tms2, NULL );
     qDebug("*** rendering image: %s at %dx%d, took %ld ms", myFilename.latin1(), myWidth, myHeight,
            (tms2.tv_usec - tms1.tv_usec)/1000+(tms2.tv_sec - tms1.tv_sec)*1000);
#endif


    emit stoppedRendering();

    myIsDirty = false;
}

QImage KuickImage::smoothTransform()
{
	int w=myViewport.width();
	int h=myViewport.height();

	QImage dst(myViewport.width(), myViewport.height(), myIm->depth(), myIm->numColors(), myIm->bitOrder());
	
	QRgb *scanline;
	
	int basis_ox, basis_oy, basis_xx, basis_yy;
	
	double scalex=myWidth/(double) myIm->width();
	double scaley=myHeight/(double) myIm->height();
	
	basis_ox=(int) (myViewport.left()*4096.0/scalex);
	basis_oy=(int) (myViewport.top()*4096.0/scaley);
	basis_xx=(int) (4096.0/scalex);
	basis_yy=(int) (4096.0/scaley);

	//qDebug("Basis: (%d, %d), (%d, 0), (0, %d)", basis_ox, basis_oy, basis_xx, basis_yy);
		
	int x2, y2;
	
	int max_x2=(myIm->width()<<12);
	int max_y2=(myIm->height()<<12);
	
	QRgb background=idata->backgroundColor.rgb();
	
	QRgb **imdata=(QRgb **) myIm->jumpTable();
	
	int y=0;
	
	for (;;)	//fill the top of the target pixmap with the background color
	{
		y2=basis_oy+y*basis_yy;
		
		if ((y2 >= 0  && (y2 >> 12) < myIm->height()) || y >= h)
			break;
		
		scanline=(QRgb*) dst.scanLine(y);
		for (int i=0; i < w; i++)
			*(scanline++)=background; //qRgb(0,255,0);
		y++;
	}
	
	for (; y < h; y++)
	{
		scanline=(QRgb*) dst.scanLine(y);
	
		x2=basis_ox;
		y2=basis_oy+y*basis_yy;
	
		if (y2 >= max_y2)
			break;
		
		int x=0;
		
		while  ((x2 < 0 || (x2>>12) >= myIm->width()) && x < w)	//fill the left of the target pixmap with the background color
		{
			*(scanline++)=background; //qRgb(0,0,255);
			x2+=basis_xx;
			x++;
		}
		
		int top=y2 >> 12;
		int bottom=top+1;
		if (bottom >= myIm->height())
			bottom--;						
				
		for (; x < w; x++)
		{
			int left=x2 >> 12;
			
			
			int right=left+1;
			
			if (right >= myIm->width())
				right=myIm->width()-1;

			unsigned int wx=x2  & 0xfff; //12 bits of precision for reasons which will become clear
			unsigned int wy=y2 & 0xfff; //12 bits of precision 
			
			unsigned int iwx=0xfff-wx;
			unsigned int iwy=0xfff-wy;
			
			QRgb tl, tr, bl, br;
			

			tl=imdata[top][left];
			tr=imdata[top][right];
			bl=imdata[bottom][left];
			br=imdata[bottom][right];

			/*			
			tl=getValidPixel(myIm, left, top, x, y);		//these calls are expensive
			tr=getValidPixel(myIm, right, top, x, y);		//use them to debug segfaults in this function
			bl=getValidPixel(myIm, left, bottom, x, y);
			br=getValidPixel(myIm, right, bottom, x, y);
			*/
			
			unsigned int r=(unsigned int) (qRed(tl)*iwx*iwy+qRed(tr)*wx*iwy+qRed(bl)*iwx*wy+qRed(br)*wx*wy); // NB 12+12+8 == 32
			unsigned int g=(unsigned int) (qGreen(tl)*iwx*iwy+qGreen(tr)*wx*iwy+qGreen(bl)*iwx*wy+qGreen(br)*wx*wy);
			unsigned int b=(unsigned int) (qBlue(tl)*iwx*iwy+qBlue(tr)*wx*iwy+qBlue(bl)*iwx*wy+qBlue(br)*wx*wy);
			
			*(scanline++)=qRgb(r >> 24, g >> 24, b >> 24); //we're actually off by one in 255 here
			
			x2+=basis_xx;
			
			if (x2 > max_x2)
			{
				x++;
				break;
			}
				
		}
		
		while  (x < w)	//fill the right of each scanline with the background colour
		{
			*(scanline++)=background; //qRgb(255,0,0);
			x++;
		}
	}
	
	for (;;)	//fill the bottom of the target pixmap with the background color
	{
		y2=basis_oy+y*basis_yy;
		
		if (y >= h)
			break;
		
		scanline=(QRgb*) dst.scanLine(y);
		for (int i=0; i < w; i++)
			*(scanline++)=background; //qRgb(255,255,0);
		y++;
	}
	
	return dst.copy();
}

QImage KuickImage::fastTransform()
{
	int w=myViewport.width();
	int h=myViewport.height();

	QImage dst(myViewport.width(), myViewport.height(), myIm->depth(), myIm->numColors(), myIm->bitOrder());
	
	QWMatrix inverse=myTransform.invert();
	
	QRgb *scanline;
	
	int basis_ox, basis_oy, basis_xx, basis_xy, basis_yx, basis_yy;
	
	double tempx, tempy;
	
	inverse.map(myViewport.left(), myViewport.top(), &tempx, &tempy);
	basis_ox=(int) (tempx*65536.0);
	basis_oy=(int) (tempy*65536.0);
	inverse.map(myViewport.left()+1, myViewport.top(), &tempx, &tempy);
	basis_xx=(int) (tempx*65536.0);
	basis_xy=(int) (tempy*65536.0);
	inverse.map(myViewport.left(), myViewport.top()+1, &tempx, &tempy);
	basis_yx=(int) (tempx*65536.0);
	basis_yy=(int) (tempy*65536.0);
	
	basis_xx-=basis_ox;
	basis_xy-=basis_oy;
	basis_yx-=basis_ox;
	basis_yy-=basis_oy;	
	
	int x2, y2;
	
	for (int y=0; y < h; y++)
	{
		scanline=(QRgb*) dst.scanLine(y);
	
		x2=basis_ox+y*basis_yx;
		y2=basis_oy+y*basis_yy;
		
		for (int x=0; x < w; x++)
		{
			int left=x2 >> 16;
			int top=y2 >> 16;
			/*
			double wx= x2-(double)left;
			double wy= y2-(double)top;
			
			if (wx > 0.5)
				left++;
			if (wy > 0.5)
				top++;*/
			
			*(scanline++)=getValidPixel(myIm, left, top, x, y);
			
			x2+=basis_xx;
			y2+=basis_xy;
		}
	}
	
	return dst.copy();
}

void KuickImage::fastXform(QImage **src, FlipMode flip, Rotation rot, int brightness, int contrast, int gamma)
{
	QImage *im=*src;

	QImage *dst;
	
	int w=im->width();
	int h=im->height();
	
	if ( rot == ROT_90 || rot == ROT_270 )
        	qSwap( w, h );	
	
	dst=new QImage(w, h, im->depth(), im->numColors(), im->bitOrder());
	
	QRgb *scanline;
	
	int x2, y2;

	int basis_ox=0, basis_oy=0, basis_xx=1, basis_xy=0, basis_yx=0, basis_yy=1;
	
	unsigned char colour_lut[256];
	
	float contrast_mult=powf(1.05f, contrast); //contrast is a logarithmic scale, with the base chosen so that -100% to +100% does something sensible
	
	float gamma_pow=powf(1.01622, -gamma); //so gamma_pow should go between 0.2 and 5 corresponding to inputs of -100 to +100 respectively
	
	for (int i=0; i <  256; i++)
	{
		int val=i;
		val=(int)((val-128)*contrast_mult)+128;	
		val=val+(int) (brightness*2.560f);	//so brightness is a percentage between -100% and +100%
		
		val=(int) (255.0f*powf(val/255.0f, gamma_pow));		//gamma is a floating point value greater than zero
		
		//do the clamp
		if (val > 255)
			val=255;
		if (val < 0)
			val=0;
		colour_lut[i]=(unsigned char) val;
	}
	
	switch (rot)
	{
		case ROT_90:
			basis_ox=h-1;
			basis_xx=0;
			basis_xy=1;
			basis_yy=0;
			basis_yx=-1;
			break;
			
		case ROT_180:
			basis_ox=w-1;
			basis_oy=h-1;
			basis_xx=-1;
			basis_yy=-1;
			break;
			
		case ROT_270:
			basis_oy=w-1;
			basis_xx=0;
			basis_xy=-1;
			basis_yy=0;
			basis_yx=1;
			break;
		default:
			break;
	}
	
	switch (flip)
	{
		case FlipHorizontal:
			basis_ox=w-1-basis_ox;
			basis_xx=-basis_xx;
			basis_yx=-basis_yx;
			break;
		
		case FlipVertical:	
			basis_oy=h-1-basis_oy;
			basis_xy=-basis_xy;
			basis_yy=-basis_yy;
			break;
			
		default:
			break;
	}
	
	QRgb **imdata=(QRgb**) im->jumpTable();
	
	for (int y=0; y < h; y++)
	{
		scanline=(QRgb*) dst->scanLine(y);
	
		x2=basis_ox+y*basis_yx;
		y2=basis_oy+y*basis_yy;
		
		for (int x=0; x < w; x++)
		{
			/*
			double wx= x2-(double)left;
			double wy= y2-(double)top;
			
			if (wx > 0.5)
				left++;
			if (wy > 0.5)
				top++;*/
			
			int r, g , b;
				
			QRgb px=imdata[y2][x2];
			//QRgb px=getValidPixel(im, x2, y2, x, y);

			//lookup transformed colour values
						
			r=colour_lut[qRed(px)];
			g=colour_lut[qGreen(px)];
			b=colour_lut[qBlue(px)];
			
			*(scanline++)=qRgb(r, g, b);
			//dst->setPixel(x, y, qRgb(r,g,b));
			
			x2+=basis_xx;
			y2+=basis_xy;
		}
	}
	
	delete im;
	
	*src=dst;
}

QRgb KuickImage::getValidPixel(QImage * im, int x, int y, int dest_x, int dest_y)
{
	if (x < 0 || y < 0 || x >= im->width() || y >= im->height())
	{
		//qDebug("KuickImage::getValidPixel() : (%d,%d) outside of image when rendering pixel (%d,%d)", x, y, dest_x, dest_y);
		return idata->backgroundColor.rgb();
	}
	
	QRgb **jumptable=(QRgb **) im->jumpTable();
	
	return jumptable[y][x];	//im->pixel does unwanted checking
}

void KuickImage::setViewportSize(const QSize &size)
{
	myIsDirty=(size == myViewport.size()) ? myIsDirty : true;
	myViewport.setSize(size);
}

void	KuickImage::setViewportPosition(const QPoint &point)
{
	myIsDirty=(point == myViewport.topLeft()) ? myIsDirty : true;
	myViewport.moveTopLeft(point);
}

void	KuickImage::setViewport(const QRect &vp)
{
	myIsDirty=(vp == myViewport) ? myIsDirty : true;
	myViewport=vp;
}


/** Rotates the source QImage
  */
void KuickImage::rotate( Rotation rot )
{

    if ( rot == ROT_180 ) { 		// rotate 180 degrees
	fastXform(&myIm, FlipNone, ROT_180, 0, 0, 0);
    }

    else if ( rot == ROT_90 || rot == ROT_270 ) {
	qSwap( myWidth, myHeight );

	fastXform(&myIm, FlipNone, rot, 0, 0, 0);
    }

    myRotation = (Rotation) ((myRotation + rot) % 4);
    
    myIsDirty = true;
}

void KuickImage::setColourTransform(int bright, int contrast, int gamma)
{
	*myIm=mySourceIm->copy();
	fastXform(&myIm, myFlipMode, myRotation, bright, contrast, gamma);
	myIsDirty=true;
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

    fastXform(&myIm, flipMode, ROT_0, 0, 0, 0);

    //TODO: modify the position of the viewport
    
    myFlipMode = (FlipMode) (myFlipMode ^ flipMode);
    myIsDirty = true;
}

bool KuickImage::flipAbs( int mode )
{
    if ( myFlipMode == mode )
	return false;

    bool changed = false;

    if ( ((myFlipMode & FlipHorizontal) && !(mode & FlipHorizontal)) ||
	 (!(myFlipMode & FlipHorizontal) && (mode & FlipHorizontal)) ) {
		fastXform(&myIm, FlipHorizontal, ROT_0, 0, 0, 0);
	changed = true;
    }

    if ( ((myFlipMode & FlipVertical) && !(mode & FlipVertical)) ||
	 (!(myFlipMode & FlipVertical) && (mode & FlipVertical)) ) {
		fastXform(&myIm, FlipVertical, ROT_0, 0, 0, 0);
	changed = true;
    }

    //TODO: modify the position of the viewport
    
    if ( changed ) {
        myFlipMode = (FlipMode) mode;
        myIsDirty = true;
        return true;
    }

    return false;
}


//----------



// uhh ugly, we have two lists to map from filename to KuickImage :-/
ImageCache::ImageCache( int maxImages )
{
    idleCount   = 0;
    myMaxImages = maxImages;
    kuickList.setAutoDelete( true );
    fileList.clear();
    kuickList.clear();
    
    KImageIO::registerFormats();
}


ImageCache::~ImageCache()
{
    kuickList.clear();
    fileList.clear();
}


void ImageCache::setMaxImages( int maxImages )
{
    myMaxImages = maxImages;
    int count   = kuickList.count();
    while ( count > myMaxImages ) {
	kuickList.removeLast();
	fileList.remove( fileList.fromLast() );
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


KuickImage * ImageCache::getKuimage( const QString& file, ImData *idata )
{
    KuickImage *kuim = 0L;
    if ( file.isEmpty() )
	return 0L;

    int index = fileList.findIndex( file );
    if ( index != -1 ) {
        if ( index == 0 )
            kuim = kuickList.at( 0 );

        // need to reorder the lists, otherwise we might delete the current
        // image when a new one is cached and the current one is the last!
        else {
            kuim = kuickList.take( index );
            kuickList.insert( 0, kuim );
            fileList.remove( file );
            fileList.prepend( file );
        }

        return kuim;
    }

    if ( !kuim ) {
	slotBusy();

// #ifndef NDEBUG
//         struct timeval tms1, tms2;
//         gettimeofday( &tms1, NULL );
// #endif

        QImage *im = new QImage(); //create null
	bool loadsuccess=im->load(file);

// #ifndef NDEBUG
//         gettimeofday( &tms2, NULL );
//         qDebug("*** LOADING image: %s, took %ld ms", file.latin1(),
//                (tms2.tv_usec - tms1.tv_usec)/1000);
// #endif	

        slotIdle();
	if ( !loadsuccess )
	    return 0L;

	kuim = new KuickImage( file, im, idata );
	connect( kuim, SIGNAL( startRendering() ),   SLOT( slotBusy() ));
	connect( kuim, SIGNAL( stoppedRendering() ), SLOT( slotIdle() ));

	kuickList.insert( 0, kuim );
	fileList.prepend( file );
    }

    if ( kuickList.count() > (uint) myMaxImages ) {
//         qDebug(":::: now removing from cache: %s", (*fileList.fromLast()).latin1());
	kuickList.removeLast();
	fileList.remove( fileList.fromLast() );
    }

    return kuim;
}


/*
KuickImage * ImageCache::find( const QString& file )
{
  KuickImage *kuim = 0L;
  int index = fileList.findIndex( file );
  if ( index >= 0 )
    kuim = kuickList.at( index );

  qDebug("found cached KuickImage? : %p", kuim );
  return kuim;
}
*/
#include "imlibwidget.moc"

