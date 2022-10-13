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
#include "imlibparams.h"

#ifdef HAVE_IMLIB2
#include <math.h>
#endif // HAVE_IMLIB2


static const int ImlibOffset = 256;
static const int ImlibLimit = 256;


ImlibWidget::ImlibWidget(QWidget *parent)
    : QScrollArea(parent)
{
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

    myBackgroundColor = Qt::black;
    m_kuim              = 0L;
    m_kuickFile = 0L;
    myUseModifications = true;

#ifdef HAVE_IMLIB1
    if (ImlibParams::imlibData()==nullptr) qFatal("Imlib not initialised");
#endif // HAVE_IMLIB1

    setAttribute( Qt::WA_DeleteOnClose );
    setAutoRender( true );

    myLabel->setContentsMargins(0, 0, 0, 0);
    myLabel->setPalette( QPalette( myBackgroundColor ));
    myLabel->setBackgroundRole( QPalette::Window );
    myLabel->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

    // TODO: ImageCache can also be a global singleton
    imageCache = new ImageCache(4); // cache 4 images (FIXME?)
    connect( imageCache, SIGNAL( sigBusy() ), SLOT( setBusyCursor() ));
    connect( imageCache, SIGNAL( sigIdle() ), SLOT( restoreCursor() ));

#ifdef HAVE_IMLIB2
    myModifier = imlib_create_color_modifier();
#endif // HAVE_IMLIB2
}

ImlibWidget::~ImlibWidget()
{
    delete imageCache;
#ifdef HAVE_IMLIB2
    imlib_context_set_color_modifier(myModifier);
    imlib_free_color_modifier();
#endif // HAVE_IMLIB2
}


QSize ImlibWidget::sizeHint() const
{
    return (myLabel->size());
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

    // Set the configured default image modification values,
    // unless the 'myUseModifications' option is not set.  This
    // will only be the case for the unmodified image preview
    // in the "Modifications" settings dialogue.
    initModifications();
    if (myUseModifications)
    {
        stepBrightnessInternal(ImlibParams::imlibConfig()->brightness);
        stepContrastInternal(ImlibParams::imlibConfig()->contrast);
        stepGammaInternal(ImlibParams::imlibConfig()->gamma);
    }

    KuickImage *kuim = imageCache->getKuimage( file );
    bool wasCached = true;
    if ( !kuim ) {
    	wasCached = false;
	kuim = imageCache->loadImage( file, myModifier );
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


// These three functions are passed a step value and cumulatively
// update the image modification values.  If the modifier has been
// reset as in loadImageInternal() above, this has the effect of
// setting the absolute values.
//
// Imlib2's colour modifications are cumulative, there is no way
// to set an absolute value without knowing the current value and
// no way to read the current value.  Therefore, this is the only
// way to actually set an absolute value.

void ImlibWidget::stepBrightnessInternal(int b)
{
     if (b==0) return;							// no change
     b = qBound(-ImlibLimit, b, +ImlibLimit);				// enforce limits

#ifdef HAVE_IMLIB1
    // ImlibOffset has already been added when the modifier
    // was initialised in loadImageInternal().
    myModifier.brightness += b;
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
    // The value passed to imlib_modify_color_modifier_brightness()
    // is cumulative.  Brightness values of 0 do not affect anything.
    // -1.0 will make things completely black and 1.0 will make things
    // all white.  Values in between vary brightness linearly.
    //
    // As opposed to contrast and gamma below, the cumulative effects
    // of brightness appear to be additive.  Therefore, the brightness
    // step is chosen to again take the same number of steps as for
    // Imlib1 to reach the limits as above.
    imlib_context_set_color_modifier(myModifier);
    imlib_modify_color_modifier_brightness(b/double(ImlibLimit));
#endif // HAVE_IMLIB2
    // TODO: for Qt only
}


void ImlibWidget::stepContrastInternal(int c)
{
    if (c==0) return;							// no change
    c = qBound(-ImlibLimit, c, +ImlibLimit);				// enforce limits

#ifdef HAVE_IMLIB1
    // See ImlibWidget::stepBrightness() above.
    myModifier.contrast += c;
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
    // The API documentation for imlib_modify_color_modifier_contrast()
    // is confusing.  It says:
    //
    //   "Modifies the current color modifier by adjusting the contrast
    //   by the value 'contrast_value'. The color modifier is modified
    //   not set, so calling this repeatedly has cumulative effects.
    //   Contrast of 1.0 does nothing. 0.0 will merge to gray, 2.0 will
    //   double contrast etc."
    //
    // The "cumulative" effects do not seem to be additive:  for example,
    // assuming that the contrast starts off at 1.0 as was explicitly set
    // in loadImageInternal() above, it would be logical to assume that a
    // subsequent step of 0.1 would make the new effective contrast 1.1 -
    // that is, slightly greater than normal.  However, the effect of this
    // value is actually to set the contrast very low (almost totally grey).
    // To achieve a contrast step of 0.1 the value here needs to be set to
    // 1.1, and for the other direction (less contrast) it needs to be
    // 1/1.1 (approximately 0.91).  Perhaps what is happening is that the
    // value multiplies the current contrast instead of adding to it.
    //
    // Working on this assumption, under Imlib1 the standard step of 10
    // would have to be applied about 25 times to bring the contrast up
    // to the limit of 256.  Therefore, the Imlib2 step is calculated to
    // require the same number of steps to bring the contrast up to 2.0,
    // although it is not certain whether this is a hard limit or what
    // happens if it is exceeded.
    double cc = pow(2, double(c)/ImlibLimit);
    imlib_context_set_color_modifier(myModifier);
    imlib_modify_color_modifier_contrast(cc);
#endif // HAVE_IMLIB2
    // TODO: for Qt only
}


void ImlibWidget::stepGammaInternal(int g)
{
    if (g==0) return;							// no change
    g = qBound(-ImlibLimit, g, +ImlibLimit);				// enforce limits

#ifdef HAVE_IMLIB1
    // See ImlibWidget::stepBrightness() above.
    myModifier.gamma += g;
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
    // The API documentation for imlib_modify_color_modifier_gamma()
    // says:
    //
    //   "Modifies the current color modifier by adjusting the gamma by
    //   the value specified 'gamma_value'.  The color modifier is modified
    //   not set, so calling this repeatedly has cumulative effects.  A
    //   gamma of 1.0 is normal linear, 2.0 brightens and 0.5 darkens etc."
    //
    // The cumulative effect again seems to be multiplicative, see the
    // comments in stepContrastInternal() above.
    double gg = pow(2, double(g)/ImlibLimit);
    imlib_context_set_color_modifier(myModifier);
    imlib_modify_color_modifier_gamma(gg);
#endif // HAVE_IMLIB2
    // TODO: for Qt only
}


// These three functions are passed step values from the GUI
// via ImageWIndow::moreBrightness() etc.  The basic step up or
// down is KuickData::brightnessSteps which is then multiplied
// here by ImData::brightnessFactor, with the default values for
// those being 1 and 10 respectively.  There is no GUI for those
// settings, so it is reasonable to assume that the step will
// be either +10 or -10.

void ImlibWidget::stepBrightness(int b)
{
    stepBrightnessInternal(b);
    setImageModifier();
    autoUpdate();
}

void ImlibWidget::stepContrast(int c)
{
    stepContrastInternal(c);
    setImageModifier();
    autoUpdate();
}

void ImlibWidget::stepGamma(int g)
{
    stepGammaInternal(g);
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

    int newWidth  = static_cast<int>(factor*m_kuim->width());
    int newHeight = static_cast<int>(factor*m_kuim->height());

    if ( canZoomTo( newWidth, newHeight ) )
    {
        m_kuim->resize( newWidth, newHeight, ImlibParams::imlibConfig()->smoothScale ? KuickImage::SMOOTH : KuickImage::FAST );
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


void ImlibWidget::updateWidget(bool geometryUpdate)
{
    if (m_kuim==nullptr) return;

    myLabel->setPixmap(QPixmap::fromImage(m_kuim->toQImage()));
    if (geometryUpdate) updateGeometry(m_kuim->width(), m_kuim->height());
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
    Imlib_set_image_modifier(id, m_kuim->imlibImage(), &myModifier);
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
    imlib_context_set_color_modifier(myModifier);
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


void ImlibWidget::setUseModifications(bool enable)
{
    qDebug() << enable;
    myUseModifications = enable;
}


void ImlibWidget::initModifications()
{
    // Start with the default image modifications
#ifdef HAVE_IMLIB1
    myModifier.brightness = ImlibOffset;
    myModifier.contrast = ImlibOffset;
    myModifier.gamma = ImlibOffset;
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
    imlib_context_set_color_modifier(myModifier);
    imlib_reset_color_modifier();

    imlib_modify_color_modifier_brightness(0.0);	// initial default values
    imlib_modify_color_modifier_contrast(1.0);
    imlib_modify_color_modifier_gamma(1.0);
#endif // HAVE_IMLIB2
}
