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

#include "imlibwidget.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QPalette>
#include <QX11Info>

#include "filecache.h"
#include "kuickdata.h"
#include "kuickfile.h"
#include "kuickimage.h"
#include "kuickshow_debug.h"
#include "imagemods.h"
#include "imagecache.h"


// TODO: can be file-static?
const int ImlibWidget::ImlibOffset = 256;

ImlibWidget::ImlibWidget( ImData *_idata, QWidget *parent )
    : QScrollArea( parent )
{
    idata 		= _idata;
    deleteImData 	= false;
    deleteImlibData 	= true;

    if ( !idata ) { // if no imlib configuration was given, create one ourself
	idata = new ImData;
	deleteImData = true;
    }

    // TODO: maybe this initialisation and 'id'/'myId' can be managed
    // in a singleton class, or at least merged into an utility
    // function. Very similar to KuickShow::initImlib().

    uint maxcache       = idata->maxCache;
#ifdef HAVE_IMLIB1
    ImlibInitParams par;

    par.paletteoverride = idata->ownPalette  ? 1 : 0;
    par.remap           = idata->fastRemap   ? 1 : 0;
    par.fastrender      = idata->fastRender  ? 1 : 0;
    par.hiquality       = idata->dither16bit ? 1 : 0;
    par.dither          = idata->dither8bit  ? 1 : 0;

    par.flags = ( PARAMS_REMAP |
                  PARAMS_FASTRENDER | PARAMS_HIQUALITY | PARAMS_DITHER |
                  PARAMS_IMAGECACHESIZE | PARAMS_PIXMAPCACHESIZE );

    // 0 == no cache
    par.imagecachesize  = maxcache * 1024;
    par.pixmapcachesize = maxcache * 1024;

    id = Imlib_init_with_params(QX11Info::display(), &par);
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
    imlib_set_cache_size(maxcache*1024);
    // Set the maximum number of colours to allocate for 8bpp or less
    imlib_set_color_usage(128);
    // Dither for depths<24bpp
    imlib_context_set_dither(idata->dither8bit  ? 1 : 0);
    // Set the display , visual and colormap we are using
    imlib_context_set_display(QX11Info::display());
#endif // HAVE_IMLIB2

    init();
}


ImlibWidget::ImlibWidget( ImData *_idata, ImlibData *_id, QWidget *parent )
    : QScrollArea( parent )
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
    myLabel = new QLabel(this);
    setWidget(myLabel);
    setWidgetResizable(true);
    // This is required so that the pixmap label fits the window exactly.
    setFrameStyle(QFrame::NoFrame);

    // The image window never showed scroll bars, the image
    // could be scrolled by mouse dragging only.
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    int w = 1; // > 0 for XCreateWindow
    int h = 1;
    myBackgroundColor = Qt::black;
    m_kuim              = 0L;
    m_kuickFile = 0L;

#ifdef HAVE_IMLIB1
    if (id==nullptr) qFatal("ImlibWidget: Imlib not initialized");
#endif // HAVE_IMLIB1

    setAttribute( Qt::WA_DeleteOnClose );
    setAutoRender( true );

    myLabel->setContentsMargins(0, 0, 0, 0);
    myLabel->setPalette( QPalette( myBackgroundColor ));
    myLabel->setBackgroundRole( QPalette::Window );
    myLabel->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

    imageCache = new ImageCache( id, 4 ); // cache 4 images (FIXME?)
    connect( imageCache, SIGNAL( sigBusy() ), SLOT( setBusyCursor() ));
    connect( imageCache, SIGNAL( sigIdle() ), SLOT( restoreCursor() ));

#ifdef HAVE_IMLIB2
    mod = imlib_create_color_modifier();
#endif // HAVE_IMLIB2
}

ImlibWidget::~ImlibWidget()
{
    delete imageCache;
    if ( deleteImlibData && id ) free ( id );
    if ( deleteImData ) delete idata;
#ifdef HAVE_IMLIB2
    imlib_context_set_color_modifier(mod);
    imlib_free_color_modifier();
#endif // HAVE_IMLIB2
}

QUrl ImlibWidget::url() const
{
    if ( m_kuickFile )
        return m_kuickFile->url();

    return QUrl();
}

KuickFile * ImlibWidget::currentFile() const
{
    return m_kuickFile;
}

// tries to load "filename" and returns the according KuickImage *
// or 0L if unsuccessful
KuickImage * ImlibWidget::loadImageInternal( KuickFile * file )
{
    assert( file->isAvailable() );

    // apply default image modifications
/////////////////////////////////////////////////////////////////////////////
//     mod.brightness = idata->brightness + ImlibOffset;
//     mod.contrast = idata->contrast + ImlibOffset;
//     mod.gamma = idata->gamma + ImlibOffset;
/////////////////////////////////////////////////////////////////////////////

    KuickImage *kuim = imageCache->getKuimage( file );
    bool wasCached = true;
    if ( !kuim ) {
    	wasCached = false;
    	kuim = imageCache->loadImage( file, mod );
    }

    if ( !kuim ) {// couldn't load file, maybe corrupt or wrong format
        qWarning("ImlibWidget: can't load image %s", qUtf8Printable(file->url().toDisplayString()));
	return 0L;
    }

    loaded( kuim, wasCached ); // maybe upscale/downscale/rotate in subclasses

    return kuim;
}

// overridden in subclass
void ImlibWidget::loaded( KuickImage *, bool /*wasCached*/ )
{
}

bool ImlibWidget::loadImage( const QUrl& url )
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


bool ImlibWidget::cacheImage( const QUrl& url )
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
    return (kuim!=nullptr);
}


void ImlibWidget::showImage()
{
    show();
}


// -256..256
void ImlibWidget::setBrightness( int factor )
{
/////////////////////////////////////////////////////////////////////////////
//     mod.brightness = factor + ImlibOffset;
//     setImageModifier();
/////////////////////////////////////////////////////////////////////////////

    autoUpdate();
}


// -256..256
void ImlibWidget::setContrast( int factor )
{
/////////////////////////////////////////////////////////////////////////////
//     mod.contrast = factor + ImlibOffset;
//     setImageModifier();
/////////////////////////////////////////////////////////////////////////////

    autoUpdate();
}


// -256..256
void ImlibWidget::setGamma( int factor )
{
/////////////////////////////////////////////////////////////////////////////
//     mod.gamma = factor + ImlibOffset;
//     setImageModifier();
/////////////////////////////////////////////////////////////////////////////

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
    /* KFileMetaInfo disappered in KF5.
     * TODO: find alternative to KFileMetaInfo

    KFileMetaInfo metadatas( kuim->file().localFile() );
    if ( !metadatas.isValid() )
        return false;

    KFileMetaInfoItem metaitem = metadatas.item("Orientation");
    if ( !metaitem.isValid() || metaitem.value().isNull() )
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
    */

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

    myLabel->setPixmap(QPixmap::fromImage(m_kuim->toQImage()));
    if ( geometryUpdate )
	updateGeometry( m_kuim->width(), m_kuim->height() );

    showImage();
}


// here we just use the size of m_kuim, may be overridden in subclass
void ImlibWidget::updateGeometry( int w, int h )
{
    myLabel->resize( w, h );
}


void ImlibWidget::closeEvent( QCloseEvent *e )
{
    e->accept();
    QWidget::closeEvent( e );
}


void ImlibWidget::setBackgroundColor( const QColor& color )
{
    myBackgroundColor = color;
    myLabel->setPalette( QPalette( myBackgroundColor ));
    repaint(); // FIXME - necessary at all?
}

const QColor& ImlibWidget::backgroundColor() const
{
    return myBackgroundColor;
}


void ImlibWidget::setImageModifier()
{
    if (m_kuim==nullptr) return;

#ifdef HAVE_IMLIB1
    Imlib_set_image_modifier(id, m_kuim->imlibImage(), &mod);
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
    imlib_context_set_color_modifier(mod);
#endif // HAVE_IMLIB2

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
    if ( testAttribute( Qt::WA_SetCursor ) )
        m_oldCursor = cursor();
    else
        m_oldCursor = QCursor();

    setCursor( QCursor( Qt::WaitCursor ) );
}

void ImlibWidget::restoreCursor()
{
    if ( cursor().shape() == QCursor(Qt::WaitCursor).shape() ) // only if nobody changed the cursor in the meantime!
    setCursor( m_oldCursor );
}


/*
// Reparenting a widget in Qt in fact means destroying the old X window of the widget
// and creating a new one. And since the X window used for the Imlib image is a child
// of this widget's X window, destroying this widget's X window would mean also
// destroying the Imlib image X window. Therefore it needs to be temporarily reparented
// away and reparented back to the new X window.
// Reparenting may happen e.g. when doing the old-style (non-NETWM) fullscreen changes.
void ImlibWidget::reparent( QWidget* parent, Qt::WFlags f, const QPoint& p, bool showIt )
{
    XWindowAttributes attr;
    XGetWindowAttributes( getX11Display(), win, &attr );
    XUnmapWindow( getX11Display(), win );
    XReparentWindow( getX11Display(), win, attr.root, 0, 0 );
    QWidget::reparent( parent, f, p, showIt );
    XReparentWindow( getX11Display(), win, winId(), attr.x, attr.y );
    if( attr.map_state != IsUnmapped )
        XMapWindow( getX11Display(), win );
}
*/
void ImlibWidget::rotated( KuickImage *, int )
{
}
