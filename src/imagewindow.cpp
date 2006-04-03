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

#include <stdlib.h>

#include <qcheckbox.h>
#include <qcursor.h>
#include <qdrawutil.h>
#include <qfileinfo.h>
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
#include <kstdguiitem.h>
#include <kstandarddirs.h>
#include <kglobalsettings.h>
#include <ktempfile.h>
#include <kwin.h>
#include <netwm.h>
#include <kurldrag.h>
#include <kio/netaccess.h>

#include "filecache.h"
#include "imagewindow.h"
#include "kuick.h"
#include "kuickdata.h"
#include "kuickfile.h"
#include "kuickimage.h"
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
    setFocusPolicy( QWidget::StrongFocus );
    
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
        QString file = locate( "appdata", "pics/handcursor.png" );
        if ( !file.isEmpty() )
            s_handCursor = new QCursor( file );
        else
            s_handCursor = new QCursor( arrowCursor );
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

    new KAction( i18n("Delete Image"), SHIFT + Key_Delete,
                 this, SLOT( imageDelete() ),
                 m_actions, "delete_image" );
    new KAction( i18n("Move Image to Trash"), Key_Delete,
                 this, SLOT( imageTrash() ),
                 m_actions, "trash_image" );

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

    new KAction( i18n("Rotate 90 Degrees"), Key_9,
                 this, SLOT( rotate90() ),
                 m_actions, "rotate90" );
    new KAction( i18n("Rotate 180 Degrees"), Key_8,
                 this, SLOT( rotate180() ),
                 m_actions, "rotate180" );
    new KAction( i18n("Rotate 270 Degrees"), Key_7,
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
    KStdAction::saveAs( this, SLOT( saveImage() ),
                 m_actions, "save_image_as" );

    KStdAction::close( this, SLOT( close() ),
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
    // --------
    new KAction( i18n("Pause Slideshow"), Key_P,
                 this, SLOT( pauseSlideShow() ),
                 m_actions, "kuick_slideshow_pause" );

    KAction *fullscreenAction = KStdAction::fullScreen(this, SLOT( toggleFullscreen() ), m_actions, 0 );

    KAction *reloadAction = new KAction( i18n("Reload Image"), KStdAccel::shortcut(KStdAccel::Reload),
                                         this, SLOT( reload() ),
                                         m_actions, "reload_image" );

    new KAction( i18n("Properties"), ALT + Key_Return,
                 this, SLOT( slotProperties() ),
                 m_actions, "properties" );

    m_actions->readShortcutSettings();

    // Unfortunately there is no KAction::setShortcutDefault() :-/
    // so add Key_Return as fullscreen shortcut _after_ readShortcutSettings()
    addAlternativeShortcut(fullscreenAction, Key_Return);
    addAlternativeShortcut(reloadAction, Key_Enter);
}

void ImageWindow::addAlternativeShortcut(KAction *action, int key)
{
    KShortcut cut( action->shortcut() );
    if (cut == action->shortcutDefault()) {
        cut.append(KKey(key));
        action->setShortcut(cut);
    }
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
	
    QString caption = i18n( "Filename (Imagewidth x Imageheight)",
                            "%3 (%1 x %2)" );
    caption = caption.arg( m_kuim->originalWidth() ).
              arg( m_kuim->originalHeight() ).arg( m_kuim->url().prettyURL() );
    setCaption( kapp->makeStdCaption( caption ) );
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
bool ImageWindow::showNextImage( const KURL& url )
{
    KuickFile *file = FileCache::self()->getFile( url );
    switch ( file->waitForDownload( this ) ) {
    	case KuickFile::ERROR:
    	{
    	    QString tmp = i18n("Unable to download the image from %1.").arg(url.prettyURL());
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
                       "your Imlib is not installed properly.").arg(file->url().prettyURL());
        emit sigImageError( file, tmp );
        return false;
    }

    else {
	// updateWidget( true ); // already called from loadImage()
	if ( !isVisible() )
		showWindow();
		
	showImage();
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
            setCursor( arrowCursor ); // need a magnify-cursor
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
                setCursor( arrowCursor );
            break;
    }
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
    XResizeWindow( x11Display(), win, w, h );
    updateWidget( false );

    xpos = xtmp; ypos = ytmp;

    XMoveWindow( x11Display(), win, xpos, ypos );
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
    if ( e->provides( "text/uri-list" ) )
	e->accept();
    else
	e->ignore();
}


void ImageWindow::dropEvent( QDropEvent *e )
{
    // FIXME - only preliminary drop-support for now
    KURL::List list;
    if ( KURLDrag::decode( e, list ) && !list.isEmpty()) {
        QString tmpFile;
        const KURL &url = list.first();
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

    m_actions->action("delete_image")->plug( viewerMenu );
    m_actions->action("print_image")->plug( viewerMenu );
    m_actions->action("save_image_as")->plug( viewerMenu );
    m_actions->action("properties")->plug( viewerMenu );

    viewerMenu->insertSeparator();
    m_actions->action("close_image")->plug( viewerMenu );
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
    KFileDialog dlg( m_saveDirectory, tmp.fileFilter, this, "filedialog", true
#if KDE_VERSION >= 310
                     ,keepSize
#endif
                   );

    QString selection = m_saveDirectory.isEmpty() ?
                            m_kuim->url().url() :
                            m_kuim->url().fileName();
    dlg.setOperationMode( KFileDialog::Saving );
    dlg.setMode( KFile::File );
    dlg.setSelection( selection );
    dlg.setCaption( i18n("Save As") );
    if ( dlg.exec() == QDialog::Accepted )
    {
        KURL url = dlg.selectedURL();
        if ( url.isValid() )
        {
            if ( !saveImage( url, keepSize->isChecked() ) )
            {
                QString tmp = i18n("Couldn't save the file.\n"
                                   "Perhaps the disk is full, or you don't "
                                   "have write permission to the file.");
                KMessageBox::sorry( this, tmp, i18n("File Saving Failed"));
            }
			else
			{
	            if ( url.equals( m_kuim->url() )) {
	                Imlib_apply_modifiers_to_rgb( id, m_kuim->imlibImage() );
	            }
            }
        }
    }

    QString lastDir = dlg.baseURL().path(+1);
    if ( lastDir != m_saveDirectory )
        m_saveDirectory = lastDir;

#if KDE_VERSION < 310
    delete keepSize;
#endif
}

bool ImageWindow::saveImage( const KURL& dest, bool keepOriginalSize )
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
		QString extension = QFileInfo( dest.fileName() ).extension();
		if ( !extension.isEmpty() )
			extension.prepend( '.' );
		
		KTempFile tmpFile( QString::null, extension );
		if ( tmpFile.status() != 0 )
			return false;
		tmpFile.close();
		if ( tmpFile.status() != 0 )
			return false;
		saveFile = tmpFile.name();		
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
        	success = KIO::NetAccess::upload( saveFile, dest, const_cast<ImageWindow*>( this ) );
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

void ImageWindow::loaded( KuickImage *kuim )
{
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

bool ImageWindow::canZoomTo( int newWidth, int newHeight )
{
    if ( !ImlibWidget::canZoomTo( newWidth, newHeight ) )
        return false;
    
    QSize desktopSize = KGlobalSettings::desktopGeometry(topLevelWidget()).size();

    int desktopArea = desktopSize.width() * desktopSize.height();
    int imageArea = newWidth * newHeight;
    
    if ( imageArea > desktopArea * kdata->maxZoomFactor )
    {
        return KMessageBox::warningContinueCancel(
            this,
            i18n("You are about to view a very large image (%1 x %2 pixels), which can be very resource-consuming and even make your computer hang.\nDo you want to continue?")
            .arg( newWidth ).arg( newHeight ),
            QString::null,
            KStdGuiItem::cont(),
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
    (void) new KPropertiesDialog( currentFile()->url(), this, "props dialog", true );
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
    
#include "imagewindow.moc"
