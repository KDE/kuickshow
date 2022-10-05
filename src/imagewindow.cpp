/* This file is part of the KDE project
   Copyright (C) 1998-2006 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include "imagewindow.h"

#include <KActionCollection>
#include <KCursor>
#include <KIconLoader>
#include <KIO/StoredTransferJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPropertiesDialog>
#include <KStandardAction>
#include <KStandardGuiItem>
#include <KStandardShortcut>
#include <KToggleFullScreenAction>
#include <KUrlMimeData>
#include <KWindowSystem>

#include <QApplication>
#include <QBitmap>
#include <QCheckBox>
#include <QContextMenuEvent>
#include <QCursor>
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFocusEvent>
#include <QGridLayout>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QRect>
#include <QResizeEvent>
#include <QScopedPointer>
#include <QScrollBar>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>
#include <QTimer>
#include <QUrl>
#include <QWheelEvent>

#include <netwm.h>
#include <stdlib.h>

#include "filecache.h"
#include "imagemods.h"
#include "kuick.h"
#include "kuickdata.h"
#include "kuickimage.h"
#include "printing.h"
#include "imagecache.h"


QCursor *ImageWindow::s_handCursor = 0L;

ImageWindow::ImageWindow( ImData *_idata, ImlibData *id, QWidget *parent )
    : ImlibWidget( _idata, id, parent )
{
    init();
}

ImageWindow::ImageWindow( ImData *_idata, QWidget *parent )
    : ImlibWidget( _idata, parent )
{
    init();
}

ImageWindow::~ImageWindow()
{
}


void ImageWindow::init()
{
    setFocusPolicy( Qt::StrongFocus );

    KCursor::setAutoHideCursor( this, true, true );
    KCursor::setHideCursorDelay( 1500 );

    // This image window will automatically be given
    // a distinctive WM_CLASS by Qt.

    viewerMenu = 0L;
    gammaMenu = 0L;
    brightnessMenu = 0L;
    contrastMenu = 0L;


    m_actions = new KActionCollection( this );
    m_actions->addAssociatedWidget( this );

    if ( !s_handCursor ) {
        QString file = QStandardPaths::locate(QStandardPaths::AppDataLocation, "pics/handcursor.png");
        if ( !file.isEmpty() )
            s_handCursor = new QCursor( QPixmap(file) );
        else
            s_handCursor = new QCursor( Qt::ArrowCursor );
    }

    setupActions();
    imageCache->setMaxImages( kdata->maxCachedImages );

    transWidget    = 0L;
    myIsFullscreen = false;

    xpos = 0, ypos = 0;

    setAcceptDrops( true );
    setBackgroundColor( kdata->backgroundColor );

    // TODO: static non-POD data - is this advisable?
    static QPixmap imageIcon = KIconLoader::global()->loadIcon("imageviewer-medium", KIconLoader::User);
    static QPixmap miniImageIcon = KIconLoader::global()->loadIcon( "imageviewer-small",KIconLoader::User);
    KWindowSystem::setIcons( winId(), imageIcon, miniImageIcon );
}

void ImageWindow::updateActions()
{
    m_actions->readSettings();
}

void ImageWindow::setupActions()
{
    QAction *nextImage = m_actions->addAction( "next_image" );
    nextImage->setText( i18n("Show Next Image") );
    m_actions->setDefaultShortcuts(nextImage, KStandardShortcut::next());
    connect( nextImage, SIGNAL( triggered() ), this, SLOT( slotRequestNext() ) );

    QAction* showPreviousImage = m_actions->addAction( "previous_image" );
    showPreviousImage->setText( i18n("Show Previous Image") );
    m_actions->setDefaultShortcuts(showPreviousImage, KStandardShortcut::prior());
    connect( showPreviousImage, SIGNAL( triggered() ), this, SLOT( slotRequestPrevious() ) );

    QAction* deleteImage = m_actions->addAction( "delete_image" );
    deleteImage->setText( i18n("Delete Image") );
    m_actions->setDefaultShortcut(deleteImage, QKeySequence(Qt::ShiftModifier | Qt::Key_Delete));
    connect( deleteImage, SIGNAL( triggered() ), this, SLOT( imageDelete() ) );

    QAction *trashImage = m_actions->addAction( "trash_image" );
    trashImage->setText( i18n("Move Image to Trash") );
    m_actions->setDefaultShortcut(trashImage, Qt::Key_Delete);
    connect( trashImage, SIGNAL( triggered() ), this, SLOT( imageTrash() ) );


    QAction* zoomIn = KStandardAction::zoomIn( this, SLOT( zoomIn() ), m_actions );
    m_actions->setDefaultShortcut(zoomIn, Qt::Key_Plus);
    m_actions->addAction( "zoom_in",  zoomIn );

    QAction *zoomOut = KStandardAction::zoomOut( this, SLOT( zoomOut() ), m_actions );
    m_actions->setDefaultShortcut(zoomOut, Qt::Key_Minus);
    m_actions->addAction( "zoom_out",  zoomOut );

    QAction *restoreSize = m_actions->addAction( "original_size" );
    restoreSize->setText( i18n("Restore Original Size") );
    m_actions->setDefaultShortcut(restoreSize, Qt::Key_O);
    connect( restoreSize, SIGNAL( triggered() ), this, SLOT( showImageOriginalSize() ) );

    QAction *maximize = m_actions->addAction( "maximize" );
    maximize->setText( i18n("Maximize") );
    m_actions->setDefaultShortcut(maximize, Qt::Key_M);
    connect( maximize, SIGNAL( triggered() ), this, SLOT( maximize() ) );

    QAction *rotate90 = m_actions->addAction( "rotate90" );
    rotate90->setText( i18n("Rotate 90 Degrees") );
    m_actions->setDefaultShortcut(rotate90, Qt::Key_9);
    connect( rotate90, SIGNAL( triggered() ), this, SLOT( rotate90() ) );

    QAction *rotate180 = m_actions->addAction( "rotate180" );
    rotate180->setText( i18n("Rotate 180 Degrees") );
    m_actions->setDefaultShortcut(rotate180, Qt::Key_8);
    connect( rotate180, SIGNAL( triggered() ), this, SLOT( rotate180() ) );

    QAction *rotate270 = m_actions->addAction( "rotate270" );
    rotate270->setText( i18n("Rotate 270 Degrees") );
    m_actions->setDefaultShortcut(rotate270, Qt::Key_7);
    connect( rotate270, SIGNAL( triggered() ), this, SLOT( rotate270() ) );

    QAction *flipHori = m_actions->addAction( "flip_horicontally" );
    flipHori->setText( i18n("Flip Horizontally") );
    m_actions->setDefaultShortcut(flipHori, Qt::Key_Asterisk);
    connect( flipHori, SIGNAL( triggered() ), this, SLOT( flipHoriz() ) );

    QAction *flipVeri = m_actions->addAction( "flip_vertically" );
    flipVeri->setText( i18n("Flip Vertically") );
    m_actions->setDefaultShortcut(flipVeri, Qt::Key_Slash);
    connect( flipVeri, SIGNAL( triggered() ), this, SLOT( flipVert() ) );

    QAction *printImage = m_actions->addAction( "print_image" );
    printImage->setText( i18n("Print Image...") );
    m_actions->setDefaultShortcuts(printImage, KStandardShortcut::print());
    connect( printImage, SIGNAL( triggered() ), this, SLOT( printImage() ) );

    QAction *a =  KStandardAction::saveAs( this, SLOT( saveImage() ), m_actions);
    m_actions->addAction( "save_image_as",  a );

    a = KStandardAction::close( this, SLOT( close() ), m_actions);
    m_actions->addAction( "close_image", a );
    // --------
    QAction *moreBrighteness = m_actions->addAction( "more_brightness" );
    moreBrighteness->setText( i18n("More Brightness") );
    m_actions->setDefaultShortcut(moreBrighteness, Qt::Key_B);
    connect( moreBrighteness, SIGNAL( triggered() ), this, SLOT( moreBrightness() ) );

    QAction *lessBrightness = m_actions->addAction( "less_brightness" );
    lessBrightness->setText(  i18n("Less Brightness") );
    m_actions->setDefaultShortcut(lessBrightness, Qt::SHIFT + Qt::Key_B);
    connect( lessBrightness, SIGNAL( triggered() ), this, SLOT( lessBrightness() ) );

    QAction *moreContrast = m_actions->addAction( "more_contrast" );
    moreContrast->setText( i18n("More Contrast") );
    m_actions->setDefaultShortcut(moreContrast, Qt::Key_C);
    connect( moreContrast, SIGNAL( triggered() ), this, SLOT( moreContrast() ) );

    QAction *lessContrast = m_actions->addAction( "less_contrast" );
    lessContrast->setText( i18n("Less Contrast") );
    m_actions->setDefaultShortcut(lessContrast, Qt::SHIFT + Qt::Key_C);
    connect( lessContrast, SIGNAL( triggered() ), this, SLOT( lessContrast() ) );

    QAction *moreGamma = m_actions->addAction( "more_gamma" );
    moreGamma->setText( i18n("More Gamma") );
    m_actions->setDefaultShortcut(moreGamma, Qt::Key_G);
    connect( moreGamma, SIGNAL( triggered() ), this, SLOT( moreGamma() ) );

    QAction *lessGamma = m_actions->addAction( "less_gamma" );
    lessGamma->setText( i18n("Less Gamma") );
    m_actions->setDefaultShortcut(lessGamma, Qt::SHIFT + Qt::Key_G);
    connect( lessGamma, SIGNAL( triggered() ), this, SLOT( lessGamma() ) );

    // --------
    QAction *scrollUp = m_actions->addAction( "scroll_up" );
    scrollUp->setText( i18n("Scroll Up") );
    m_actions->setDefaultShortcut(scrollUp, Qt::Key_Up);
    connect( scrollUp, SIGNAL( triggered() ), this, SLOT( scrollUp() ) );

    QAction *scrollDown = m_actions->addAction( "scroll_down" );
    scrollDown->setText( i18n("Scroll Down") );
    m_actions->setDefaultShortcut(scrollDown, Qt::Key_Down);
    connect( scrollDown, SIGNAL( triggered() ), this, SLOT( scrollDown() ) );

    QAction *scrollLeft = m_actions->addAction( "scroll_left" );
    scrollLeft->setText( i18n("Scroll Left") );
    m_actions->setDefaultShortcut(scrollLeft, Qt::Key_Left);
    connect( scrollLeft, SIGNAL( triggered() ), this, SLOT( scrollLeft() ) );

    QAction *scrollRight = m_actions->addAction( "scroll_right" );
    scrollRight->setText( i18n("Scroll Right") );
    m_actions->setDefaultShortcut(scrollRight, Qt::Key_Right);
    connect( scrollRight, SIGNAL( triggered() ), this, SLOT( scrollRight() ) );
    // --------
    QAction *pause = m_actions->addAction( "kuick_slideshow_pause" );
    pause->setText( i18n("Pause Slideshow") );
    m_actions->setDefaultShortcut(pause, Qt::Key_P);
    connect( pause, SIGNAL( triggered() ), this, SLOT( pauseSlideShow() ) );

    QAction *fullscreenAction = m_actions->addAction( KStandardAction::FullScreen, "fullscreen", this, SLOT( toggleFullscreen() ));
    QList<QKeySequence> shortcuts = fullscreenAction->shortcuts();
    if(!shortcuts.contains(Qt::Key_Return)) shortcuts << Qt::Key_Return;
    m_actions->setDefaultShortcuts(fullscreenAction, shortcuts);
//    KAction *fullscreenAction = KStandardAction::fullScreen(this, SLOT( toggleFullscreen() ), m_actions);
//    m_actions->addAction( "", fullscreenAction );

    QAction *reloadAction = m_actions->addAction( "reload_image" );
    reloadAction->setText( i18n("Reload Image") );
    if(!(shortcuts = KStandardShortcut::reload()).contains(Qt::Key_Enter)) shortcuts << Qt::Key_Enter;
    m_actions->setDefaultShortcuts(reloadAction, shortcuts);
    connect( reloadAction, SIGNAL( triggered() ), this, SLOT( reload() ) );

    QAction *properties = m_actions->addAction("properties" );
    properties->setText( i18n("Properties") );
    m_actions->setDefaultShortcut(properties, Qt::ALT + Qt::Key_Return);
    connect( properties, SIGNAL( triggered() ), this, SLOT( slotProperties() ) );

    m_actions->readSettings();
}

void ImageWindow::showWindow()
{
	if ( myIsFullscreen )
		showFullScreen();
	else
		showNormal();
}

void ImageWindow::setFullscreen( bool enable )
{
    xpos = 0; ypos = 0;

//    if ( enable && !myIsFullscreen ) { // set Fullscreen
//        showFullScreen();
//    }
//    else if ( !enable && myIsFullscreen ) { // go into window mode
//        showNormal();
//    }

    myIsFullscreen = enable;
//    centerImage(); // ### really necessary (multihead!)
}


void ImageWindow::updateGeometry( int imWidth, int imHeight )
{
//     qDebug("::updateGeometry: %i, %i", imWidth, imHeight);
    resize(imWidth, imHeight);

    if ( imWidth != width() || imHeight != height() ) {
	if ( myIsFullscreen ) {
	    centerImage();
	}
	else { // window mode
	    resizeOptimal( imWidth, imHeight ); // also centers the image
	}
    }
    else { // image size == widget size
	xpos = 0; ypos = 0;
	move(0, 0);
    }

    updateCursor();

    QString caption = i18nc( "Filename (Imagewidth x Imageheight)",
                             "%3 (%1 x %2)",
                             m_kuim->originalWidth(), m_kuim->originalHeight(),
                             m_kuim->url().toDisplayString() );
    setWindowTitle( caption );
}


void ImageWindow::centerImage()
{
    int w, h;
    if ( myIsFullscreen )
    {
        QRect desktopRect = QApplication::desktop()->screenGeometry(this);
        w = desktopRect.width();
        h = desktopRect.height();
    }
    else
    {
        w = width();
        h = height();
    }

    xpos = w/2 - imageWidth()/2;
    ypos = h/2 - imageHeight()/2;

    // Modified by Evan for his Multi-Head (2 screens)
    // This should center on the first head
//     if ( myIsFullscreen && m_numHeads > 1 && ((m_numHeads % 2) == 0) )
//         xpos = ((width()/m_numHeads) / 2) - imageWidth()/2;
//     else
//         xpos = width()/2 - imageWidth()/2;

//     ypos = height()/2 - imageHeight()/2;
    move(xpos, ypos);
}


void ImageWindow::scrollImage( int x, int y, bool restrict )
{
    horizontalScrollBar()->setValue(horizontalScrollBar()->value()-x);
    verticalScrollBar()->setValue(verticalScrollBar()->value()-y);
}


// image loading performs:
// ---------------------
// loadImageInternal();
//     reset image mods
//     load image from disk / get from cache
//     loaded(); // apply modifications, scale
//     render pixmap
//
// updateWidget();
//     XUnmapWindow();
//     XSetWindowBackgroundPixmap()
//     resize window to fit image size, center image
//     XClearWindow(); // repaint
//     XMapWindow(), XSync();
//
bool ImageWindow::showNextImage( const QUrl& url )
{
    KuickFile *file = FileCache::self()->getFile( url );
    switch ( file->waitForDownload( this ) ) {
    	case KuickFile::ERROR:
    	{
            QString tmp = i18n("Unable to download the image from %1.", url.toDisplayString());
	        emit sigImageError( file, tmp );
	        return false;
    	}
	    case KuickFile::CANCELED:
	    	return false; // just abort, no error message
	    default:
	    	break; // go on...
    }

    return showNextImage( file );
}

bool ImageWindow::showNextImage( KuickFile *file )
{
    if ( !loadImage( file ) ) {
   	    QString tmp = i18n("Unable to load the image %1.\n"
                       "Perhaps the file format is unsupported or "
                       "your Imlib is not installed properly.", file->url().toDisplayString());
        emit sigImageError( file, tmp );
	return false;
    }

    else {
	// updateWidget( true ); // already called from loadImage()
	if ( !isVisible() )
		showWindow();

//	showImage();
	return true;
    }
}

void ImageWindow::reload()
{
    showNextImage( currentFile() );
}

void ImageWindow::pauseSlideShow()
{
    emit pauseSlideShowSignal();
}

void ImageWindow::addBrightness( int factor )
{
    if ( factor == 0 )
	return;

    int oldValue = mod.brightness - ImlibOffset;
    setBrightness( oldValue + (idata->brightnessFactor * (int) factor) );
}

void ImageWindow::addContrast( int factor )
{
    if ( factor == 0 )
	return;

    int oldValue = mod.contrast - ImlibOffset;
    setContrast( oldValue + (idata->contrastFactor * (int) factor) );
}

void ImageWindow::addGamma( int factor )
{
    if ( factor == 0 )
	return;

    int oldValue = mod.gamma - ImlibOffset;
    setGamma( oldValue + (idata->gammaFactor * (int) factor) );
}


////////////
////
// slots for keyboard/popupmenu actions


void ImageWindow::scrollUp()
{
    scrollImage( 0, 20 * kdata->scrollSteps );
}

void ImageWindow::scrollDown()
{
    scrollImage( 0, - 20 * kdata->scrollSteps );
}

void ImageWindow::scrollLeft()
{
    scrollImage( 20 * kdata->scrollSteps, 0 );
}

void ImageWindow::scrollRight()
{
    scrollImage( - 20 * kdata->scrollSteps, 0 );
}

///

void ImageWindow::zoomIn()
{
    zoomImage( kdata->zoomSteps );
}

void ImageWindow::zoomOut()
{
    Q_ASSERT( kdata->zoomSteps != 0 );
    zoomImage( 1.0 / kdata->zoomSteps );
}

///

void ImageWindow::moreBrightness()
{
    addBrightness( kdata->brightnessSteps );
}

void ImageWindow::moreContrast()
{
    addContrast( kdata->contrastSteps );
}

void ImageWindow::moreGamma()
{
    addGamma( kdata->gammaSteps );
}


void ImageWindow::lessBrightness()
{
    addBrightness( - kdata->brightnessSteps );
}

void ImageWindow::lessContrast()
{
    addContrast( - kdata->contrastSteps );
}

void ImageWindow::lessGamma()
{
    addGamma( - kdata->gammaSteps );
}

void ImageWindow::imageDelete()
{
    emit deleteImage(this);
}

void ImageWindow::imageTrash()
{
    emit trashImage(this);
}

///




/////////////
////
// event handlers

void ImageWindow::wheelEvent( QWheelEvent *e )
{
    e->accept();
    static const int WHEEL_DELTA = 120;
    int delta = e->angleDelta().y();

    if ( delta == 0 )
        return;

    int steps = delta / WHEEL_DELTA;
    emit requestImage( this, -steps );
}

void ImageWindow::keyPressEvent( QKeyEvent *e )
{
    uint key = e->key() | e->modifiers();

    if ( key == Qt::Key_Shift )
        updateCursor( ZoomCursor );

    if ( key == Qt::Key_Escape || KStandardShortcut::close().contains( key ) )
        close();
    else if ( KStandardShortcut::save().contains( key ) )
        saveImage();
    else if ( key == Qt::Key_Right || key == Qt::Key_Down )
        emit nextSlideRequested();
    else if ( key == Qt::Key_Left || key == Qt::Key_Up )
        emit prevSlideRequested(); // For future use...

    else {
        e->ignore();
        return;
    }

    e->accept();
}

void ImageWindow::keyReleaseEvent( QKeyEvent *e )
{
    if ( e->modifiers() & Qt::ShiftModifier ) { // Shift-key released
        updateCursor();
        if ( transWidget ) {
            delete transWidget;
            transWidget = 0L;
        }
    }

    e->accept();
}

void ImageWindow::mousePressEvent( QMouseEvent *e )
{
    xmove = e->x(); // for moving the image with the mouse
    ymove = e->y();

    xzoom = xmove;  // for zooming with the mouse
    yzoom = ymove;

    xposPress = xmove;
    yposPress = ymove;

    if ( e->button() == Qt::LeftButton ) {
        if ( e->modifiers() & Qt::ShiftModifier )
            updateCursor( ZoomCursor );
        else
            updateCursor( MoveCursor );
    }

    ImlibWidget::mousePressEvent( e );
}

void ImageWindow::contextMenuEvent( QContextMenuEvent *e )
{
    e->accept();

    if ( !viewerMenu )
        setPopupMenu();

    viewerMenu->popup( e->globalPos() );
}

void ImageWindow::updateCursor( KuickCursor cursor )
{
    switch ( cursor )
    {
        case ZoomCursor:
            setCursor( Qt::ArrowCursor ); // need a magnify-cursor
            break;
        case MoveCursor:
            setCursor( *s_handCursor );
            break;
        case DefaultCursor:
        default:
            if ( isCursorHidden() )
                return;

            if ( imageWidth() > width() || imageHeight() > height() )
                setCursor( *s_handCursor );
            else
                setCursor( Qt::ArrowCursor );
            break;
    }
}

void ImageWindow::mouseMoveEvent( QMouseEvent *e )
{
    if ( (e->buttons() != Qt::LeftButton) ) { // only handle Qt::LeftButton actions
	return;
    }

    // FIXME: the zoom-box doesn't work at all
    if ( false && (e->modifiers() & Qt::ShiftModifier) != 0 ) {

	if ( !transWidget ) {
	    transWidget = new QWidget( this );
	    transWidget->setGeometry( 0, 0, width(), height() );
	    transWidget->setAttribute( Qt::WA_NoSystemBackground, true );
	}

  	transWidget->hide();
	QPainter p( transWidget );
 	// really required?
 	p.eraseRect( transWidget->rect() );
	transWidget->show();
	//qApp->processOneEvent();
	qApp->processEvents( QEventLoop::ExcludeUserInputEvents );

	int width  = e->x() - xposPress;
	int height = e->y() - yposPress;

	if ( width < 0 ) {
	    width = abs( width );
	    xzoom = e->x();
	}

	if ( height < 0 ) {
	    height = abs( height );
	    yzoom = e->y();
	}

	QPen pen( Qt::white, 1, Qt::DashLine );
	p.setPen( pen );     // for drawing white dashed line
	p.drawRect( xzoom, yzoom, width, height );
	p.setPen( Qt::DotLine ); // defaults to black dotted line pen
	p.drawRect( xzoom, yzoom, width, height );
    }

    else { // move the image
	// scrolling with mouse
	uint xtmp = e->x();
	uint ytmp = e->y();
	scrollImage( xtmp - xmove, ytmp - ymove );
	xmove = xtmp;
	ymove = ytmp;
    }
}

void ImageWindow::mouseReleaseEvent( QMouseEvent *e )
{
    updateCursor();

    if ( transWidget ) {
       // destroy the transparent widget, used for showing the rectangle (zoom)
	delete transWidget;
	transWidget = 0L;
    }

    // only proceed if shift-Key is still pressed
    if ( !(e->button() == Qt::LeftButton && e->modifiers() & Qt::ShiftModifier) )
	return;

    int neww, newh, topX, topY, botX, botY;
    float factor, factorx, factory;

    // zoom into the selected area
    uint x = e->x();
    uint y = e->y();

    if ( xposPress == x || yposPress == y )
	return;

    if ( xposPress > x ) {
	topX = x;
	botX = xposPress;
    }
    else {
	topX = xposPress;
	botX = x;
    }

    if ( yposPress > y ) {
	topY = y;
	botY = yposPress;
    }
    else {
	topY = yposPress;
	botY = y;
    }

    neww = botX - topX;
    newh = botY - topY;

    factorx = ((float) width() / (float) neww);
    factory = ((float) height() / (float) newh);

    if ( factorx < factory ) // use the smaller factor
	factor = factorx;
    else factor = factory;

    uint w = 0; // shut up compiler!
    uint h = 0;
    w = (uint) ( factor * (float) imageWidth() );
    h = (uint) ( factor * (float) imageHeight() );

    if ( !canZoomTo( w, h ) )
	return;

    int xtmp = - (int) (factor * abs(xpos - topX) );
    int ytmp = - (int) (factor * abs(ypos - topY) );

    // if image has different ratio (width()/height()), center it
    int xcenter = (width()  - (int) (neww * factor)) / 2;
    int ycenter = (height() - (int) (newh * factor)) / 2;

    xtmp += xcenter;
    ytmp += ycenter;

    m_kuim->resize( w, h, idata->smoothScale ? KuickImage::SMOOTH : KuickImage::FAST );
    resize( w, h );
    updateWidget( false );

    xpos = xtmp; ypos = ytmp;
    move(xpos, ypos);
    scrollImage( 1, 1, true ); // unrestricted scrolling
}


void ImageWindow::focusInEvent( QFocusEvent *ev )
{
    ImlibWidget::focusInEvent( ev );
    emit sigFocusWindow( this );
}


void ImageWindow::resizeEvent( QResizeEvent *e )
{
    ImlibWidget::resizeEvent( e );

    centerImage();
    updateCursor();
}


void ImageWindow::dragEnterEvent( QDragEnterEvent *e )
{
    //  if ( e->provides( "image/*" ) ) // can't do this right now with Imlib
    if ( e->mimeData()->hasFormat( "text/uri-list" ) )
	e->accept();
    else
	e->ignore();
}


void ImageWindow::dropEvent( QDropEvent *e )
{
    // FIXME - only preliminary drop-support for now
    QList<QUrl> list = KUrlMimeData::urlsFromMimeData(e->mimeData());
    if ( !list.isEmpty()) {
        for(const QUrl& url : list) {
            if(url.isValid()) {
                loadImage(url);
                break;
            }
        }
	updateWidget();
	e->accept();
    }
    else
	e->ignore();
}


////////////////////
/////////
// misc stuff

void ImageWindow::setPopupMenu()
{
    viewerMenu = new QMenu( this );

    viewerMenu->addAction(m_actions->action("next_image"));
    viewerMenu->addAction(m_actions->action("previous_image"));
    viewerMenu->addSeparator();

    brightnessMenu = new QMenu( i18n("Brightness"), viewerMenu );
    brightnessMenu->addAction(m_actions->action("more_brightness"));
    brightnessMenu->addAction(m_actions->action("less_brightness"));

    contrastMenu = new QMenu( i18n("Contrast"), viewerMenu );
    contrastMenu->addAction(m_actions->action("more_contrast"));
    contrastMenu->addAction(m_actions->action("less_contrast"));

    gammaMenu = new QMenu( i18n("Gamma"), viewerMenu );
    gammaMenu->addAction(m_actions->action("more_gamma"));
    gammaMenu->addAction(m_actions->action("less_gamma"));

    viewerMenu->addAction(m_actions->action("zoom_in"));
    viewerMenu->addAction(m_actions->action("zoom_out"));
    viewerMenu->addAction(m_actions->action("original_size"));
    viewerMenu->addAction(m_actions->action("maximize"));

    viewerMenu->addSeparator();
    viewerMenu->addAction(m_actions->action("rotate90"));
    viewerMenu->addAction(m_actions->action("rotate180"));
    viewerMenu->addAction(m_actions->action("rotate270"));

    viewerMenu->addSeparator();
    viewerMenu->addAction(m_actions->action("flip_vertically"));
    viewerMenu->addAction(m_actions->action("flip_horicontally"));
    viewerMenu->addSeparator();
    viewerMenu->addMenu( brightnessMenu );
    viewerMenu->addMenu( contrastMenu );
    viewerMenu->addMenu( gammaMenu );
    viewerMenu->addSeparator();

    viewerMenu->addAction(m_actions->action("delete_image"));
    viewerMenu->addAction(m_actions->action("print_image"));
    viewerMenu->addAction(m_actions->action("save_image_as"));
    viewerMenu->addAction(m_actions->action("properties"));

    viewerMenu->addSeparator();
    viewerMenu->addAction(m_actions->action("close_image"));
}

void ImageWindow::printImage()
{
    if ( !m_kuim )
        return;

    if ( !Printing::printImage( *this, this ) )
    {
        KMessageBox::error( this, i18n("Unable to print the image."),
                            i18n("Printing Failed") );
    }
}

void ImageWindow::saveImage()
{
    if ( !m_kuim )
        return;

    KuickData tmp;
    QCheckBox *keepSize = new QCheckBox( i18n("Keep original image size"), 0L);
    keepSize->setChecked( true );

    QFileDialog dlg(this);
    dlg.setWindowTitle( i18n("Save As") );
    dlg.setOption(QFileDialog::DontUseNativeDialog);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setNameFilter(i18n("Image Files (%1)").arg(tmp.fileFilter));
    dlg.setDirectoryUrl(QUrl::fromUserInput(m_saveDirectory, QDir::currentPath(), QUrl::AssumeLocalFile));

    // insert the checkbox below the filter box
    if(QGridLayout* gl = qobject_cast<QGridLayout*>(dlg.layout())) {
        gl->addWidget(keepSize, gl->rowCount(), 0, 1, gl->columnCount());
    }

    QString selection = m_saveDirectory.isEmpty() ?
                            m_kuim->url().url() :
                            m_kuim->url().fileName();
    dlg.selectFile( selection );
    if ( dlg.exec() == QDialog::Accepted )
    {
        QList<QUrl> urls = dlg.selectedUrls();
        QUrl url = urls.value(0);
        if ( url.isValid() )
        {
            if ( !saveImage( url, keepSize->isChecked() ) )
            {
                QString tmp = i18n("Could not save the file.\n"
                                   "Perhaps the disk is full, or you do not "
                                   "have write permission to the file.");
                KMessageBox::error( this, tmp, i18n("File Saving Failed"));
            }
            else
            {
                if ( url == m_kuim->url() ) {
					Imlib_apply_modifiers_to_rgb( id, m_kuim->imlibImage() );
				}
            }
        }
    }

    QString lastDir = dlg.directoryUrl().toDisplayString();
    if ( lastDir != m_saveDirectory )
        m_saveDirectory = lastDir;
}

bool ImageWindow::saveImage( const QUrl& dest, bool keepOriginalSize )
{
    int w = keepOriginalSize ? m_kuim->originalWidth()  : m_kuim->width();
    int h = keepOriginalSize ? m_kuim->originalHeight() : m_kuim->height();
    if ( m_kuim->absRotation() == ROT_90 || m_kuim->absRotation() == ROT_270 )
        qSwap( w, h );

    ImlibImage *saveIm = Imlib_clone_scaled_image( id, m_kuim->imlibImage(),
                                                   w, h );
    bool success = false;

	QString saveFile;
	if ( dest.isLocalFile() )
		saveFile = dest.path();
	else
	{

		QString extension = QFileInfo( dest.fileName() ).completeSuffix();
        if(!extension.isEmpty()) extension.prepend('.');
        QScopedPointer<QTemporaryFile> tmpFilePtr(FileCache::self()->createTempFile(extension));
        if(tmpFilePtr.isNull()) return false;

        tmpFilePtr->setAutoRemove(false);
        if ( !tmpFilePtr->open() )
			return false;
        saveFile = tmpFilePtr->fileName();
        tmpFilePtr->close();
	}

    if ( saveIm )
    {
        Imlib_apply_modifiers_to_rgb( id, saveIm );
        success = Imlib_save_image( id, saveIm,
                                    QFile::encodeName( saveFile ).data(),
                                    NULL );
        if ( success && !dest.isLocalFile() )
        {
        	if ( isFullscreen() )
        		toggleFullscreen(); // otherwise upload window would block us invisibly

            QFile sourceFile(saveFile);
            if(!sourceFile.open(QIODevice::ReadOnly)) {
                // TODO: implement better error handling
                qWarning("failed to open file \"%s\": %s", qUtf8Printable(saveFile), qUtf8Printable(sourceFile.errorString()));
                return false;
            }

            KIO::StoredTransferJob* job = KIO::storedPut(&sourceFile, dest, -1);
            KJobWidgets::setWindow(job, this);
            success = job->exec();

            sourceFile.close();
        }

        Imlib_kill_image( id, saveIm );
    }

    return success;
}

void ImageWindow::toggleFullscreen()
{
    setFullscreen( !myIsFullscreen );
    showWindow();
}

void ImageWindow::loaded( KuickImage *kuim, bool wasCached )
{
	if (wasCached)
	{
		return; // keep it as it is
	}

    if ( !ImageMods::restoreFor( kuim, idata ) )
    {
    	// if no cached image modifications are available, apply the default modifications
        if ( !kdata->isModsEnabled ) {
    		// ### BUG: should be "restorePreviousSize"
        	kuim->restoreOriginalSize();
        }
        else
        {
        	autoRotate( kuim );
        	autoScale( kuim );
        }
    }
}

// upscale/downscale depending on configuration
void ImageWindow::autoScale( KuickImage *kuim )
{
    int newW = kuim->originalWidth();
    int newH = kuim->originalHeight();

    QSize s = maxImageSize();
    int mw = s.width();
    int mh = s.height();

    if ( kuim->absRotation() == ROT_90 || kuim->absRotation() == ROT_270 )
        qSwap( newW, newH );

    bool doIt = false;

    if ( kdata->upScale )
    {
	if ( (newW < mw) && (newH < mh) )
        {
            doIt = true;

	    float ratio1, ratio2;
	    int maxUpScale = kdata->maxUpScale;

	    ratio1 = (float) mw / (float) newW;
	    ratio2 = (float) mh / (float) newH;
	    ratio1 = (ratio1 < ratio2) ? ratio1 : ratio2;
	    if ( maxUpScale > 0 )
		ratio1 = (ratio1 < maxUpScale) ? ratio1 : maxUpScale;
	    newH = (int) ((float) newH * ratio1);
	    newW = (int) ((float) newW * ratio1);
	}
    }

    if ( kdata->downScale )
    {
	// eventually set width and height to the best/max possible screen size
	if ( (newW > mw) || (newH > mh) )
        {
            doIt = true;

	    if ( newW > mw )
            {
		float ratio = (float) newW / (float) newH;
		newW = mw;
		newH = (int) ((float) newW / ratio);
	    }

	    // the previously calculated "h" might be larger than screen
	    if ( newH > mh ) {
		float ratio = (float) newW / (float) newH;
		newH = mh;
		newW = (int) ((float) newH * ratio);
	    }
	}
    }

    if ( doIt )
        kuim->resize( newW, newH, idata->smoothScale ? KuickImage::SMOOTH : KuickImage::FAST );
}

// only called when kdata->isModsEnabled is true
bool ImageWindow::autoRotate( KuickImage *kuim )
{
    if ( kdata->autoRotation && ImlibWidget::autoRotate( kuim ) )
        return true;

    else // rotation by metadata not available or not configured
    {
        // only apply default mods to newly loaded images

        // ### actually we should have a dirty flag ("neverManuallyFlipped")
        if ( kuim->flipMode() == FlipNone )
        {
            int flipMode = 0;
            if ( kdata->flipVertically )
                flipMode |= FlipVertical;
            if ( kdata->flipHorizontally )
                flipMode |= FlipHorizontal;

            kuim->flipAbs( flipMode );
        }

        if ( kuim->absRotation() == ROT_0 )
            kuim->rotateAbs( kdata->rotation );
    }

    return true;
}

int ImageWindow::desktopWidth( bool totalScreen ) const
{
    if ( myIsFullscreen || totalScreen )
    {
        return QApplication::desktop()->screenGeometry(topLevelWidget()).width();
    } else
	return Kuick::workArea().width();
}


int ImageWindow::desktopHeight( bool totalScreen ) const
{
    if ( myIsFullscreen || totalScreen ) {
        return QApplication::desktop()->screenGeometry(topLevelWidget()).height();
    } else {
	return Kuick::workArea().height();
    }
}

QSize ImageWindow::maxImageSize() const
{
    if ( myIsFullscreen ) {
        return QApplication::desktop()->screenGeometry(topLevelWidget()).size();
    }
    else {
	return Kuick::workArea().size() - Kuick::frameSize( winId() );
    }
}

void ImageWindow::resizeOptimal( int w, int h )
{
    QSize s = maxImageSize();
    int mw = s.width();
    int mh = s.height();
    int neww = (w >= mw) ? mw : w;
    int newh = (h >= mh) ? mh : h;

    if ( neww == width() && newh == height() )
	centerImage();
    else
	resize( neww, newh ); // also centers the image
}

void ImageWindow::maximize()
{
    if ( !m_kuim )
	return;

    bool oldUpscale = kdata->upScale;
    bool oldDownscale = kdata->downScale;

    kdata->upScale = true;
    kdata->downScale = true;

    autoScale( m_kuim );
    updateWidget( true );

    if ( !myIsFullscreen )
	resizeOptimal( imageWidth(), imageHeight() );

    kdata->upScale = oldUpscale;
    kdata->downScale = oldDownscale;
}

bool ImageWindow::canZoomTo( int newWidth, int newHeight )
{
    if ( !ImlibWidget::canZoomTo( newWidth, newHeight ) )
        return false;

    QSize desktopSize = QApplication::desktop()->screenGeometry(topLevelWidget()).size();

    int desktopArea = desktopSize.width() * desktopSize.height();
    int imageArea = newWidth * newHeight;

    if ( imageArea > desktopArea * kdata->maxZoomFactor )
    {
        return KMessageBox::warningContinueCancel(
            this,
            i18n("You are about to view a very large image (%1 x %2 pixels), which can be very resource-consuming and even make your computer hang.\nDo you want to continue?",
            newWidth, newHeight ),
            QString(),
            KStandardGuiItem::cont(),
            KStandardGuiItem::cancel(),
            "ImageWindow_confirm_very_large_window"
            ) == KMessageBox::Continue;
    }

    return true;
}

void ImageWindow::rotated( KuickImage *kuim, int rotation )
{
    if ( !m_kuim )
        return;

    ImlibWidget::rotated( kuim, rotation );

    if ( rotation == ROT_90 || rotation == ROT_270 )
        autoScale( kuim ); // ### BUG: only autoScale when configured!
}

void ImageWindow::slotProperties()
{
    KPropertiesDialog dlg( currentFile()->url(), this );
    dlg.exec();
}

void ImageWindow::setBusyCursor()
{
    // avoid busy cursor in fullscreen mode
    if ( !isFullscreen() )
        ImlibWidget::setBusyCursor();
}

void ImageWindow::restoreCursor()
{
    // avoid busy cursor in fullscreen mode
    if ( !isFullscreen() )
        ImlibWidget::restoreCursor();
}

bool ImageWindow::isCursorHidden() const
{
    return cursor().shape() == Qt::BlankCursor;
}
