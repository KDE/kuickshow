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
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpopupmenu.h>
#include <qprintdialog.h>

#ifdef KDE_USE_FINAL
#undef GrayScale
#undef Color
#endif
#include <qprinter.h>
#include <qrect.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qdragobject.h>

#include <kaccel.h>
#include <kconfig.h>
#include <kcursor.h>
#ifdef KDE_USE_FINAL
#undef Unsorted
#endif
#include <kfiledialog.h>
#include <klocale.h>
#include <kstdaccel.h>
#include <kstddirs.h>
#include <ktempfile.h>
#include <kwin.h>
#include <netwm.h>

#include "imagewindow.h"
#include "kuick.h"
#include "kuickdata.h"

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
//   KCursor::setAutoHideCursor( this, true, true );

    m_actions = new KActionCollection( this );

    if ( !s_handCursor ) {
        QString file = locate( "appdata", "pics/handcursor.png" );
        if ( !file.isEmpty() )
            s_handCursor = new QCursor( file );
        else
            s_handCursor = new QCursor( arrowCursor );
    }

    KAction *action = new KAction( i18n("Show next Image"), KStdAccel::next(),
                                   this, "next_image" );
    connect( action, SIGNAL( activated() ), SLOT( slotRequestNext() ));
    m_actions->insert( action );

    action = new KAction( i18n("Show previous Image"), KStdAccel::prior(),
                          this, "previous_image" );
    connect( action, SIGNAL( activated() ), SLOT( slotRequestPrevious() ));
    m_actions->insert( action );


    m_accel        = 0L;
    transWidget    = 0L;
    myIsFullscreen = false;
    initialFullscreen = kdata->fullScreen;
    ignore_resize_hack = false;

    xpos = 0, ypos = 0;
    m_numHeads = ScreenCount( x11Display() );

    setAcceptDrops( true );
    updateAccel();
    setBackgroundColor( kdata->backgroundColor );

    static QPixmap imageIcon = UserIcon( "imageviewer-medium" );
    static QPixmap miniImageIcon = UserIcon( "imageviewer-small" );
    KWin::setIcons( winId(), imageIcon, miniImageIcon );
}

void ImageWindow::updateAccel()
{
  delete m_accel;

  // configurable shortcuts
  // yeah, I know you've waited for them for a long time :o)
  m_accel = new KAccel( this );

  m_actions->action( "next_image" )->plugAccel( m_accel );
  m_actions->action( "previous_image" )->plugAccel( m_accel );

  m_accel->insertItem( i18n("Scroll Up"),	 "Scroll Up",         "Up" );
  m_accel->insertItem( i18n("Scroll Down"),      "Scroll Down",       "Down" );
  m_accel->insertItem( i18n("Scroll Left"),      "Scroll Left",       "Left" );
  m_accel->insertItem( i18n("Scroll Right"),     "Scroll Right",      "Right");
  m_accel->insertItem( i18n("Zoom In"),          "Zoom In",           "Plus" );
  m_accel->insertItem( i18n("Zoom Out"),         "Zoom Out",          "Minus");
  m_accel->insertItem( i18n("Flip Horizontally"),"Flip Horizontally","Asterisk");
  m_accel->insertItem( i18n("Flip Vertically"),  "Flip Vertically",   "Slash");
  m_accel->insertItem( i18n("Rotate 90 degrees"), "Rotate 90",         "9" );
  m_accel->insertItem( i18n("Rotate 180 degrees"),"Rotate 180",        "8" );
  m_accel->insertItem( i18n("Rotate 270 degrees"),"Rotate 270",        "7" );
  m_accel->insertItem( i18n("Maximize"), 	 "Maximize", 	      "M" );
  m_accel->insertItem( i18n("Restore original size"), "OriginalSize", "O" );
  m_accel->insertItem( i18n("More Brightness"),  "More Brightness",   "B");
  m_accel->insertItem( i18n("Less Brightness"),  "Less Brightness", "SHIFT+B");
  m_accel->insertItem( i18n("More Contrast"),    "More Contrast",     "C");
  m_accel->insertItem( i18n("Less Contrast"),    "Less Contrast",   "SHIFT+C");
  m_accel->insertItem( i18n("More Gamma"),       "More Gamma",        "G");
  m_accel->insertItem( i18n("Less Gamma"),       "Less Gamma",      "SHIFT+G");
  m_accel->insertItem( i18n("Toggle Fullscreen mode"), "Toggle Fullscreen",
		       "Return" );
  m_accel->insertItem( i18n("Close"), 		 "Close Viewer",      "Q" );
  //  m_accel->insertItem( i18n(""), "", "" );

  m_accel->connectItem( "Scroll Up",         this, SLOT( scrollUp() ));
  m_accel->connectItem( "Scroll Down",       this, SLOT( scrollDown() ));
  m_accel->connectItem( "Scroll Left",       this, SLOT( scrollLeft() ));
  m_accel->connectItem( "Scroll Right",      this, SLOT( scrollRight() ));
  m_accel->connectItem( "Zoom In",           this, SLOT( zoomIn() ));
  m_accel->connectItem( "Zoom Out",          this, SLOT( zoomOut() ));
  m_accel->connectItem( "Flip Horizontally", this, SLOT( flipHoriz() ));
  m_accel->connectItem( "Flip Vertically",   this, SLOT( flipVert() ));
  m_accel->connectItem( "Rotate 90",         this, SLOT( rotate90() ));
  m_accel->connectItem( "Rotate 180",        this, SLOT( rotate180() ));
  m_accel->connectItem( "Rotate 270",        this, SLOT( rotate270() ));
  m_accel->connectItem( "Maximize", 	     this, SLOT( maximize() ));
  m_accel->connectItem( "OriginalSize", this, SLOT( showImageOriginalSize() ));
  m_accel->connectItem( "More Brightness"  , this, SLOT( moreBrightness() ));
  m_accel->connectItem( "Less Brightness"  , this, SLOT( lessBrightness() ));
  m_accel->connectItem( "More Contrast",     this, SLOT( moreContrast() ));
  m_accel->connectItem( "Less Contrast",     this, SLOT( lessContrast() ));
  m_accel->connectItem( "More Gamma",        this, SLOT( moreGamma() ));
  m_accel->connectItem( "Less Gamma",        this, SLOT( lessGamma() ));
  m_accel->connectItem( "Toggle Fullscreen", this, SLOT( toggleFullscreen() ));
  m_accel->connectItem( "Close Viewer",      this, SLOT( close() ));

  m_accel->readSettings();
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

	setFixedSize( QApplication::desktop()->size() );

	KWin::setType( winId(), NET::Override );
	KWin::setState( winId(), NET::StaysOnTop );

	setGeometry( QApplication::desktop()->geometry() );
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
	if ( wasInitialFullscreen )
	    hide(); show();
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
    setCaption( caption );
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
    if ( key == Key_Escape || KStdAccel::isEqual( e, KStdAccel::close() ))
	close( true );
    else if ( KStdAccel::isEqual( e, KStdAccel::save() ))
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

    if ( e->button() == RightButton )
	viewerMenu->popup( mapToGlobal( e->pos() ) );
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
	if ( w == QApplication::desktop()->width() &&
	     h == QApplication::desktop()->height() &&
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
  
  itemBrightnessPlus = brightnessMenu->insertItem( i18n("+"), this,
						   SLOT( moreBrightness() ));
  itemBrightnessMinus = brightnessMenu->insertItem( i18n("-"), this,
						    SLOT( lessBrightness() ));

  contrastMenu = new QPopupMenu( viewerMenu );
  itemContrastPlus = contrastMenu->insertItem( i18n("+"), this,
					       SLOT( moreContrast() ));
  itemContrastMinus = contrastMenu->insertItem( i18n("-"), this,
						SLOT( lessContrast() ));

  gammaMenu = new QPopupMenu( viewerMenu );
  itemGammaPlus = gammaMenu->insertItem( i18n("+"), this, SLOT( moreGamma() ));
  itemGammaMinus = gammaMenu->insertItem( i18n("-"), this,SLOT( lessGamma() ));

  itemViewerZoomIn = viewerMenu->insertItem( i18n("Zoom in"), this,
					     SLOT( zoomIn() ));
  itemViewerZoomOut = viewerMenu->insertItem( i18n("Zoom out"), this,
					      SLOT( zoomOut() ));
  viewerMenu->insertSeparator();
  itemRotate90    = viewerMenu->insertItem( i18n("Rotate 90 degrees"), this,
					    SLOT( rotate90() ));
  itemRotate180   = viewerMenu->insertItem( i18n("Rotate 180 degrees"), this,
					    SLOT( rotate180() ));
  itemRotate270   = viewerMenu->insertItem( i18n("Rotate 270 degrees"), this,
					    SLOT( rotate270() ));
  viewerMenu->insertSeparator();
  itemViewerFlipH = viewerMenu->insertItem( i18n("Flip horizontally"), this,
					    SLOT( flipHoriz() ));
  itemViewerFlipV = viewerMenu->insertItem( i18n("Flip vertically"), this,
					    SLOT( flipVert() ));
  viewerMenu->insertSeparator();
  viewerMenu->insertItem( i18n("Brightness"), brightnessMenu );
  viewerMenu->insertItem( i18n("Contrast"), contrastMenu );
  viewerMenu->insertItem( i18n("Gamma"), gammaMenu );
  viewerMenu->insertSeparator();
  itemViewerPrint = viewerMenu->insertItem( i18n("Print image..."), this,
					    SLOT( printImage() ));
  itemViewerSave = viewerMenu->insertItem( i18n("Save as..."), this,
					   SLOT( saveImage() ));
  viewerMenu->insertSeparator();
  itemViewerClose = viewerMenu->insertItem( i18n("Close"), this,
					    SLOT( close() ));

  setPopupAccels();
}


void ImageWindow::setPopupAccels()
{
  m_accel->changeMenuAccel(brightnessMenu, itemBrightnessPlus,
			   "More Brightness");
  m_accel->changeMenuAccel(brightnessMenu, itemBrightnessMinus,
			   "Less Brightness");
  m_accel->changeMenuAccel(contrastMenu, itemContrastPlus,    "More Contrast");
  m_accel->changeMenuAccel(contrastMenu, itemContrastMinus,   "Less Contrast");
  m_accel->changeMenuAccel(gammaMenu,    itemGammaPlus,       "More Gamma");
  m_accel->changeMenuAccel(gammaMenu,    itemGammaMinus,      "Less Gamma");

  m_accel->changeMenuAccel(viewerMenu,  itemViewerZoomIn,  "Zoom In" );
  m_accel->changeMenuAccel(viewerMenu,  itemViewerZoomOut, "Zoom Out" );
  m_accel->changeMenuAccel(viewerMenu,  itemRotate90,      "Rotate 90" );
  m_accel->changeMenuAccel(viewerMenu,  itemRotate180,     "Rotate 180" );
  m_accel->changeMenuAccel(viewerMenu,  itemRotate270,     "Rotate 270" );
  m_accel->changeMenuAccel(viewerMenu,  itemViewerFlipH,   "Flip Horizontally" );
  m_accel->changeMenuAccel(viewerMenu,  itemViewerFlipV,   "Flip Vertically" );
  m_accel->changeMenuAccel(viewerMenu,  itemViewerPrint,   KStdAccel::Print );
  m_accel->changeMenuAccel(viewerMenu,  itemViewerSave,    KStdAccel::Save );
  m_accel->changeMenuAccel(viewerMenu,  itemViewerClose,   "Close Viewer" );
}


void ImageWindow::printImage()
{
  QString printCmd, printerName;

  QPrinter p;
  if ( !QPrintDialog::getPrinterSetup( &p ) )
    return;

  // map Qt's options to imlibs'
  int pagesize  = PAGE_SIZE_A4;	// default value: Din A4
  int colormode = 0; 		// default value: grayscale
  int copies    = 1;		// just print one copy
  bool ofile    = false;        // don't output as file

  QPrinter::PageSize ps = p.pageSize();

  if ( ps == QPrinter::A4 )
    pagesize = PAGE_SIZE_A4;

  else if ( ps == QPrinter::Letter )
    pagesize = PAGE_SIZE_LETTER;

  else if ( ps == QPrinter::Legal )
    pagesize = PAGE_SIZE_LEGAL;

  else if ( ps == QPrinter::Executive )
    pagesize = PAGE_SIZE_EXECUTIVE;

  if ( p.colorMode() == QPrinter::Color )
    colormode = 1;

  copies      = p.numCopies();
//  printCmd    = p.printProgram();
// since Qt2, printProgram returns a null string by default!!!!!!!!!!
  KConfig *config = KGlobal::config();
  KConfigGroupSaver cs( config, "GeneralConfiguration" );
  printCmd    = config->readEntry( "PrinterCommand", "lpr" );
  printerName = p.printerName();
  ofile       = p.outputToFile();

  ImlibSaveInfo info;

  info.page_size      = pagesize;
  info.color          = colormode;
  info.scaling        = 1024; // how to save in original size, without scaling?
  info.xjustification = 512;  // center
  info.yjustification = 512;  // center

  KTempFile tmpFile( "kuickshow", ".ppm" );
  tmpFile.setAutoDelete( true );
  QString tmpName;
  if ( ofile ) // user just wants to print to file
    tmpName = p.outputFileName();
  else
    tmpName = tmpFile.name();

  if ( Imlib_save_image( id, kuim->imlibImage(),
                         QFile::encodeName( tmpName ).data(), &info ) == 0 )
  {
    qDebug("KuickShow: Couldn't print image."); // FIXME, show messagebox
    return;
  }

  if ( ofile ) // done, user just wanted that postscript file
    return;

  // finally print the postscript file
  QString cmdline = printCmd + " -P\"" + printerName + "\" " + tmpName;
  qDebug("KuickShow: print commandline: %s", cmdline.local8Bit().data() );
  for ( int i=0; i < copies; i++ ) // FIXME: better use a switch in lpr...
    system( QFile::encodeName( cmdline ).data() );
}


void ImageWindow::saveImage()
{
  QString file;
  KuickData tmp;
  file = KFileDialog::getSaveFileName( kuim->filename(), tmp.fileFilter );
  if ( !file.isEmpty() )
  {
    bool success = false;
    ImlibImage *saveIm = 0L;
    saveIm = Imlib_clone_scaled_image( id, kuim->imlibImage(),
				       kuim->width(), kuim->height() );
    if ( saveIm ) {
      Imlib_apply_modifiers_to_rgb( id, saveIm );
      if ( Imlib_save_image( id, saveIm,
                             QFile::encodeName( file ).data(), NULL ) )
	success = true;
      // FIXME: remove the old image from cache and load the new one?
    }

    if ( !success ) {
      QString tmp = i18n("Couldn't save the file,\n"
    		"maybe this disk is full, or you don't\n"
    		"have write permissions to the file.");
      QMessageBox::warning( this, i18n("File saving failed"), tmp );
    }
  }
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
    if ( myIsFullscreen || totalScreen )
	return QApplication::desktop()->width();

    else
	return Kuick::workArea().width();
}


int ImageWindow::desktopHeight( bool totalScreen ) const
{
    if ( myIsFullscreen || totalScreen )
	return QApplication::desktop()->height();

    else
	return Kuick::workArea().height();
}

QSize ImageWindow::maxImageSize() const
{
    if ( myIsFullscreen || initialFullscreen )
	return QApplication::desktop()->size();
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

#include "imagewindow.moc"
