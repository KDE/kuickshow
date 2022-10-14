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
#include "imlibparams.h"


QCursor *ImageWindow::s_handCursor = nullptr;

ImageWindow::ImageWindow(QWidget *parent)
    : ImlibWidget(parent)
{
    init();
}


void ImageWindow::init()
{
    setFocusPolicy( Qt::StrongFocus );

    KCursor::setAutoHideCursor( this, true, true );
    KCursor::setHideCursorDelay( 1500 );

    // This image window will automatically be given
    // a distinctive WM_CLASS by Qt.

    viewerMenu = nullptr;
    gammaMenu = nullptr;
    brightnessMenu = nullptr;
    contrastMenu = nullptr;


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
    imageCache->setMaxImages( ImlibParams::kuickConfig()->maxCachedImages );

    transWidget    = nullptr;
    myIsFullscreen = false;

    xpos = 0, ypos = 0;

    setAcceptDrops( true );
    setBackgroundColor( ImlibParams::kuickConfig()->backgroundColor );

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
    QAction *duplicateWindow = m_actions->addAction( "duplicate_window" );
    duplicateWindow->setText( i18n("Duplicate Window") );
    m_actions->setDefaultShortcut(duplicateWindow, Qt::Key_D );
    connect(duplicateWindow, &QAction::triggered, this, &ImageWindow:: slotDuplicateWindow);

    QAction *nextImage = m_actions->addAction( "next_image" );
    nextImage->setText( i18n("Show Next Image") );
    m_actions->setDefaultShortcuts(nextImage, KStandardShortcut::next());
    connect(nextImage, &QAction::triggered, this, &ImageWindow::slotRequestNext);

    QAction* showPreviousImage = m_actions->addAction( "previous_image" );
    showPreviousImage->setText( i18n("Show Previous Image") );
    m_actions->setDefaultShortcuts(showPreviousImage, KStandardShortcut::prior());
    connect( showPreviousImage, &QAction::triggered, this, &ImageWindow::slotRequestPrevious);

    QAction* deleteImage = m_actions->addAction( "delete_image" );
    deleteImage->setText( i18n("Delete Image") );
    m_actions->setDefaultShortcut(deleteImage, QKeySequence(Qt::ShiftModifier | Qt::Key_Delete));
    connect(deleteImage, &QAction::triggered, this, &ImageWindow::imageDelete);

    QAction *trashImage = m_actions->addAction( "trash_image" );
    trashImage->setText( i18n("Move Image to Trash") );
    m_actions->setDefaultShortcut(trashImage, Qt::Key_Delete);
    connect(trashImage, &QAction::triggered, this, &ImageWindow::imageTrash);

    QAction* zoomIn = KStandardAction::zoomIn(this, &ImageWindow::zoomIn, m_actions);
    m_actions->setDefaultShortcut(zoomIn, Qt::Key_Plus);
    m_actions->addAction( "zoom_in",  zoomIn );

    QAction *zoomOut = KStandardAction::zoomOut(this, &ImageWindow::zoomOut, m_actions);
    m_actions->setDefaultShortcut(zoomOut, Qt::Key_Minus);
    m_actions->addAction( "zoom_out",  zoomOut );

    QAction *restoreSize = m_actions->addAction( "original_size" );
    restoreSize->setText( i18n("Restore Original Size") );
    m_actions->setDefaultShortcut(restoreSize, Qt::Key_O);
    connect(restoreSize, &QAction::triggered, this, &ImlibWidget::showImageOriginalSize);

    QAction *maximize = m_actions->addAction( "maximize" );
    maximize->setText( i18n("Maximize") );
    m_actions->setDefaultShortcut(maximize, Qt::Key_M);
    connect(maximize, &QAction::triggered, this, &ImageWindow::maximize);

    QAction *rotate90 = m_actions->addAction( "rotate90" );
    rotate90->setText( i18n("Rotate 90 Degrees") );
    m_actions->setDefaultShortcut(rotate90, Qt::Key_9);
    connect(rotate90, &QAction::triggered, this, &ImageWindow::rotate90);

    QAction *rotate180 = m_actions->addAction( "rotate180" );
    rotate180->setText( i18n("Rotate 180 Degrees") );
    m_actions->setDefaultShortcut(rotate180, Qt::Key_8);
    connect(rotate180, &QAction::triggered, this, &ImageWindow::rotate180);

    QAction *rotate270 = m_actions->addAction( "rotate270" );
    rotate270->setText( i18n("Rotate 270 Degrees") );
    m_actions->setDefaultShortcut(rotate270, Qt::Key_7);
    connect(rotate270, &QAction::triggered, this, &ImageWindow::rotate270);

    QAction *flipHori = m_actions->addAction( "flip_horicontally" );
    flipHori->setText( i18n("Flip Horizontally") );
    m_actions->setDefaultShortcut(flipHori, Qt::Key_Asterisk);
    connect(flipHori, &QAction::triggered, this, &ImageWindow::flipHoriz);

    QAction *flipVeri = m_actions->addAction( "flip_vertically" );
    flipVeri->setText( i18n("Flip Vertically") );
    m_actions->setDefaultShortcut(flipVeri, Qt::Key_Slash);
    connect(flipVeri, &QAction::triggered, this, &ImageWindow::flipVert);

    QAction *printImage = m_actions->addAction( "print_image" );
    printImage->setText( i18n("Print Image...") );
    m_actions->setDefaultShortcuts(printImage, KStandardShortcut::print());
    connect(printImage, &QAction::triggered, this, &ImageWindow::printImage);

    QAction *a =  KStandardAction::saveAs(this, QOverload<>::of(&ImageWindow::saveImage), m_actions);
    m_actions->addAction( "save_image_as",  a );

    a = KStandardAction::close(this, &QWidget::close, m_actions);
    m_actions->addAction( "close_image", a );
    // --------
    QAction *moreBrighteness = m_actions->addAction( "more_brightness" );
    moreBrighteness->setText( i18n("More Brightness") );
    m_actions->setDefaultShortcut(moreBrighteness, Qt::Key_B);
    connect(moreBrighteness, &QAction::triggered, this, &ImageWindow::moreBrightness);

    QAction *lessBrightness = m_actions->addAction( "less_brightness" );
    lessBrightness->setText(  i18n("Less Brightness") );
    m_actions->setDefaultShortcut(lessBrightness, Qt::SHIFT + Qt::Key_B);
    connect(lessBrightness, &QAction::triggered, this, &ImageWindow::lessBrightness);

    QAction *moreContrast = m_actions->addAction( "more_contrast" );
    moreContrast->setText( i18n("More Contrast") );
    m_actions->setDefaultShortcut(moreContrast, Qt::Key_C);
    connect(moreContrast, &QAction::triggered, this, &ImageWindow::moreContrast);

    QAction *lessContrast = m_actions->addAction( "less_contrast" );
    lessContrast->setText( i18n("Less Contrast") );
    m_actions->setDefaultShortcut(lessContrast, Qt::SHIFT + Qt::Key_C);
    connect(lessContrast, &QAction::triggered, this, &ImageWindow::lessContrast);

    QAction *moreGamma = m_actions->addAction( "more_gamma" );
    moreGamma->setText( i18n("More Gamma") );
    m_actions->setDefaultShortcut(moreGamma, Qt::Key_G);
    connect(moreGamma, &QAction::triggered, this, &ImageWindow::moreGamma);

    QAction *lessGamma = m_actions->addAction( "less_gamma" );
    lessGamma->setText( i18n("Less Gamma") );
    m_actions->setDefaultShortcut(lessGamma, Qt::SHIFT + Qt::Key_G);
    connect(lessGamma, &QAction::triggered, this, &ImageWindow::lessGamma);

    // --------
    QAction *scrollUp = m_actions->addAction( "scroll_up" );
    scrollUp->setText( i18n("Scroll Up") );
    m_actions->setDefaultShortcut(scrollUp, Qt::Key_Up);
    connect(scrollUp, &QAction::triggered, this, &ImageWindow::scrollUp);

    QAction *scrollDown = m_actions->addAction( "scroll_down" );
    scrollDown->setText( i18n("Scroll Down") );
    m_actions->setDefaultShortcut(scrollDown, Qt::Key_Down);
    connect(scrollDown, &QAction::triggered, this, &ImageWindow::scrollDown);

    QAction *scrollLeft = m_actions->addAction( "scroll_left" );
    scrollLeft->setText( i18n("Scroll Left") );
    m_actions->setDefaultShortcut(scrollLeft, Qt::Key_Left);
    connect(scrollLeft, &QAction::triggered, this, &ImageWindow::scrollLeft);

    QAction *scrollRight = m_actions->addAction( "scroll_right" );
    scrollRight->setText( i18n("Scroll Right") );
    m_actions->setDefaultShortcut(scrollRight, Qt::Key_Right);
    connect(scrollRight, &QAction::triggered, this, &ImageWindow::scrollRight);
    // --------
    QAction *pause = m_actions->addAction( "kuick_slideshow_pause" );
    pause->setText( i18n("Pause Slideshow") );
    m_actions->setDefaultShortcut(pause, Qt::Key_P);
    connect(pause, &QAction::triggered, this, &ImageWindow::pauseSlideShow);

    QAction *fullscreenAction = m_actions->addAction( KStandardAction::FullScreen, "fullscreen", this, &ImageWindow::toggleFullscreen);
    QList<QKeySequence> shortcuts = fullscreenAction->shortcuts();
    if(!shortcuts.contains(Qt::Key_Return)) shortcuts << Qt::Key_Return;
    m_actions->setDefaultShortcuts(fullscreenAction, shortcuts);
// TODO: make full screen work
//    KAction *fullscreenAction = KStandardAction::fullScreen(this, &ImageWindow::toggleFullscreen, m_actions);
//    m_actions->addAction( "", fullscreenAction );

    QAction *reloadAction = m_actions->addAction( "reload_image" );
    reloadAction->setText( i18n("Reload Image") );
    if(!(shortcuts = KStandardShortcut::reload()).contains(Qt::Key_Enter)) shortcuts << Qt::Key_Enter;
    m_actions->setDefaultShortcuts(reloadAction, shortcuts);
    connect(reloadAction, &QAction::triggered, this, &ImageWindow:: reload);

    QAction *properties = m_actions->addAction("properties" );
    properties->setText( i18n("Properties") );
    m_actions->setDefaultShortcut(properties, Qt::ALT + Qt::Key_Return);
    connect(properties, &QAction::triggered, this, &ImageWindow::slotProperties);

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
                             m_kuim->url().fileName());
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
	if (!isVisible()) showWindow();
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

// Slots for keyboard/popupmenu actions

void ImageWindow::scrollUp()
{
    scrollImage( 0, 20 * ImlibParams::kuickConfig()->scrollSteps );
}

void ImageWindow::scrollDown()
{
    scrollImage( 0, - 20 * ImlibParams::kuickConfig()->scrollSteps );
}

void ImageWindow::scrollLeft()
{
    scrollImage( 20 * ImlibParams::kuickConfig()->scrollSteps, 0 );
}

void ImageWindow::scrollRight()
{
    scrollImage( - 20 * ImlibParams::kuickConfig()->scrollSteps, 0 );
}

///

void ImageWindow::zoomIn()
{
    zoomImage( ImlibParams::kuickConfig()->zoomSteps );
}

void ImageWindow::zoomOut()
{
    Q_ASSERT( ImlibParams::kuickConfig()->zoomSteps != 0 );
    zoomImage( 1.0 / ImlibParams::kuickConfig()->zoomSteps );
}

// The 'brightnessSteps', 'contrastSteps' and 'gammaSteps' are
// all set to 1 by KuickData::KuickData().  The corresponding
// 'brightnessFactor', 'contrastFactor' and 'gammaFactor' are
// all set to 10 by ImData::ImData().  There is no GUI for those
// settings, so it is reasonable to assume that the calculated
// step will be either +10 or -10.

void ImageWindow::moreBrightness()
{
    stepBrightness(ImlibParams::kuickConfig()->brightnessSteps*ImlibParams::imlibConfig()->brightnessFactor);
}

void ImageWindow::moreContrast()
{
    stepContrast(ImlibParams::kuickConfig()->contrastSteps*ImlibParams::imlibConfig()->contrastFactor);
}

void ImageWindow::moreGamma()
{
    stepGamma(ImlibParams::kuickConfig()->gammaSteps*ImlibParams::imlibConfig()->gammaFactor);
}

void ImageWindow::lessBrightness()
{
    stepBrightness(-ImlibParams::kuickConfig()->brightnessSteps*ImlibParams::imlibConfig()->brightnessFactor);
}

void ImageWindow::lessContrast()
{
    stepContrast(-ImlibParams::kuickConfig()->contrastSteps*ImlibParams::imlibConfig()->contrastFactor);
}

void ImageWindow::lessGamma()
{
    stepGamma(-ImlibParams::kuickConfig()->gammaSteps*ImlibParams::imlibConfig()->gammaFactor);
}

void ImageWindow::imageDelete()
{
    emit deleteImage(this);
}

void ImageWindow::imageTrash()
{
    emit trashImage(this);
}

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
            transWidget = nullptr;
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
	transWidget = nullptr;
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

    factorx = static_cast<float>(width())/neww;
    factory = static_cast<float>(height())/newh;

    if ( factorx < factory ) // use the smaller factor
	factor = factorx;
    else factor = factory;

    uint w = 0; // shut up compiler!
    uint h = 0;
    w = static_cast<uint>(factor*static_cast<float>(imageWidth()));
    h = static_cast<uint>(factor*static_cast<float>(imageHeight()));

    if ( !canZoomTo( w, h ) )
	return;

    int xtmp = -static_cast<int>(factor*qAbs(xpos-topX));
    int ytmp = -static_cast<int>(factor*qAbs(ypos-topY));

    // if image has different ratio (width()/height()), center it
    int xcenter = (width()  - static_cast<int>(neww * factor)) / 2;
    int ycenter = (height() - static_cast<int>(newh * factor)) / 2;

    xtmp += xcenter;
    ytmp += ycenter;

    m_kuim->resize( w, h, ImlibParams::imlibConfig()->smoothScale ? KuickImage::SMOOTH : KuickImage::FAST );
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

// misc stuff

void ImageWindow::setPopupMenu()
{
    viewerMenu = new QMenu( this );

    viewerMenu->addAction(m_actions->action("duplicate_window"));
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

#ifdef HAVE_QTONLY
    // Colour controls are not supported without Imlib at the moment.
    brightnessMenu->setEnabled(false);
    contrastMenu->setEnabled(false);
    gammaMenu->setEnabled(false);
#endif // HAVE_QTONLY

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

    QCheckBox *keepSize = new QCheckBox( i18n("Keep original image size"), nullptr);
    keepSize->setChecked( true );

    QFileDialog dlg(this);
    dlg.setWindowTitle( i18n("Save As") );
    dlg.setOption(QFileDialog::DontUseNativeDialog);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setNameFilter(i18n("Image Files (%1)").arg(ImlibParams::kuickConfig()->fileFilter));
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
                // TODO: report error if available, don't speculate
                KMessageBox::error(this,
                                   i18n("Could not save the file.\n"
                                        "Perhaps the disk is full, or you do not "
                                        "have write permission to the file."),
                                   i18n("File Saving Failed"));
            }
            else
            {
                // TODO: what does this do?
                // if ( url == m_kuim->url() ) {
                //   Imlib_apply_modifiers_to_rgb( id, m_kuim->imlibImage() );
                // }
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

    bool success = false;
#if 0
// TODO: Imlib 2 and Qt
///////////////////////////////////////////////////////////////////////////
    ImlibImage *saveIm = Imlib_clone_scaled_image( id, m_kuim->imlibImage(),
                                                   w, h );

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
        Imlib_apply_modifiers_to_rgb( ImlibParams::data(), saveIm );
        success = Imlib_save_image( ImlibParams::data(), saveIm,
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

        Imlib_kill_image( ImlibParams::data(), saveIm );
    }
#endif
///////////////////////////////////////////////////////////////////////////

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

    if ( !ImageMods::restoreFor(kuim))
    {
    	// if no cached image modifications are available, apply the default modifications
        if ( !ImlibParams::kuickConfig()->isModsEnabled ) {
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

    if ( ImlibParams::kuickConfig()->upScale )
    {
	if ( (newW < mw) && (newH < mh) )
        {
            doIt = true;

	    float ratio1, ratio2;
	    int maxUpScale = ImlibParams::kuickConfig()->maxUpScale;

	    ratio1 = static_cast<float>(mw)/newW;
	    ratio2 = static_cast<float>(mh)/newH;
	    ratio1 = (ratio1 < ratio2) ? ratio1 : ratio2;
	    if ( maxUpScale > 0 )
		ratio1 = (ratio1 < maxUpScale) ? ratio1 : maxUpScale;
	    newH = static_cast<int>(newH * ratio1);
            newW = static_cast<int>(newW * ratio1);
	}
    }

    if ( ImlibParams::kuickConfig()->downScale )
    {
	// eventually set width and height to the best/max possible screen size
	if ( (newW > mw) || (newH > mh) )
        {
            doIt = true;

	    if ( newW > mw )
            {
		float ratio = static_cast<float>(newW)/static_cast<float>(newH);
		newW = mw;
		newH = static_cast<int>(newW/ratio);
	    }

	    // the previously calculated "h" might be larger than screen
	    if ( newH > mh ) {
		float ratio = static_cast<float>(newW)/static_cast<float>(newH);
		newH = mh;
		newW = static_cast<int>(newH*ratio);
	    }
	}
    }

    if ( doIt )
        kuim->resize( newW, newH, ImlibParams::imlibConfig()->smoothScale ? KuickImage::SMOOTH : KuickImage::FAST );
}

// only called when ImlibParams::kuickConfig()->isModsEnabled is true
bool ImageWindow::autoRotate( KuickImage *kuim )
{
    if ( ImlibParams::kuickConfig()->autoRotation && ImlibWidget::autoRotate( kuim ) )
        return true;

    else // rotation by metadata not available or not configured
    {
        // only apply default mods to newly loaded images

        // ### actually we should have a dirty flag ("neverManuallyFlipped")
        if ( kuim->flipMode() == FlipNone )
        {
            int flipMode = 0;
            if ( ImlibParams::kuickConfig()->flipVertically )
                flipMode |= FlipVertical;
            if ( ImlibParams::kuickConfig()->flipHorizontally )
                flipMode |= FlipHorizontal;

            kuim->flipAbs( flipMode );
        }

        if ( kuim->absRotation() == ROT_0 )
            kuim->rotateAbs( ImlibParams::kuickConfig()->rotation );
    }

    return true;
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

    bool oldUpscale = ImlibParams::kuickConfig()->upScale;
    bool oldDownscale = ImlibParams::kuickConfig()->downScale;

    ImlibParams::kuickConfig()->upScale = true;
    ImlibParams::kuickConfig()->downScale = true;

    autoScale( m_kuim );
    updateWidget( true );

    if ( !myIsFullscreen )
	resizeOptimal( imageWidth(), imageHeight() );

    ImlibParams::kuickConfig()->upScale = oldUpscale;
    ImlibParams::kuickConfig()->downScale = oldDownscale;
}

bool ImageWindow::canZoomTo( int newWidth, int newHeight )
{
    if ( !ImlibWidget::canZoomTo( newWidth, newHeight ) )
        return false;

    QSize desktopSize = QApplication::desktop()->screenGeometry(topLevelWidget()).size();

    int desktopArea = desktopSize.width() * desktopSize.height();
    int imageArea = newWidth * newHeight;

    if ( imageArea > desktopArea * ImlibParams::kuickConfig()->maxZoomFactor )
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

void ImageWindow::slotDuplicateWindow()
{
    emit duplicateWindow(currentFile()->url());
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
