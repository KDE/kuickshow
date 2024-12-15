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
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QPalette>

#include "filecache.h"
#include "kuickfile.h"
#include "kuickimage.h"
#include "kuickshow_debug.h"
#include "imagemods.h"
#include "imagecache.h"


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
    m_kuim              = nullptr;
    m_kuickFile = nullptr;
    myUseModifications = true;

    const auto imlib = ImageLibrary::get();
    if (!imlib->isInitialized())
        qFatal("ImlibWidget: ImageLibrary \"%s\" not initialized, aborting.", qPrintable(imlib->getLibraryName()));

    setAttribute( Qt::WA_DeleteOnClose );
    setAutoRender( true );

    myLabel->setContentsMargins(0, 0, 0, 0);
    myLabel->setPalette( QPalette( myBackgroundColor ));
    myLabel->setBackgroundRole( QPalette::Window );
    myLabel->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

    // TODO: ImageCache can also be a global singleton
    imageCache = new ImageCache(4); // cache 4 images (FIXME?)
    connect(imageCache, &ImageCache::sigBusy, this, &ImlibWidget::setBusyCursor);
    connect(imageCache, &ImageCache::sigIdle, this, &ImlibWidget::restoreCursor);
}

ImlibWidget::~ImlibWidget()
{
    delete imageCache;
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
// or nullptr if unsuccessful
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
        const KuickConfig& config = KuickConfig::get();
        stepBrightnessInternal(config.brightness);
        stepContrastInternal(config.contrast);
        stepGammaInternal(config.gamma);
    }

    KuickImage *kuim = imageCache->getKuimage( file );
    bool wasCached = true;
    if ( !kuim ) {
    	wasCached = false;
	kuim = imageCache->loadImage( file, myModifier );
    }

    if ( !kuim ) {// couldn't load file, maybe corrupt or wrong format
        qWarning("ImlibWidget: can't load image %s", qUtf8Printable(file->url().toDisplayString()));
	return nullptr;
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
        connect(file, &KuickFile::downloaded, this, QOverload<KuickFile *>::of(&ImlibWidget::cacheImage));
        return true; // optimistic
    }
}

bool ImlibWidget::cacheImage( KuickFile * file )
{
//    qDebug("cache image: %s", file->url().url().latin1());
    KuickImage *kuim = loadImageInternal( file );
    return (kuim!=nullptr);
}


// These three functions are passed a step value and cumulatively
// update the image modification values.  If the modifier has been
// reset as in loadImageInternal() above, this has the effect of
// setting the absolute values.

void ImlibWidget::stepBrightnessInternal(int b)
{
	myModifier.setBrightness(myModifier.getBrightness() + b);
}


void ImlibWidget::stepContrastInternal(int c)
{
	myModifier.setContrast(myModifier.getContrast() + c);
}


void ImlibWidget::stepGammaInternal(int g)
{
	myModifier.setGamma(myModifier.getGamma() + g);
}


// These three functions are passed step values from the GUI
// via ImageWIndow::moreBrightness() etc.  The basic step up or
// down is KuickConfig::brightnessSteps which is then multiplied
// by KuickConfig::brightnessFactor, with the default values for
// those being 1 and 10 respectively.  There is no GUI for those
// settings, but they could possibly be changed in the application's
// config file.

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
        m_kuim->resize( newWidth, newHeight, KuickConfig::get().smoothScale ? KuickImage::SMOOTH : KuickImage::FAST );
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
}

bool ImlibWidget::autoRotate( KuickImage *kuim )
{
    // TODO: find alternative to KFileMetaInfo (disappered in KF5)

    /*
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

    ImageLibrary::setImageModifiers(m_kuim->imlibImage(), myModifier);
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
    myModifier = ImageModifiers();
}
