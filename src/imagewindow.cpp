/***************************************************************************
** $Id$
**
** ImageWindow: extends the simple  ImlibWidget to display image in a Qt window
**		can zoom and scroll images inside the QWidget
**
** Created : 99
**
** Copyright (C) 1999-2001 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/

#include <stdlib.h>

#include <qcursor.h>
#include <qdrawutil.h>
#include <qkeycode.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpopupmenu.h>

#ifdef KDE_USE_FINAL
#undef GrayScale
#undef Color
#endif
#include <qrect.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qdragobject.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kcursor.h>
#include <kdebug.h>
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
#include <ktempfile.h>
#include <kwin.h>
#include <netwm.h>

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
//     KCursor::setAutoHideCursor( this, true, true );
//     KCursor::setHideCursorDelay( 1500 );
    viewerMenu = 0L;
    gammaMenu = 0L;
    brightnessMenu = 0L;
    contrastMenu = 0L;


    m_actions = new KActionCollection( this );

    if ( !s_handCursor ) {
        QString file = locate( "appdata", "pics/handcursor.png" );
        if ( !file.isEmpty() )
            s_handCursor = new QCursor( file );
        else
            s_handCursor = new QCursor( arrowCursor );
    }

    setupActions();

    transWidget    = 0L;
    myIsFullscreen = false;
    initialFullscreen = kdata->fullScreen;
    ignore_resize_hack = false;

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
    m_actions->readShortcutSettings();
}

void ImageWindow::setupActions()
{
    new KAction( i18n("Show Next Image"), KStdAccel::next(),
                 this, SLOT( slotRequestNext() ),
                 m_actions, "next_image" );
    new KAction( i18n("Show Previous Image"), KStdAccel::prior(),
                 this, SLOT( slotRequestPrevious() ),
                 m_actions, "previous_image" );

    new KAction( i18n("Zoom In"), Key_Plus,
                 this, SLOT( zoomIn() ),
                 m_actions, "zoom_in" );
    new KAction( i18n("Zoom Out"), Key_Minus,
                 this, SLOT( zoomOut() ),
                 m_actions, "zoom_out" );
    new KAction( i18n("Restore Original Size"), Key_O,
                 this, SLOT( showImageOriginalSize() ),
                 m_actions, "original_size" );
    new KAction( i18n("Maximize"), Key_M,
                 this, SLOT( maximize() ),
                 m_actions, "maximize" );

    new KAction( i18n("Rotate 90 degrees"), Key_9,
                 this, SLOT( rotate90() ),
                 m_actions, "rotate90" );
    new KAction( i18n("Rotate 180 degrees"), Key_8,
                 this, SLOT( rotate180() ),
                 m_actions, "rotate180" );
    new KAction( i18n("Rotate 270 degrees"), Key_7,
                 this, SLOT( rotate270() ),
                 m_actions, "rotate270" );

    new KAction( i18n("Flip Horizontally"), Key_Asterisk,
                 this, SLOT( flipHoriz() ),
                 m_actions, "flip_horicontally" );
    new KAction( i18n("Flip Vertically"), Key_Slash,
                 this, SLOT( flipVert() ),
                 m_actions, "flip_vertically" );

    new KAction( i18n("Print Image..."), KStdAccel::print(),
                 this, SLOT( printImage() ),
                 m_actions, "print_image" );
    new KAction( i18n("Save As..."), KStdAccel::save(),
                 this, SLOT( saveImage() ),
                 m_actions, "save_image_as" );

    new KAction( i18n("Close"), Key_Q,
                 this, SLOT( close() ),
                 m_actions, "close_image" );
    // --------
    new KAction( i18n("More Brightness"), Key_B,
                 this, SLOT( moreBrightness() ),
                 m_actions, "more_brightness" );
    new KAction( i18n("Less Brightness"), SHIFT + Key_B,
                 this, SLOT( lessBrightness() ),
                 m_actions, "less_brightness" );
    new KAction( i18n("More Contrast"), Key_C,
                 this, SLOT( moreContrast() ),
                 m_actions, "more_contrast" );
    new KAction( i18n("Less Contrast"), SHIFT + Key_C,
                 this, SLOT( lessContrast() ),
                 m_actions, "less_contrast" );
    new KAction( i18n("More Gamma"), Key_G,
                 this, SLOT( moreGamma() ),
                 m_actions, "more_gamma" );
    new KAction( i18n("Less Gamma"), SHIFT + Key_G,
                 this, SLOT( lessGamma() ),
                 m_actions, "less_gamma" );

    // --------
    new KAction( i18n("Scroll Up"), Key_Up,
                 this, SLOT( scrollUp() ),
                 m_actions, "scroll_up" );
    new KAction( i18n("Scroll Down"), Key_Down,
                 this, SLOT( scrollDown() ),
                 m_actions, "scroll_down" );
    new KAction( i18n("Scroll Left"), Key_Left,
                 this, SLOT( scrollLeft() ),
                 m_actions, "scroll_left" );
    new KAction( i18n("Scroll Right"), Key_Right,
                 this, SLOT( scrollRight() ),
                 m_actions, "scroll_right" );

    new KAction( i18n("Toggle Fullscreen mode"), Key_Return,
                 this, SLOT( toggleFullscreen() ),
                 m_actions, "toggle_fullscreen" );
    new KAction( i18n("Reload Image"), Key_Enter,
                 this, SLOT( reload() ),
                 m_actions, "reload_image" );

    new KAction( i18n("Properties..."), ALT + Key_Return,
                 this, SLOT( slotProperties() ),
                 m_actions, "properties" );

    m_actions->readShortcutSettings();
}


void ImageWindow::setFullscreen( bool enable )
{
    xpos = 0; ypos = 0;

    if ( enable && !myIsFullscreen ) { // set Fullscreen
        KWin::Info info = KWin::info( winId() );
        oldGeometry = info.frameGeometry;

	// qDebug("** oldGeometry: %i, %i, %i, %i",
	// oldGeometry.x(), oldGeometry.y(),
	// oldGeometry.width(), oldGeometry.height());

        int scnum = QApplication::desktop()->screenNumber(this);
        setFixedSize( QApplication::desktop()->screenGeometry(scnum).size() );

        KWin::setType( winId(), NET::Override );
        KWin::setState( winId(), NET::StaysOnTop );

        setGeometry( QApplication::desktop()->screenGeometry(scnum) );
        // qApp->processEvents(); // not necessary anymore

    }

    else if ( !enable && myIsFullscreen ) { // go into window mode
        bool wasInitialFullscreen = initialFullscreen;
        initialFullscreen = false;
	
        ignore_resize_hack = true; //ignore the resizeEvent triggered by move()
        move( oldGeometry.topLeft() );
        setMinimumSize(0,0);
        myIsFullscreen = false; // we want resizeOptimal to use window-mode
        resizeOptimal( imageWidth(), imageHeight() ); // resizeEvent centers

        KWin::setType( winId(), NET::Normal );
        KWin::clearState( winId(), NET::StaysOnTop );

        // hack around kwin not giving us a decoration, when going into window
        // mode and initially started in fullscreen mode
        if ( wasInitialFullscreen ) {
            hide();
            show();
        }
    }

    myIsFullscreen = enable;
    centerImage(); // ### really necessary (multihead!)
}


void ImageWindow::updateGeometry( int imWidth, int imHeight )
{
    //  XMoveWindow( x11Display(), win, 0, 0 );
    XResizeWindow( x11Display(), win, imWidth, imHeight );

    if ( imWidth != (int) m_width || imHeight != (int) m_height ) {
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

    QString caption = i18n( "Filename (Imagewidth x Imageheight)",
                            "%1 (%2 x %3)" );
    caption = caption.arg( kuim->filename() ).
              arg( kuim->originalWidth() ).
              arg( kuim->originalHeight() );
    setCaption( kapp->makeStdCaption( caption ) );
}


void ImageWindow::centerImage()
{
    // Modified by Evan for his Multi-Head (2 screens)
    // This should center on the first head
    if ( myIsFullscreen && m_numHeads > 1 && ((m_numHeads % 2) == 0) )
        xpos = ((m_width/m_numHeads) / 2) - imageWidth()/2;
    else
        xpos = m_width/2 - imageWidth()/2;

    ypos = m_height/2 - imageHeight()/2;
    XMoveWindow( x11Display(), win, xpos, ypos );
}


void ImageWindow::scrollImage( int x, int y, bool restrict )
{
    xpos += x;
    ypos += y;

    int cwlocal = m_width;
    int chlocal = m_height;

    int iw = imageWidth();
    int ih = imageHeight();

    if ( myIsFullscreen || m_width > desktopWidth() )
	cwlocal = desktopWidth();

    if ( myIsFullscreen || m_height > desktopHeight() )
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
    scrollImage( 0, kdata->scrollSteps );
}

void ImageWindow::scrollDown()
{
    scrollImage( 0, - kdata->scrollSteps );
}

void ImageWindow::scrollLeft()
{
    scrollImage( kdata->scrollSteps, 0 );
}

void ImageWindow::scrollRight()
{
    scrollImage( - kdata->scrollSteps, 0 );
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
    uint key = e->key();

    if ( key == Key_Escape || KStdAccel::close().contains( KKey( e ) ) )
	close( true );
    else if ( KStdAccel::save().contains( KKey( e ) ) )
	saveImage();

    else {
 	e->ignore();
 	return;
    }

    e->accept();
}

void ImageWindow::keyReleaseEvent( QKeyEvent *e )
{
    if ( e->state() & ShiftButton ) { // Shift-key released
	setCursor( arrowCursor );
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

    if ( e->button() == LeftButton ) {
        if ( e->state() & ShiftButton )
            setCursor( arrowCursor ); // need a magnify-cursor
        else
            setCursor( *s_handCursor );
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

void ImageWindow::mouseDoubleClickEvent( QMouseEvent *e )
{
    if ( e->button() == LeftButton )
	close( true );
}


void ImageWindow::mouseMoveEvent( QMouseEvent *e )
{
    if ( !(e->state() & LeftButton) ) { // only handle LeftButton actions
	return;
    }

    if ( e->state() & ShiftButton ) {
	
	if ( !transWidget ) {
	    transWidget = new QWidget( this );
	    transWidget->setGeometry( 0, 0, m_width, m_height );
	    transWidget->setBackgroundMode( NoBackground );
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

	QPen pen( Qt::white, 1, DashLine );
	p.setPen( pen );     // for drawing white dashed line
	p.drawRect( xzoom, yzoom, width, height );
	p.setPen( DotLine ); // defaults to black dotted line pen
	p.drawRect( xzoom, yzoom, width, height );
	p.flush();
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
    setCursor( arrowCursor );

    if ( transWidget ) {
       // destroy the transparent widget, used for showing the rectangle (zoom)
	delete transWidget;
	transWidget = 0L;
    }

    // only proceed if shift-Key is still pressed
    if ( !(e->button() == LeftButton && e->state() & ShiftButton) )
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

    factorx = ((float) m_width / (float) neww);
    factory = ((float) m_height / (float) newh);

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

    // if image has different ratio (m_width/m_height), center it
    int xcenter = (m_width  - (int) (neww * factor)) / 2;
    int ycenter = (m_height - (int) (newh * factor)) / 2;

    xtmp += xcenter;
    ytmp += ycenter;

    kuim->resize( w, h );
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

    if ( ignore_resize_hack ) {
	ignore_resize_hack = false;
	
	int w = width();
	int h = height();
        int scnum = QApplication::desktop()->screenNumber(this);
	if ( w == QApplication::desktop()->screenGeometry(scnum).width() &&
	     h == QApplication::desktop()->screenGeometry(scnum).height() &&
	     imageWidth() < w && imageHeight() < h ) {
	
	    return;
	}
    }

    // to save a lot of calls in scrollImage() for example
    m_width  = width();
    m_height = height();

    centerImage();
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
    QStringList list;
    if ( QUriDrag::decodeLocalFiles( e, list ) ) {
	loadImage( list.first() );
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
    viewerMenu = new QPopupMenu( this );

    m_actions->action("next_image")->plug( viewerMenu );
    m_actions->action("previous_image")->plug( viewerMenu );
    viewerMenu->insertSeparator();

    brightnessMenu = new QPopupMenu( viewerMenu );
    m_actions->action("more_brightness")->plug(brightnessMenu);
    m_actions->action("less_brightness")->plug(brightnessMenu);

    contrastMenu = new QPopupMenu( viewerMenu );
    m_actions->action("more_contrast")->plug(contrastMenu);
    m_actions->action("less_contrast")->plug(contrastMenu);

    gammaMenu = new QPopupMenu( viewerMenu );
    m_actions->action("more_gamma")->plug(gammaMenu);
    m_actions->action("less_gamma")->plug(gammaMenu);

    m_actions->action("zoom_in")->plug( viewerMenu );
    m_actions->action("zoom_out")->plug( viewerMenu );
    m_actions->action("original_size")->plug( viewerMenu );
    m_actions->action("maximize")->plug( viewerMenu );

    viewerMenu->insertSeparator();
    m_actions->action("rotate90")->plug( viewerMenu );
    m_actions->action("rotate180")->plug( viewerMenu );
    m_actions->action("rotate270")->plug( viewerMenu );

    viewerMenu->insertSeparator();
    m_actions->action("flip_vertically")->plug( viewerMenu );
    m_actions->action("flip_horicontally")->plug( viewerMenu );
    viewerMenu->insertSeparator();
    viewerMenu->insertItem( i18n("Brightness"), brightnessMenu );
    viewerMenu->insertItem( i18n("Contrast"), contrastMenu );
    viewerMenu->insertItem( i18n("Gamma"), gammaMenu );
    viewerMenu->insertSeparator();

    m_actions->action("print_image")->plug( viewerMenu );
    m_actions->action("save_image_as")->plug( viewerMenu );
    m_actions->action("properties")->plug( viewerMenu );

    viewerMenu->insertSeparator();
    m_actions->action("close_image")->plug( viewerMenu );
}

void ImageWindow::printImage()
{
    if ( !kuim )
        return;

    if ( !Printing::printImage( *this ) )
    {
        KMessageBox::sorry( this, i18n("Unable to print the image."),
                            i18n("Printing Failed") );
    }
}

void ImageWindow::saveImage()
{
    KuickData tmp;
    QString file = KFileDialog::getSaveFileName( kuim->filename(), 
                                                 tmp.fileFilter, this );
    if ( !file.isEmpty() )
    {
        if ( !saveImage( file ) )
        {
            QString tmp = i18n("Couldn't save the file.\n"
                               "Perhaps the disk is full, or you don't "
                               "have write permission to the file.");
            KMessageBox::sorry( this, tmp, i18n("File Saving Failed"));
        }
    }
}

bool ImageWindow::saveImage( const QString& filename ) const
{
    ImlibImage *saveIm = Imlib_clone_scaled_image( id, kuim->imlibImage(),
                                                   kuim->width(),
                                                   kuim->height() );
    if ( saveIm ) {
        Imlib_apply_modifiers_to_rgb( id, saveIm );
        bool success = Imlib_save_image( id, saveIm,
                                         QFile::encodeName( filename ).data(),
                                         NULL );
        Imlib_kill_image( id, saveIm );
        return success;
    }

    return false;
}

void ImageWindow::toggleFullscreen()
{
    setFullscreen( !myIsFullscreen );
}

// upscale/downscale depending on configuration
void ImageWindow::loaded( KuickImage *kuim )
{
    if ( !(kdata->isModsEnabled || kdata->upScale || kdata->downScale) ) {
	kuim->restoreOriginalSize();
	return;
    }

    int newW = kuim->originalWidth();
    int newH = kuim->originalHeight();

    QSize s = maxImageSize();
    int mw = s.width();
    int mh = s.height();

    if ( kdata->upScale ) {
	if ( (newW < mw) && (newH < mh) ) {
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

    if ( kdata->downScale ) {
	// eventually set width and height to the best/max possible screen size
	if ( (newW > mw) || (newH > mh) ) {
	    if ( newW > mw ) {
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

    kuim->resize( newW, newH );
}

int ImageWindow::desktopWidth( bool totalScreen ) const
{
    if ( myIsFullscreen || totalScreen ) {
        int scnum = QApplication::desktop()->screenNumber(topLevelWidget());
	return QApplication::desktop()->screenGeometry(scnum).width();
    } else {
	return Kuick::workArea().width();
    }
}


int ImageWindow::desktopHeight( bool totalScreen ) const
{
    if ( myIsFullscreen || totalScreen ) {
        int scnum = QApplication::desktop()->screenNumber(topLevelWidget());
	return QApplication::desktop()->screenGeometry(scnum).height();
    } else {
	return Kuick::workArea().height();
    }
}

QSize ImageWindow::maxImageSize() const
{
    if ( myIsFullscreen || initialFullscreen ) {
        int scnum = QApplication::desktop()->screenNumber(topLevelWidget());
	return QApplication::desktop()->screenGeometry(scnum).size();
    } else {
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

    if ( neww == m_width && newh == m_height )
	centerImage();
    else
	resize( neww, newh ); // also centers the image
}

void ImageWindow::maximize()
{
    if ( !kuim )
	return;

    bool oldUpscale = kdata->upScale;
    bool oldDownscale = kdata->downScale;

    kdata->upScale = true;
    kdata->downScale = true;

    loaded( kuim );
    updateWidget( true );

    if ( !myIsFullscreen )
	resizeOptimal( imageWidth(), imageHeight() );

    kdata->upScale = oldUpscale;
    kdata->downScale = oldDownscale;
}

void ImageWindow::slotProperties()
{
    KURL url;
    url.setPath( filename() ); // ###
    (void) new KPropertiesDialog( url, this, "props dialog", true );
}


#include "imagewindow.moc"
