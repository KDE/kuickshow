/* This file is part of the KDE project
   Copyright (C) 1998-2003 Carsten Pfeiffer <pfeiffer@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>

#include <kmainwindow.h>

#include <qcheckbox.h>
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
#include <kglobalsettings.h>
#include <kio/netaccess.h>
#include <kiconloader.h>
#include <kimageeffect.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprinter.h>
#include <kpropertiesdialog.h>
#include <kstdaccel.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <kurldrag.h>
#include <kwin.h>
#include <netwm.h>

#include "kuick.h"
#include "kuickdata.h"
#include "printing.h"
#include "imagewindow.h"

#undef GrayScale

QCursor *ImageWindow::s_handCursor = 0L;

ImageWindow::ImageWindow( ImData *_idata, QWidget *parent, const char *name )
   : KMainWindow(parent, name, WDestructiveClose), image( _idata, this, name )
{
    idata=_idata;

    init();
}

ImageWindow::~ImageWindow()
{
	kdata->save();
}


void ImageWindow::init()
{
    KCursor::setAutoHideCursor( this, true, true );
    KCursor::setHideCursorDelay( 1500 );

    viewerMenu = 0L;
    gammaMenu = 0L;
    brightnessMenu = 0L;
    contrastMenu = 0L;
    xpos=0; ypos=0;
    
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

    xpos = 0, ypos = 0;

    setAcceptDrops( true );

    setupIcons();
    
    setCentralWidget(&image);
    
    setBackgroundColor(kdata->backgroundColor);
    
    connect( &image, SIGNAL( loaded( KuickImage * )),
            this, SLOT( loaded( KuickImage * )));
}

void ImageWindow::setupIcons()
{
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

    new KAction( i18n("Rotate Clockwise"), Key_9,
                 &image, SLOT( rotate270() ),
                 m_actions, "rotate270" );		 
    new KAction( i18n("Rotate 180 degrees"), Key_8,
                 &image, SLOT( rotate180() ),
                 m_actions, "rotate180" );
    new KAction( i18n("Rotate Counterclockwise"), Key_7,
                 &image, SLOT( rotate90() ),
                 m_actions, "rotate90" );


    new KAction( i18n("Flip Horizontally"), Key_Asterisk,
                 &image, SLOT( flipHoriz() ),
                 m_actions, "flip_horicontally" );
    new KAction( i18n("Flip Vertically"), Key_Slash,
                 &image, SLOT( flipVert() ),
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
		 
    new KAction( i18n("Hide Toolbar"), Key_T,
                 this, SLOT( slotToggleToolBar() ),
                 m_actions, "toggle_toolbar" );
    new KAction (i18n("Pause Slideshow"), Key_Space,
    		this,SLOT( slotPauseSlideshow() ),
		m_actions, "pause_slideshow" );
		
		 
#if KDE_IS_VERSION(3,2,0)
    KShortcut cut(Key_Return);
    cut.append(KStdAccel::shortcut(KStdAccel::FullScreen));

    KAction *action = KStdAction::fullScreen(this, SLOT( toggleFullscreen() ), m_actions, 0, "toggle_fullscreen");		     
    
    action->setShortcut(cut);
    action->setIcon(QString("window_fullscreen"));
#else
     new KAction( i18n("Toggle Fullscreen mode"), Key_Return, 
                  this, SLOT( toggleFullscreen() ),
                  m_actions, "toggle_fullscreen" );
#endif

    new KAction( i18n("Reload Image"), Key_Enter,
                 this, SLOT( reload() ),
                 m_actions, "reload_image" );

    new KAction( i18n("Properties..."), ALT + Key_Return,
                 this, SLOT( slotProperties() ),
                 m_actions, "properties" );

    m_actions->readShortcutSettings();
    
    m_actions->action("previous_image")->setIcon(QString("previous"));
    m_actions->action("next_image")->setIcon(QString("next"));
    m_actions->action("zoom_in")->setIcon(QString("viewmag+"));
    m_actions->action("zoom_out")->setIcon(QString("viewmag-"));
    m_actions->action("original_size")->setIcon(QString("viewmag1"));
    m_actions->action("maximize")->setIcon(QString("viewmagfit"));
    m_actions->action("close_image")->setIcon(QString("fileclose"));
    m_actions->action("rotate270")->setIcon(QString("rotate_cw"));
    m_actions->action("rotate90")->setIcon(QString("rotate_ccw"));
    m_actions->action("rotate180")->setIcon(QString("rotate"));
    m_actions->action("save_image_as")->setIcon(QString("filesaveas"));
    m_actions->action("print_image")->setIcon(QString("fileprint"));
    m_actions->action("reload_image")->setIcon(QString("reload"));
    m_actions->action("toggle_toolbar")->setIcon(QString("showmenu"));
    
    m_actions->action("previous_image")->plug( toolBar() );
    m_actions->action("next_image")->plug( toolBar() );
    toolBar()->insertSeparator();
    m_actions->action("zoom_in")->plug( toolBar() );
    m_actions->action("zoom_out")->plug( toolBar() );
    m_actions->action("original_size")->plug( toolBar() );
    m_actions->action("maximize")->plug( toolBar() );
    toolBar()->insertSeparator();
    m_actions->action("toggle_toolbar")->plug( toolBar());

    toolBar()->insertSeparator();
    action->plug(toolBar());
    
    toolBar()->setTitle(i18n("Navigation"));
    
    setToolBarVisible(kdata->showImageWindowToolBar);
    
}

void ImageWindow::showImage()
{
	image.setViewportPosition(QPoint(-xpos, -ypos));
	image.showImage();
}

void ImageWindow::setFullscreen( bool enable )
{
    xpos = 0; ypos = 0;

    if ( enable && !myIsFullscreen ) { // set Fullscreen
        showFullScreen();
    }

    else if ( !enable && myIsFullscreen ) { // go into window mode
        showNormal();
	setupIcons();
    }

    m_actions->action("toggle_fullscreen")->setIcon(QString((enable)?"window_nofullscreen":"window_fullscreen"));
    
    myIsFullscreen = enable;
    centerImage(); // ### really necessary (multihead!)
    showImage();
}

void ImageWindow::showImageOriginalSize()
{
	image.showImageOriginalSize();
	
	QSize s=image.originalImageSize();
	resizeOptimal(s.width(), s.height());
}

void ImageWindow::updateGeometry( int imWidth, int imHeight )
{
     qDebug("::updateGeometry: %i, %i", imWidth, imHeight);

    if ( imWidth != image.width() || imHeight != image.height() ) {
	if ( myIsFullscreen ) {
	    centerImage();
	    showImage();
	}
	else { // window mode
	    resizeOptimal( imWidth, imHeight ); // also centers the image
	}
    }
    else { // image size == widget size
	xpos = 0; ypos = 0;
	showImage();
    }

    QString caption = i18n( "Filename (Imagewidth x Imageheight)",
                            "%3 (%1 x %2)" );
    QSize s=image.originalImageSize();
    caption = caption.arg( s.width() ).
              arg( s.height() ).arg( image.imageFilename() );
    setCaption( kapp->makeStdCaption( caption ) );
}


void ImageWindow::centerImage()
{
    int w, h;
    /*if ( myIsFullscreen )
    {
        QRect desktopRect = KGlobalSettings::desktopGeometry( this );
        w = desktopRect.width();
        h = desktopRect.height();
    }
    else
    {*/
        w = image.width();
        h = image.height();
    /*}*/

    if (w == 0 || h == 0)
    {
    	xpos=ypos=0;
	image.setViewportPosition(QPoint(-xpos, -ypos));
	return;
    }
    
    xpos = w/2 - image.imageWidth()/2;
    ypos = h/2 - image.imageHeight()/2;
    //kdDebug() << "Centering image to " << -xpos << ", " << -ypos << endl;
    image.setViewportPosition(QPoint(-xpos, -ypos));
}


void ImageWindow::scrollImage( int x, int y, bool restrict )
{
    xpos += x;
    ypos += y;

    int cwlocal = width();
    int chlocal = height();

    int iw = image.imageWidth();
    int ih = image.imageHeight();

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
//	geometryUpdate();
//		if a repaint would not otherwise be forced, showImage()
//
bool ImageWindow::showNextImage( const QString& filename )
{
    if ( !image.loadImage( filename ) ) {
	emit sigBadImage( filename );
	return false;
    }

    else {
    	int w=image.imageWidth();
	int h=image.imageHeight();
	updateGeometry(w, h);
	centerImage();
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

    int oldValue = image.brightness();
    image.setBrightness( oldValue + (idata->brightnessFactor * (int) factor) );
}

void ImageWindow::addContrast( int factor )
{
    if ( factor == 0 )
	return;

    int oldValue = image.contrast();
    image.setContrast( oldValue + (idata->contrastFactor * (int) factor) );
}

void ImageWindow::addGamma( int factor )
{
    if ( factor == 0 )
	return;

    int oldValue = image.gamma();
    image.setGamma( oldValue + (idata->gammaFactor * (int) factor) );
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
    scrollImage( -20 * kdata->scrollSteps, 0 );
}

///

void ImageWindow::zoomIn()
{
    image.zoomImage( kdata->zoomSteps );
    QRect r=image.getViewport();
    xpos=-r.x();
    ypos=-r.x();
}

void ImageWindow::zoomOut()
{
    Q_ASSERT( kdata->zoomSteps != 0 );
    image.zoomImage( 1.0 / kdata->zoomSteps );
    QRect r=image.getViewport();
    xpos=-r.x();
    ypos=-r.x();
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
    if ( key == Key_Shift )
        updateCursor( ZoomCursor );

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

    if ( e->button() == LeftButton ) {
        if ( e->state() & ShiftButton )
            updateCursor( ZoomCursor );
        else
            setCursor( *s_handCursor );
    }

    image.mousePressEvent( e );
}

void ImageWindow::updateCursor( KuickCursor cursor )
{
    switch ( cursor )
    {
        case ZoomCursor:
            setCursor( arrowCursor ); // need a magnify-cursor
            break;
        case MoveCursor:
            setCursor( *s_handCursor );
            break;
        case DefaultCursor:
        default:
            if ( image.imageWidth() > width() || image.imageHeight() > height() )
                setCursor( *s_handCursor );
            else
                setCursor( arrowCursor );
            break;
    }
}

void ImageWindow::contextMenuEvent( QContextMenuEvent *e )
{
    e->accept();

    if ( !viewerMenu )
        setPopupMenu();

    viewerMenu->popup( e->globalPos() );
}

void ImageWindow::mouseMoveEvent( QMouseEvent *e )
{
    if ( !(e->state() & LeftButton) ) { // only handle LeftButton actions
	return;
    }

    if ( e->state() & ShiftButton ) {
	
	if ( !transWidget ) {
	    transWidget = new QWidget( this );
	    transWidget->setGeometry( 0, 0, width(), height() );
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
    updateCursor();

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

    factorx = ((float) width() / (float) neww);
    factory = ((float) height() / (float) newh);

    if ( factorx < factory ) // use the smaller factor
	factor = factorx;
    else factor = factory;

    uint w = 0; // shut up compiler!
    uint h = 0;
    w = (uint) ( factor * (float) image.imageWidth() );
    h = (uint) ( factor * (float) image.imageHeight() );

    if ( w > kdata->maxWidth || h > kdata->maxHeight ) {
	kdDebug() << "KuickShow: scaling larger than configured maximum -> aborting" << endl;
	return;
    }

    int xtmp = - (int) (factor * abs(xpos - topX) );
    int ytmp = - (int) (factor * abs(ypos - topY) );

    // if image has different ratio (width()/height()), center it
    int xcenter = (width()  - (int) (neww * factor)) / 2;
    int ycenter = (height() - (int) (newh * factor)) / 2;

    xtmp += xcenter;
    ytmp += ycenter;

    image.zoomImage(factor);
    //m_kuim->resize( w, h );

    image.updateWidget( false );

    xpos = xtmp; ypos = ytmp;

    scrollImage( 1, 1, true ); // unrestricted scrolling
}


void ImageWindow::focusInEvent( QFocusEvent * )
{
    emit sigFocusWindow( this );
}


void ImageWindow::resizeEvent( QResizeEvent * )
{
    //image.resizeEvent( e );

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
    KURL::List list;
    if ( KURLDrag::decode( e, list ) && !list.isEmpty()) {
        QString tmpFile;
        const KURL &url = list.first();
        if (KIO::NetAccess::download( url, tmpFile, this ) )
        {
            image.loadImage( tmpFile );
            KIO::NetAccess::removeTempFile( tmpFile );
        }
        image.updateWidget();
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
    m_actions->action("rotate270")->plug( viewerMenu );
    m_actions->action("rotate180")->plug( viewerMenu );
    m_actions->action("rotate90")->plug( viewerMenu );

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
    m_actions->action("toggle_toolbar")->plug( viewerMenu );
    
    viewerMenu->insertSeparator();
    m_actions->action("close_image")->plug( viewerMenu );
}

void ImageWindow::printImage()
{
	kdDebug() << "ImageWindow::printImage() unimplemented." << endl;
/*
    if ( !image.imageLoaded() )
        return;

    if ( !Printing::printImage( *this, this ) )
    {
        KMessageBox::sorry( this, i18n("Unable to print the image."),
                            i18n("Printing Failed") );
    }
*/
}

void ImageWindow::saveImage()
{
    if ( !image.imageLoaded() )
        return;

    KuickData tmp;
    QCheckBox *keepSize = new QCheckBox( i18n("Keep original image Size"), 0L);
    keepSize->setChecked( true );
    KFileDialog dlg( m_saveDirectory, tmp.fileFilter, this, "filedialog", true
#if KDE_IS_VERSION(3,1,0)
                     ,keepSize
#endif
                   );

    QString selection = m_saveDirectory.isEmpty() ?
                            image.filename() :
                            KURL::fromPathOrURL( image.filename() ).fileName();
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
                QString tmp = i18n("Couldn't save the file.\n"
                                   "Perhaps the disk is full, or you don't "
                                   "have write permission to the file.");
                KMessageBox::sorry( this, tmp, i18n("File Saving Failed"));
            }
        }
    }

    QString lastDir = dlg.baseURL().path(+1);
    if ( lastDir != m_saveDirectory )
        m_saveDirectory = lastDir;

#if !KDE_IS_VERSION(3,1,0)
    delete keepSize;
#endif
}

bool ImageWindow::saveImage( const QString& /*filename*/, bool /*keepOriginalSize*/ ) const
{
  /*  int w = keepOriginalSize ? m_kuim->originalWidth()  : m_kuim->width();
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

    return success;*/
    
    kdDebug() << "ImageWindow::saveImage() unimplemented." << endl;
    
    return false;
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
	kuim->renderPixmap();
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
    if ( kdata->autoRotation && image.autoRotate( kuim ) )
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

QSize ImageWindow::maxWindowSize()
{
    if ( myIsFullscreen ) {
        return KGlobalSettings::desktopGeometry(topLevelWidget()).size();
    }
    else {
	return Kuick::workArea().size() - Kuick::frameSize( winId() );
    }
}

QSize ImageWindow::maxImageSize()
{
    QSize windowsize=maxWindowSize();
    
    if (!toolBar()->isVisible())
    	return windowsize;
    
    //Subtract toolbar dimensions if toolbar is showing   
    switch (toolBar()->barPos())
    {
    	case KToolBar::Unmanaged:
    	case KToolBar::Floating:
		return windowsize;
		
	case KToolBar::Flat:
	case KToolBar::Top:
	case KToolBar::Bottom:
		windowsize.setHeight(windowsize.height()-toolBar()->height());
		return windowsize;
		
	case KToolBar::Left:
	case KToolBar::Right:
		windowsize.setWidth(windowsize.width()-toolBar()->width());
		return windowsize;
		
	default:
		kdDebug() << "Unknown KToolBar::BarPosition" << endl;
		return windowsize;
    }
}

void ImageWindow::resizeOptimal( int nw, int nh )
{
    int w=nw;
    int h=nh;

    QSize s = maxWindowSize();
    int mw = s.width();
    int mh = s.height();

    if (toolBar()->isVisible())
    {    
	//Add toolbar dimensions if toolbar is showing   
	switch (toolBar()->barPos())
	{
		case KToolBar::Unmanaged:
		case KToolBar::Floating:
			break;
			
		case KToolBar::Flat:
		case KToolBar::Top:
		case KToolBar::Bottom:
			h+=toolBar()->height();
			break;
			
		case KToolBar::Left:
		case KToolBar::Right:
			w+=toolBar()->width();
			break;
			
		default:
			kdDebug() << "Unknown KToolBar::BarPosition" << endl;
			break;
	}
    }
    
    int neww = (w >= mw) ? mw : w;
    int newh = (h >= mh) ? mh : h;
    
    if ( neww == width() && newh == height() )
    {
	centerImage();
	showImage();
    }
    else
    {
	resize( neww, newh );
	centerImage();
	showImage();
    }
}

void ImageWindow::maximize()
{
    if ( image.imageLoaded() )
	return;

    bool oldUpscale = kdata->upScale;
    bool oldDownscale = kdata->downScale;

    kdata->upScale = true;
    kdata->downScale = true;

    image.autoScaleImage(maxImageSize());
    //updateWidget( true );

    if ( !myIsFullscreen )
	resizeOptimal( image.imageWidth(), image.imageHeight() );

    kdata->upScale = oldUpscale;
    kdata->downScale = oldDownscale;
}

void ImageWindow::slotProperties()
{
    KURL url;
    url.setPath( filename() ); // ###
    (void) new KPropertiesDialog( url, this, "props dialog", true );
}

void ImageWindow::setToolBarVisible(bool visible)
{
	if (visible)
	{
		m_actions->action("toggle_toolbar")->setText(i18n("Hide Toolbar"));
		toolBar()->show();
	}
	else
	{
		m_actions->action("toggle_toolbar")->setText(i18n("Show Toolbar"));
		toolBar()->hide();
	}
	
	kdata->showImageWindowToolBar=visible;
}

void ImageWindow::slotToggleToolBar()
{
	setToolBarVisible(!toolBar()->isVisible());

}

#include "imagewindow.moc"
