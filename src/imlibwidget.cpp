/***************************************************************************
** $Id$
**
** ImlibWidget: maps an Xlib window with Imlib's contents on a QWidget
**		can zoom and modify (brighten, ...) images inside the QWidget
**
** Created : 98
**
** Copyright (C) 1998, 1999 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/

#include "kuickdata.h"

#include <sys/time.h>
#include <unistd.h>

#include <stdlib.h>

#include <qcolor.h>
#include <qfile.h>
#include <qglobal.h>
#include <qobject.h>
#include <qpalette.h>

#include <kcursor.h>
#include <kdebug.h>
#include <kfilemetainfo.h>

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
    par.flags = ( PARAMS_REMAP |
		  PARAMS_FASTRENDER | PARAMS_HIQUALITY | PARAMS_DITHER |
		  PARAMS_IMAGECACHESIZE | PARAMS_PIXMAPCACHESIZE );

    par.paletteoverride = idata->ownPalette ? 1 : 0;
    par.remap           = idata->fastRemap ? 1 : 0;
    par.fastrender      = idata->fastRender ? 1 : 0;
    par.hiquality       = idata->dither16bit ? 1 : 0;
    par.dither          = idata->dither8bit ? 1 : 0;
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
    myFlipMode        = FlipNone;
    myBackgroundColor = Qt::black;
    m_kuim              = 0L;

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
    if ( deleteImlibData ) delete id;
    if ( win ) XDestroyWindow( x11Display(), win );
    if ( deleteImData ) delete idata;
    delete imageCache;
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
    mod.brightness = idata->brightness + ImlibOffset;
    mod.contrast = idata->contrast + ImlibOffset;
    mod.gamma = idata->gamma + ImlibOffset;

    KuickImage *kuim = imageCache->getKuimage( filename, mod );
    if ( !kuim ) {// couldn't load file, maybe corrupt or wrong format
	kdWarning() << "ImlibWidget: can't load image " << filename << endl;
	return 0L;
    }

    if ( kdata->autoRotation )
    {
        KFileMetaInfo metadatas( filename );
        KFileMetaInfoItem metaitem = metadatas.item("Orientation");
        if ( metaitem.isValid() )
        {
#if QT_VERSION >= 0x030100
            if ( !metaitem.value().isNull() )
#endif
            {
                switch ( metaitem.value().toInt() )
                {
                    case 1:
                    default:
                        kuim->rotateAbs( ROT_0 );
                        break;
                    case 6:
                        kuim->rotateAbs( ROT_90 );
                        break;
                    case 8:
                        kuim->rotateAbs( ROT_270 );
                        break;
                }
            }
        }
    }

    loaded( kuim ); // maybe upscale/downscale in subclasses

    return kuim;
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
	autoUpdate( true ); // -> updateWidget() -> updateGeometry()
	m_filename = filename;
        return true;
    }

    return false;
}


bool ImlibWidget::cacheImage( const QString& filename )
{
    KuickImage *kuim = loadImageInternal( filename );
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

    showImage();
}


void ImlibWidget::setRotation( Rotation rot )
{
    if ( m_kuim )
        m_kuim->rotateAbs( rot );

    autoUpdate( true );
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


void ImlibWidget::setFlipMode( FlipMode mode )
{
    if ( !m_kuim )
	return;

    bool changed = false;

    if ( (myFlipMode & FlipHorizontal) && !(mode & FlipHorizontal) ||
	 (!(myFlipMode & FlipHorizontal) && mode & FlipHorizontal) ) {
	Imlib_flip_image_horizontal( id, m_kuim->imlibImage() );
	changed = true;
    }

    if ( (myFlipMode & FlipVertical) && (mode & ~FlipVertical) ||
	 ((myFlipMode & ~FlipVertical) && mode & FlipVertical) ) {
	Imlib_flip_image_vertical( id, m_kuim->imlibImage() );
	changed = true;
    }

    if ( changed ) {
	m_kuim->setDirty( true );
	autoUpdate();
	myFlipMode = mode;
    }
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
    setCursor( KCursor::waitCursor() );
}

void ImlibWidget::restoreCursor()
{
    setCursor( KCursor::arrowCursor() );
}

//----------



KuickImage::KuickImage( const QString& filename, ImlibImage *im, ImlibData *id)
    : QObject( 0L, 0L )
{
    myFilename = filename;
    myIm       = im;
    myId       = id;
    myPixmap   = 0L;
    myWidth    = im->rgb_width;
    myHeight   = im->rgb_height;
    myIsDirty  = true;

    myOrigWidth  = myWidth;
    myOrigHeight = myHeight;
    myRotation   = ROT_0;
}

KuickImage::~KuickImage()
{
    Imlib_free_pixmap( myId, myPixmap );
    Imlib_destroy_image( myId, myIm );
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


Pixmap& KuickImage::pixmap()
{
    if ( myIsDirty )
	renderPixmap();

    return myPixmap;
}


void KuickImage::renderPixmap()
{
    if ( !myIsDirty )
	return;

//     qDebug("### rendering: %s", myFilename.latin1());

    if ( myPixmap )
	Imlib_free_pixmap( myId, myPixmap );

    emit startRendering();

// #ifndef NDEBUG
//     struct timeval tms1, tms2;
//     gettimeofday( &tms1, NULL );
// #endif

    Imlib_render( myId, myIm, myWidth, myHeight );
    myPixmap = Imlib_move_image( myId, myIm );

// #ifndef NDEBUG
//     gettimeofday( &tms2, NULL );
//     qDebug("*** rendering image: %s, took %ld ms", myFilename.latin1(),
//            (tms2.tv_usec - tms1.tv_usec)/1000);
// #endif


    emit stoppedRendering();

    myIsDirty = false;
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

    myRotation = (Rotation) ((myRotation + rot) % 4);
    myIsDirty = true;
}


void KuickImage::flip( FlipMode flipMode )
{
    if ( flipMode & FlipHorizontal )
	Imlib_flip_image_horizontal( myId, myIm );
    if ( flipMode & FlipVertical )
	Imlib_flip_image_vertical( myId, myIm );

    myIsDirty = true;
}

void KuickImage::rotateAbs( Rotation rot )
{
    if ( myRotation == rot )
	return;

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


KuickImage * ImageCache::getKuimage( const QString& file,
				     ImlibColorModifier mod )
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

        ImlibImage *im = Imlib_load_image( myId,
                                           QFile::encodeName( file ).data() );

// #ifndef NDEBUG
//         gettimeofday( &tms2, NULL );
//         qDebug("*** LOADING image: %s, took %ld ms", file.latin1(),
//                (tms2.tv_usec - tms1.tv_usec)/1000);
// #endif	

        slotIdle();
	if ( !im )
	    return 0L;

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
