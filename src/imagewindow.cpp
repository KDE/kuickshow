/* This file is part of the KDE project
   Copyright (C) 1998-2004 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include <stdlib.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <ktogglefullscreenaction.h>
#include <qcheckbox.h>
#include <qcursor.h>
#include <qdrawutil.h>
#include <qnamespace.h>
#include <qpainter.h>
#include <qpen.h>
#include <q3popupmenu.h>
#include <QBitmap>
#ifdef KDE_USE_FINAL
#undef GrayScale
#undef Color
#endif
#include <qrect.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtimer.h>
//Added by qt3to4:
#include <QWheelEvent>
#include <QPixmap>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QDropEvent>
#include <QContextMenuEvent>
#include <QResizeEvent>
#include <QDragEnterEvent>
#include <QMouseEvent>

#include <kapplication.h>
#include <kconfig.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kdeversion.h>
#ifdef KDE_USE_FINAL
#undef Unsorted
#endif
#include <kfiledialog.h>
#include <kiconloader.h>
#include <kimageeffect.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprinter.h>
#include <kpropertiesdialog.h>
#include <kstdaccel.h>
#include <kstandarddirs.h>
#include <kglobalsettings.h>
#include <ktemporaryfile.h>
#include <kwin.h>
#include <netwm.h>
#include <kio/netaccess.h>

#include "imagewindow.h"
#include "kuick.h"
#include "kuickdata.h"
#include "printing.h"

#undef GrayScale

QCursor *ImageWindow::s_handCursor = 0L;

ImageWindow::ImageWindow( ImData *_idata, ImlibData *id, QWidget *parent,
			  const char *name )
    : ImlibWidget( _idata, id, parent, name )
{
    init();
}

ImageWindow::ImageWindow( ImData *_idata, QWidget *parent, const char *name )
    : ImlibWidget( _idata, parent, name )
{
    init();
}

ImageWindow::~ImageWindow()
{
}


void ImageWindow::init()
{
    KCursor::setAutoHideCursor( this, true, true );
    KCursor::setHideCursorDelay( 1500 );

    // give the image window a different WM_CLASS
    XClassHint hint;
    hint.res_name = const_cast<char*>( kapp->name() );
    hint.res_class = const_cast<char*>( "ImageWindow" );
    XSetClassHint( x11Display(), winId(), &hint );

    viewerMenu = 0L;
    gammaMenu = 0L;
    brightnessMenu = 0L;
    contrastMenu = 0L;


    m_actions = new KActionCollection( this );

    if ( !s_handCursor ) {
        QString file = KStandardDirs::locate( "appdata", "pics/handcursor.png" );
        if ( !file.isEmpty() )
            s_handCursor = new QCursor( QBitmap(file) );
        else
            s_handCursor = new QCursor( Qt::ArrowCursor );
    }

    setupActions();
    imageCache->setMaxImages( kdata->maxCachedImages );

    transWidget    = 0L;
    myIsFullscreen = false;

    xpos = 0, ypos = 0;
    m_numHeads = ScreenCount( x11Display() );

    setAcceptDrops( true );
    setBackgroundColor( kdata->backgroundColor );

    static QPixmap imageIcon = UserIcon( "imageviewer-medium" );
    static QPixmap miniImageIcon = UserIcon( "imageviewer-small" );
    KWin::setIcons( winId(), imageIcon, miniImageIcon );
}

void ImageWindow::updateActions()
{
    m_actions->readSettings();
}

void ImageWindow::setupActions()
{
    KAction* nextImage = new KAction( i18n("Show Next Image"), m_actions, "next_image" );
    nextImage->setShortcut( KStdAccel::next() );
    connect( nextImage, SIGNAL( triggered() ), this, SLOT( slotRequestNext() ) );

    KAction* showPreviousImage = new KAction( i18n("Show Previous Image"), m_actions, "previous_image" );
    showPreviousImage->setShortcut(KStdAccel::prior());
    connect( showPreviousImage, SIGNAL( triggered() ), this, SLOT( slotRequestPrevious() ) );

    KAction* deleteImage = new KAction( i18n("Delete Image"), m_actions, "delete_image" );
    deleteImage->setShortcut(Qt::Key_Delete);
    connect( showPreviousImage, SIGNAL( triggered() ), this, SLOT( imageDelete() ) );

    KAction* zoomIn = new KAction( i18n("Zoom In"), m_actions, "zoom_in" );
    zoomIn->setShortcut(Qt::Key_Plus),
    connect( zoomIn, SIGNAL( triggered() ), this, SLOT( zoomIn() ));

    KAction *zoomOut = new KAction( i18n("Zoom Out"), m_actions, "zoom_out" );
    zoomOut->setShortcut(Qt::Key_Minus);
    connect( zoomOut, SIGNAL( triggered() ), this, SLOT( zoomOut() ) );

    KAction *restoreSize = new KAction( i18n("Restore Original Size"), m_actions, "original_size" );
    restoreSize->setShortcut(Qt::Key_O);
    connect( restoreSize, SIGNAL( triggered() ), this, SLOT( showImageOriginalSize() ) );

    KAction *maximize = new KAction( i18n("Maximize"), m_actions, "maximize" );
    maximize->setShortcut(Qt::Key_M);
    connect( maximize, SIGNAL( triggered() ), this, SLOT( maximize() ) );

    KAction *rotate90 = new KAction( i18n("Rotate 90 Degrees"), m_actions, "rotate90" );
    rotate90->setShortcut(Qt::Key_9);
    connect( rotate90, SIGNAL( triggered() ), this, SLOT( rotate90() ) );

    KAction *rotate180 = new KAction( i18n("Rotate 180 Degrees"), m_actions, "rotate180" );
    rotate180->setShortcut(Qt::Key_8);
    connect( rotate180, SIGNAL( triggered() ), this, SLOT( rotate180() ) );

    KAction *rotate270 = new KAction( i18n("Rotate 270 Degrees"), m_actions, "rotate270" );
    rotate270->setShortcut(Qt::Key_7);
    connect( rotate270, SIGNAL( triggered() ), this, SLOT( rotate270() ) );

    KAction *flipHori = new KAction( i18n("Flip Horizontally"), m_actions, "flip_horicontally" );
    flipHori->setShortcut(Qt::Key_Asterisk);
    connect( flipHori, SIGNAL( triggered() ), this, SLOT( flipHoriz() ) );

    KAction *flipVeri = new KAction( i18n("Flip Vertically"), m_actions, "flip_vertically" );
    flipVeri->setShortcut(Qt::Key_Slash);
    connect( flipVeri, SIGNAL( triggered() ), this, SLOT( flipVert() ) );

    KAction *printImage = new KAction( i18n("Print Image..."), m_actions, "print_image" );
    printImage->setShortcut(KStdAccel::print());
    connect( printImage, SIGNAL( triggered() ), this, SLOT( printImage() ) );

    KStandardAction::saveAs( this, SLOT( saveImage() ),
                 m_actions, "save_image_as" );

    KStandardAction::close( this, SLOT( close() ),
                 m_actions, "close_image" );
    // --------
    KAction *moreBrighteness = new KAction( i18n("More Brightness"), m_actions, "more_brightness" );
    moreBrighteness->setShortcut(Qt::Key_B);
    connect( moreBrighteness, SIGNAL( triggered() ), this, SLOT( moreBrightness() ) );

    KAction *lessBrightness = new KAction( i18n("Less Brightness"), m_actions, "less_brightness" );
    lessBrightness->setShortcut(Qt::SHIFT + Qt::Key_B);
    connect( lessBrightness, SIGNAL( triggered() ), this, SLOT( lessBrightness() ) );

    KAction *moreContrast = new KAction( i18n("More Contrast"), m_actions, "more_contrast" );
    moreContrast->setShortcut(Qt::Key_C);
    connect( moreContrast, SIGNAL( triggered() ), this, SLOT( moreContrast() ) );

    KAction *lessContrast = new KAction( i18n("Less Contrast"), m_actions, "less_contrast" );
    lessContrast->setShortcut(Qt::SHIFT + Qt::Key_C);
    connect( lessContrast, SIGNAL( triggered() ), this, SLOT( lessContrast() ) );

    KAction *moreGamma = new KAction( i18n("More Gamma"), m_actions, "more_gamma" );
    moreGamma->setShortcut(Qt::Key_G);
    connect( moreGamma, SIGNAL( triggered() ), this, SLOT( moreGamma() ) );

    KAction *lessGamma = new KAction( i18n("Less Gamma"), m_actions, "less_gamma" );
    lessGamma->setShortcut(Qt::SHIFT + Qt::Key_G);
    connect( lessGamma, SIGNAL( triggered() ), this, SLOT( lessGamma() ) );

    // --------
    KAction *scrollUp = new KAction( i18n("Scroll Up"), m_actions, "scroll_up" );
    scrollUp->setShortcut(Qt::Key_Up);
    connect( scrollUp, SIGNAL( triggered() ), this, SLOT( scrollUp() ) );

    KAction *scrollDown = new KAction( i18n("Scroll Down"), m_actions, "scroll_down" );
    scrollDown->setShortcut(Qt::Key_Down);
    connect( scrollDown, SIGNAL( triggered() ), this, SLOT( scrollDown() ) );

    KAction *scrollLeft = new KAction( i18n("Scroll Left"), m_actions, "scroll_left" );
    scrollLeft->setShortcut(Qt::Key_Left);
    connect( scrollLeft, SIGNAL( triggered() ), this, SLOT( scrollLeft() ) );

    KAction *scrollRight = new KAction( i18n("Scroll Right"), m_actions, "scroll_right" );
    scrollRight->setShortcut(Qt::Key_Right);
    connect( scrollRight, SIGNAL( triggered() ), this, SLOT( scrollRight() ) );
    // --------
    KAction *pause = new KAction( i18n("Pause Slideshow"), m_actions, "kuick_slideshow_pause" );
    pause->setShortcut(Qt::Key_P);
    connect( pause, SIGNAL( triggered() ), this, SLOT( pauseSlideShow() ) );

    KAction *fullscreenAction = KStandardAction::fullScreen(this, SLOT( toggleFullscreen() ), m_actions, 0 );
    fullscreenAction->setShortcut(KShortcut(Qt::Key_Return), KAction::DefaultShortcut);

    KAction *reloadImage = new KAction( i18n("Reload Image"), m_actions, "reload_image" );
    reloadImage->setShortcut(Qt::Key_Enter);
    connect( reloadImage, SIGNAL( triggered() ), this, SLOT( reload() ) );

    KAction *properties = new KAction( i18n("Properties"), m_actions, "properties" );
    properties->setShortcut(Qt::ALT + Qt::Key_Return);
    connect( reloadImage, SIGNAL( triggered() ), this, SLOT( slotProperties() ) );

    m_actions->readSettings();
}


void ImageWindow::setFullscreen( bool enable )
{
    xpos = 0; ypos = 0;

    if ( enable && !myIsFullscreen ) { // set Fullscreen
        showFullScreen();
    }
    else if ( !enable && myIsFullscreen ) { // go into window mode
        showNormal();
    }

    myIsFullscreen = enable;
    centerImage(); // ### really necessary (multihead!)
}


void ImageWindow::updateGeometry( int imWidth, int imHeight )
{
//     qDebug("::updateGeometry: %i, %i", imWidth, imHeight);
    //  XMoveWindow( x11Display(), win, 0, 0 );
    XResizeWindow( x11Display(), win, imWidth, imHeight );

    if ( imWidth != width() || imHeight != height() ) {
	if ( myIsFullscreen ) {
	    centerImage();
	}
	else { // window mode
	    // XMoveWindow( x11Display(), win, 0, 0 );
	    resizeOptimal( imWidth, imHeight ); // also centers the image
	}
    }
    else { // image size == widget size
	xpos = 0; ypos = 0;
	XMoveWindow( x11Display(), win, 0, 0 );
    }

    updateCursor();

    QString caption = i18nc( "Filename (Imagewidth x Imageheight)",
                             "%3 (%1 x %2)",
                             m_kuim->originalWidth(), m_kuim->originalHeight(),
                             m_kuim->filename() );
    setWindowTitle( KDialog::makeStandardCaption( caption, this ) );
}


void ImageWindow::centerImage()
{
    int w, h;
    if ( myIsFullscreen )
    {
        QRect desktopRect = KGlobalSettings::desktopGeometry( this );
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

    XMoveWindow( x11Display(), win, xpos, ypos );

    // Modified by Evan for his Multi-Head (2 screens)
    // This should center on the first head
//     if ( myIsFullscreen && m_numHeads > 1 && ((m_numHeads % 2) == 0) )
//         xpos = ((width()/m_numHeads) / 2) - imageWidth()/2;
//     else
//         xpos = width()/2 - imageWidth()/2;

//     ypos = height()/2 - imageHeight()/2;
//     XMoveWindow( x11Display(), win, xpos, ypos );
}


void ImageWindow::scrollImage( int x, int y, bool restrict )
{
    xpos += x;
    ypos += y;

    int cwlocal = width();
    int chlocal = height();

    int iw = imageWidth();
    int ih = imageHeight();

    if ( myIsFullscreen || width() > desktopWidth() )
	cwlocal = desktopWidth();

    if ( myIsFullscreen || height() > desktopHeight() )
	chlocal = desktopHeight();

    if ( restrict ) { // don't allow scrolling in certain cases
	if ( x != 0 ) { // restrict x-movement
	    if ( iw <= cwlocal )
		xpos -= x; // restore previous position
	    else if ( (xpos <= 0) && (xpos + iw <= cwlocal) )
		xpos = cwlocal - iw;
	    else if ( (xpos + iw >= cwlocal) && xpos >= 0 )
		xpos = 0;
	}

	if ( y != 0 ) { // restrict y-movement
	    if ( ih <= chlocal )
		ypos -= y;
	    else if ( (ypos <= 0) && (ypos + ih <= chlocal) )
		ypos = chlocal - ih;
	    else if ( (ypos + ih >= chlocal) && ypos >= 0 )
		ypos = 0;
	}
    }

    XMoveWindow( x11Display(), win, xpos, ypos );
    XClearArea( x11Display(), win, xpos, ypos, iw, ih, false );
    showImage();
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
bool ImageWindow::showNextImage( const QString& filename )
{
    if ( !loadImage( filename ) ) {
	emit sigBadImage( filename );
	return false;
    }

    else {
	// updateWidget( true ); // already called from loadImage()
	showImage();
	return true;
    }
}

void ImageWindow::reload()
{
    showNextImage( filename() );
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
    emit deleteImage();
}

///




/////////////
////
// event handlers

void ImageWindow::wheelEvent( QWheelEvent *e )
{
    e->accept();
    static const int WHEEL_DELTA = 120;
    int delta = e->delta();

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

    if ( key == Qt::Key_Escape || KStdAccel::close().contains( key ) )
        close( true );
    else if ( KStdAccel::save().contains( key ) )
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
    if ( e->state() & Qt::ShiftModifier ) { // Shift-key released
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
        if ( e->state() & Qt::ShiftModifier )
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
            if ( imageWidth() > width() || imageHeight() > height() )
                setCursor( *s_handCursor );
            else
                setCursor( Qt::ArrowCursor );
            break;
    }
}

void ImageWindow::mouseMoveEvent( QMouseEvent *e )
{
    if ( !(e->state() & Qt::LeftButton) ) { // only handle Qt::LeftButton actions
	return;
    }

    if ( e->state() & Qt::ShiftModifier ) {

	if ( !transWidget ) {
	    transWidget = new QWidget( this );
	    transWidget->setGeometry( 0, 0, width(), height() );
	    transWidget->setBackgroundMode( Qt::NoBackground );
	}

  	transWidget->hide();
	QPainter p( transWidget );
 	// really required?
 	p.eraseRect( transWidget->rect() );
	transWidget->show();
	qApp->processOneEvent();

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

#ifdef __GNUC__
#warning "qt4 what will replace p.flush() ????? "
#endif	
	//p.flush();
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
    if ( !(e->button() == Qt::LeftButton && e->state() & Qt::ShiftModifier) )
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

    if ( w > kdata->maxWidth || h > kdata->maxHeight ) {
	qDebug("KuickShow: scaling larger than configured maximum -> aborting" );
	return;
    }

    int xtmp = - (int) (factor * abs(xpos - topX) );
    int ytmp = - (int) (factor * abs(ypos - topY) );

    // if image has different ratio (width()/height()), center it
    int xcenter = (width()  - (int) (neww * factor)) / 2;
    int ycenter = (height() - (int) (newh * factor)) / 2;

    xtmp += xcenter;
    ytmp += ycenter;

    m_kuim->resize( w, h );
    XResizeWindow( x11Display(), win, w, h );
    updateWidget( false );

    xpos = xtmp; ypos = ytmp;

    XMoveWindow( x11Display(), win, xpos, ypos );
    scrollImage( 1, 1, true ); // unrestricted scrolling
}


void ImageWindow::focusInEvent( QFocusEvent * )
{
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
    if ( e->provides( "text/uri-list" ) )
	e->accept();
    else
	e->ignore();
}


void ImageWindow::dropEvent( QDropEvent *e )
{
    // FIXME - only preliminary drop-support for now
	KUrl::List list = KUrl::List::fromMimeData( e->mimeData() );
    if ( !list.isEmpty()) {
        QString tmpFile;
        const KUrl &url = list.first();
        if (KIO::NetAccess::download( url, tmpFile, this ) )
        {
	    loadImage( tmpFile );
	    KIO::NetAccess::removeTempFile( tmpFile );
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
    viewerMenu = new Q3PopupMenu( this );

    viewerMenu->addAction(m_actions->action("next_image"));
    viewerMenu->addAction(m_actions->action("previous_image"));
    viewerMenu->insertSeparator();

    brightnessMenu = new Q3PopupMenu( viewerMenu );
    brightnessMenu->addAction(m_actions->action("more_brightness"));
    brightnessMenu->addAction(m_actions->action("less_brightness"));

    contrastMenu = new Q3PopupMenu( viewerMenu );
    contrastMenu->addAction(m_actions->action("more_contrast"));
    contrastMenu->addAction(m_actions->action("less_contrast"));

    gammaMenu = new Q3PopupMenu( viewerMenu );
    gammaMenu->addAction(m_actions->action("more_gamma"));
    gammaMenu->addAction(m_actions->action("less_gamma"));

    viewerMenu->addAction(m_actions->action("zoom_in"));
    viewerMenu->addAction(m_actions->action("zoom_out"));
    viewerMenu->addAction(m_actions->action("original_size"));
    viewerMenu->addAction(m_actions->action("maximize"));

    viewerMenu->insertSeparator();
    viewerMenu->addAction(m_actions->action("rotate90"));
    viewerMenu->addAction(m_actions->action("rotate180"));
    viewerMenu->addAction(m_actions->action("rotate270"));

    viewerMenu->insertSeparator();
    viewerMenu->addAction(m_actions->action("flip_vertically"));
    viewerMenu->addAction(m_actions->action("flip_horicontally"));
    viewerMenu->insertSeparator();
    viewerMenu->insertItem( i18n("Brightness"), brightnessMenu );
    viewerMenu->insertItem( i18n("Contrast"), contrastMenu );
    viewerMenu->insertItem( i18n("Gamma"), gammaMenu );
    viewerMenu->insertSeparator();

    viewerMenu->addAction(m_actions->action("delete_image"));
    viewerMenu->addAction(m_actions->action("print_image"));
    viewerMenu->addAction(m_actions->action("save_image_as"));
    viewerMenu->addAction(m_actions->action("properties"));

    viewerMenu->insertSeparator();
    viewerMenu->addAction(m_actions->action("close_image"));
}

void ImageWindow::printImage()
{
    if ( !m_kuim )
        return;

    if ( !Printing::printImage( *this, this ) )
    {
        KMessageBox::sorry( this, i18n("Unable to print the image."),
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
    KFileDialog dlg( m_saveDirectory, tmp.fileFilter, this,keepSize);

    QString selection = m_saveDirectory.isEmpty() ?
                            m_kuim->filename() :
                            KUrl::fromPathOrUrl( m_kuim->filename() ).fileName();
    dlg.setSelection( selection );
    dlg.setOperationMode( KFileDialog::Saving );
    dlg.setCaption( i18n("Save As") );
    if ( dlg.exec() == QDialog::Accepted )
    {
        QString file = dlg.selectedFile();
        if ( !file.isEmpty() )
        {
            if ( !saveImage( file, keepSize->isChecked() ) )
            {
                QString tmp = i18n("Could not save the file.\n"
                                   "Perhaps the disk is full, or you don't "
                                   "have write permission to the file.");
                KMessageBox::sorry( this, tmp, i18n("File Saving Failed"));
            }

            if ( file == m_kuim->filename() ) {
                Imlib_apply_modifiers_to_rgb( id, m_kuim->imlibImage() );
            }
        }
    }

    QString lastDir = dlg.baseUrl().path(KUrl::AddTrailingSlash);
    if ( lastDir != m_saveDirectory )
        m_saveDirectory = lastDir;
}

bool ImageWindow::saveImage( const QString& filename, bool keepOriginalSize ) const
{
    int w = keepOriginalSize ? m_kuim->originalWidth()  : m_kuim->width();
    int h = keepOriginalSize ? m_kuim->originalHeight() : m_kuim->height();
    if ( m_kuim->absRotation() == ROT_90 || m_kuim->absRotation() == ROT_270 )
        qSwap( w, h );

    ImlibImage *saveIm = Imlib_clone_scaled_image( id, m_kuim->imlibImage(),
                                                   w, h );
    bool success = false;

    if ( saveIm ) {
        Imlib_apply_modifiers_to_rgb( id, saveIm );
        success = Imlib_save_image( id, saveIm,
                                    QFile::encodeName( filename ).data(),
                                    NULL );
        Imlib_kill_image( id, saveIm );
    }

    return success;
}

void ImageWindow::toggleFullscreen()
{
    setFullscreen( !myIsFullscreen );
}

void ImageWindow::loaded( KuickImage *kuim )
{
    if ( !kdata->isModsEnabled ) {
	kuim->restoreOriginalSize();
    }
    else
    {
        autoRotate( kuim );
        autoScale( kuim );
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
        kuim->resize( newW, newH );
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
        return KGlobalSettings::desktopGeometry(topLevelWidget()).width();
    } else
	return Kuick::workArea().width();
}


int ImageWindow::desktopHeight( bool totalScreen ) const
{
    if ( myIsFullscreen || totalScreen ) {
        return KGlobalSettings::desktopGeometry(topLevelWidget()).height();
    } else {
	return Kuick::workArea().height();
    }
}

QSize ImageWindow::maxImageSize() const
{
    if ( myIsFullscreen ) {
        return KGlobalSettings::desktopGeometry(topLevelWidget()).size();
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

void ImageWindow::slotProperties()
{
    KUrl url;
    url.setPath( filename() ); // ###
    (void) new KPropertiesDialog( url, this, "props dialog", true );
}

#include "imagewindow.moc"
