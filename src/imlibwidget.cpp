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

#include "kuickdata.h"

#include <sys/time.h>
#include <unistd.h>

#include <stdlib.h>
#include <assert.h>

#include <qcolor.h>
#include <qfile.h>
#include <qglobal.h>
#include <qimage.h>
#include <qobject.h>
#include <qpalette.h>

#include <kcursor.h>
#include <kdebug.h>
#include <kfilemetainfo.h>
#include <kimageio.h>

#include "filecache.h"
#include "kuickfile.h"
#include "kuickimage.h"
#include "imlibwidget.h"

const int ImlibWidget::ImlibOffset = 256;

ImlibWidget::ImlibWidget( ImData *_idata, QWidget *parent, const char *name ) :
  QWidget( parent, name, WDestructiveClose )
{
    idata 		= _idata;
    deleteImData 	= false;
    deleteImlibData 	= true;

    if ( !idata ) { // if no imlib configuration was given, create one ourself
	idata = new ImData;
	deleteImData = true;
    }

    ImlibInitParams par;

    // PARAMS_PALETTEOVERRIDE taken out because of segfault in imlib :o(
    par.flags = ( PARAMS_REMAP | PARAMS_VISUALID |
		  PARAMS_FASTRENDER | PARAMS_HIQUALITY | PARAMS_DITHER |
		  PARAMS_IMAGECACHESIZE | PARAMS_PIXMAPCACHESIZE );

    Visual* defaultvis = DefaultVisual(x11Display(), x11Screen());

    par.paletteoverride = idata->ownPalette ? 1 : 0;
    par.remap           = idata->fastRemap ? 1 : 0;
    par.fastrender      = idata->fastRender ? 1 : 0;
    par.hiquality       = idata->dither16bit ? 1 : 0;
    par.dither          = idata->dither8bit ? 1 : 0;
    par.visualid        = defaultvis->visualid;
    uint maxcache       = idata->maxCache;

    // 0 == no cache
    par.imagecachesize  = maxcache * 1024;
    par.pixmapcachesize = maxcache * 1024;

    id = Imlib_init_with_params( x11Display(), &par );

    init();
}


ImlibWidget::ImlibWidget( ImData *_idata, ImlibData *_id, QWidget *parent,
			  const char *name )
    : QWidget( parent, name, WDestructiveClose )
{
    id              = _id;
    idata           = _idata;
    deleteImData    = false;
    deleteImlibData = false;

    if ( !idata ) {
	idata = new ImData;
	deleteImData = true;
    }

    init();
}


void ImlibWidget::init()
{
    int w = 1; // > 0 for XCreateWindow
    int h = 1;
    myBackgroundColor = Qt::black;
    m_kuim              = 0L;
    m_kuickFile = 0L;

    if ( !id )
	qFatal("ImlibWidget: Imlib not initialized, aborting.");

    setAutoRender( true );

    setPalette( QPalette( myBackgroundColor ));
    setBackgroundMode( PaletteBackground );

    imageCache = new ImageCache( id, 4 ); // cache 4 images (FIXME?)
    connect( imageCache, SIGNAL( sigBusy() ), SLOT( setBusyCursor() ));
    connect( imageCache, SIGNAL( sigIdle() ), SLOT( restoreCursor() ));

    win = XCreateSimpleWindow(x11Display(), winId(), 0,0,w,h,0,0,0);
}

ImlibWidget::~ImlibWidget()
{
    delete imageCache;
    if ( deleteImlibData && id ) free ( id );
    if ( win ) XDestroyWindow( x11Display(), win );
    if ( deleteImData ) delete idata;
}

KURL ImlibWidget::url() const
{
    if ( m_kuickFile )
        return m_kuickFile->url();

    return KURL();
}

KuickFile * ImlibWidget::currentFile() const
{
    return m_kuickFile;
}

// tries to load "file" and returns the according KuickImage *
// or 0L if unsuccessful
// Note that the given file MUST already be downloaded prior to calling this function
KuickImage * ImlibWidget::loadImageInternal( KuickFile * file )
{
    assert( file->isAvailable() );

    // apply default image modifications
    mod.brightness = idata->brightness + ImlibOffset;
    mod.contrast = idata->contrast + ImlibOffset;
    mod.gamma = idata->gamma + ImlibOffset;

    KuickImage *kuim = imageCache->getKuimage( file, mod );
    if ( !kuim ) {// couldn't load file, maybe corrupt or wrong format
	kdWarning() << "ImlibWidget: can't load image " << file->url().prettyURL() << endl;
	return 0L;
    }

    loaded( kuim ); // maybe upscale/downscale/rotate in subclasses

    return kuim;
}

// overridden in subclass
void ImlibWidget::loaded( KuickImage * )
{
}

bool ImlibWidget::loadImage( const KURL& url )
{
    return loadImage( FileCache::self()->getFile( url ));
}

bool ImlibWidget::loadImage( KuickFile * file )
{
    if ( file->waitForDownload( this ) != KuickFile::OK)
    	return false;
    	
    KuickImage *kuim = loadImageInternal( file );
    // FIXME - check everywhere if we have a kuim or not!

    if ( kuim ) {
	m_kuim = kuim;
	autoUpdate( true ); // -> updateWidget() -> updateGeometry()
	m_kuickFile = file;
        return true;
    }

    return false;
}


bool ImlibWidget::cacheImage( const KURL& url )
{
//    qDebug("cache image: %s", url.url().latin1());
    KuickFile *file = FileCache::self()->getFile( url );
    if ( file->isAvailable() )
        return cacheImage( file );
    else {
        if ( !file->download() ) {
            return false;
        }
        connect( file, SIGNAL( downloaded( KuickFile * )), SLOT( cacheImage( KuickFile * )) );
        return true; // optimistic
    }
}

bool ImlibWidget::cacheImage( KuickFile * file )
{
//    qDebug("cache image: %s", file->url().url().latin1());
    KuickImage *kuim = loadImageInternal( file );
    if ( kuim ) {
        kuim->renderPixmap();
        return true;
    }
    return false;
}


void ImlibWidget::showImage()
{
    XMapWindow( x11Display(), win );
    XSync( x11Display(), False );
}


// -256..256
void ImlibWidget::setBrightness( int factor )
{
    mod.brightness = factor + ImlibOffset;
    setImageModifier();

    autoUpdate();
}


// -256..256
void ImlibWidget::setContrast( int factor )
{
    mod.contrast = factor + ImlibOffset;
    setImageModifier();

    autoUpdate();
}


// -256..256
void ImlibWidget::setGamma( int factor )
{
    mod.gamma = factor + ImlibOffset;
    setImageModifier();

    autoUpdate();
}


Rotation ImlibWidget::rotation() const 
{ 
    return m_kuim ? m_kuim->absRotation() : ROT_0; 
}

FlipMode ImlibWidget::flipMode()  const 
{ 
    return m_kuim ? m_kuim->flipMode() : FlipNone; 
}

void ImlibWidget::zoomImage( float factor )
{
    if ( factor == 1 || factor == 0 || !m_kuim )
	return;

    int newWidth  = (int) (factor * (float) m_kuim->width());
    int newHeight = (int) (factor * (float) m_kuim->height());

    if ( canZoomTo( newWidth, newHeight ) )
    {
        m_kuim->resize( newWidth, newHeight, idata->smoothScale ? KuickImage::SMOOTH : KuickImage::FAST );
        autoUpdate( true );
    }
}

bool ImlibWidget::canZoomTo( int newWidth, int newHeight )
{
    if ( newWidth <= 2 || newHeight <= 2 ) // minimum size for an image is 2x2 pixels
	return false;

    return true;
}


void ImlibWidget::showImageOriginalSize()
{
    if ( !m_kuim )
	return;

    m_kuim->restoreOriginalSize();
    autoUpdate( true );

    showImage();
}

bool ImlibWidget::autoRotate( KuickImage *kuim )
{
    KFileMetaInfo metadatas( kuim->file().localFile() );
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
    rotated( m_kuim, ROT_90 );
    autoUpdate( true );
}

void ImlibWidget::rotate180()
{
    if ( !m_kuim )
	return;

    m_kuim->rotate( ROT_180 );
    rotated( m_kuim, ROT_180 );
    autoUpdate();
}

void ImlibWidget::rotate270()
{
    if ( !m_kuim )
	return;

    m_kuim->rotate( ROT_270 );
    rotated( m_kuim, ROT_270 );
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

//     if ( geometryUpdate )
//         XUnmapWindow( x11Display(), win );// remove the old image -> no flicker

    XSetWindowBackgroundPixmap( x11Display(), win, m_kuim->pixmap() );

    if ( geometryUpdate )
	updateGeometry( m_kuim->width(), m_kuim->height() );

    XClearWindow( x11Display(), win );

    showImage();
}


// here we just use the size of m_kuim, may be overridden in subclass
void ImlibWidget::updateGeometry( int w, int h )
{
    XMoveWindow( x11Display(), win, 0, 0 ); // center?
    XResizeWindow( x11Display(), win, w, h );
    resize( w, h );
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

    Imlib_set_image_modifier( id, m_kuim->imlibImage(), &mod );
    m_kuim->setDirty( true );
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
    if ( cursor().shape() == KCursor::waitCursor().shape() ) // only if nobody changed the cursor in the meantime!
         setCursor( m_oldCursor );
}

// Reparenting a widget in Qt in fact means destroying the old X window of the widget
// and creating a new one. And since the X window used for the Imlib image is a child
// of this widget's X window, destroying this widget's X window would mean also
// destroying the Imlib image X window. Therefore it needs to be temporarily reparented
// away and reparented back to the new X window.
// Reparenting may happen e.g. when doing the old-style (non-NETWM) fullscreen changes.
void ImlibWidget::reparent( QWidget* parent, WFlags f, const QPoint& p, bool showIt )
{
    XWindowAttributes attr;
    XGetWindowAttributes( x11Display(), win, &attr );
    XUnmapWindow( x11Display(), win );
    XReparentWindow( x11Display(), win, attr.root, 0, 0 );
    QWidget::reparent( parent, f, p, showIt );
    XReparentWindow( x11Display(), win, winId(), attr.x, attr.y );
    if( attr.map_state != IsUnmapped )
        XMapWindow( x11Display(), win );
}

void ImlibWidget::rotated( KuickImage *, int )
{
}

//----------


// uhh ugly, we have two lists to map from filename to KuickImage :-/
ImageCache::ImageCache( ImlibData *id, int maxImages )
{
    myId        = id;
    idleCount   = 0;
    myMaxImages = maxImages;
    kuickList.setAutoDelete( true );
    fileList.clear();
    kuickList.clear();
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


KuickImage * ImageCache::getKuimage( KuickFile * file,
				     ImlibColorModifier mod )
{
    if ( !file )
	return 0L;

    assert( file->isAvailable() ); // debug build
    if ( file->waitForDownload( 0L ) != KuickFile::OK ) // and for users
    	return 0L;

    KuickImage *kuim = 0L;
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

        ImlibImage *im = Imlib_load_image( myId,
                                           QFile::encodeName( file->localFile() ).data() );

// #ifndef NDEBUG
//         gettimeofday( &tms2, NULL );
//         qDebug("*** LOADING image: %s, took %ld ms", file.latin1(),
//                (tms2.tv_usec - tms1.tv_usec)/1000);
// #endif	

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
    }

    if ( kuickList.count() > (uint) myMaxImages ) {
//         qDebug(":::: now removing from cache: %s", (*fileList.fromLast()).latin1());
	kuickList.removeLast();
	fileList.remove( fileList.fromLast() );
    }

    return kuim;
}


// Note: the returned image's filename will not be the real filename (which it usually
// isn't anyway, according to Imlib's sources).
ImlibImage * ImageCache::loadImageWithQt( const QString& fileName ) const
{
    kdDebug() << "Trying to load " << fileName << " with KImageIO..." << endl;

    KImageIO::registerFormats();

    QImage image( fileName );
    if ( image.isNull() )
	return 0L;
    if ( image.depth() != 32 ) {
	image.setAlphaBuffer(false);
	image = image.convertDepth(32);

    if ( image.isNull() )
	return 0L;
    }

    // convert to 24 bpp (discard alpha)
    int numPixels = image.width() * image.height();
    const int NUM_BYTES_NEW  = 3; // 24 bpp
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

    return im;
}

#include "imlibwidget.moc"
