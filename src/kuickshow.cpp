/****************************************************************************
** $Id$
**
** KuickShow - a fast and comfortable image viewer based on Rasterman's Imlib
**
** Created : 98
**
** Copyright (C) 1998-2001 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/

#include <stdio.h>
#include <assert.h>

#include <qdir.h>
#include <qdialog.h>
#include <qglobal.h>
#include <qkeycode.h>
#include <qsize.h>
#include <qstring.h>

#include <kaccel.h>
#include <kaction.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kcursor.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpropsdlg.h>
#include <kstatusbar.h>
#include <kstdaction.h>
#include <kstandarddirs.h>
#include <ktoolbar.h>
#include <kurldrag.h>
#include <kwin.h>

#include "aboutwidget.h"
#include "filewidget.h"
#include "imdata.h"
#include "imagewindow.h"
#include "imlibwidget.h"
#include "kuick.h"
#include "kuickio.h"

#ifdef index
#undef index
#endif

#include "kuickconfigdlg.h"
#include "kuickdata.h"
#include "kuickshow.h"
#include "version.h"

#ifdef KeyPress
#undef KeyPress
#endif

KuickData* kdata;

static const int SIZE_ITEM = 0;
static const int URL_ITEM  = 1;

QValueList<ImageWindow*> KuickShow::s_viewers;


KuickShow::KuickShow( const char *name )
    : KMainWindow( 0L, name ),
      fileWidget( 0L ),
      dialog( 0L ),
      id( 0L ),
      viewer( 0L ),
      newWindowAction( 0L ),
      m_accel( 0L ),
      m_delayedRepeatItem( 0L )
{
  kdata = new KuickData;
  kdata->load();

  initImlib();

  KConfig *kc = KGlobal::config();

  bool isDir = false; // true if we get a directory on the commandline

  // parse commandline options
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		
  // files to display
  // either a directory to display, an absolute path, a relative path, or a URL
  KURL base;
  base.setPath( QDir::currentDirPath() + '/' );
  KURL startDir = base;
  for ( int i = 0; i < args->count(); i++ ) {
      KURL url( base, args->arg( i ) );
      KFileItem item( -1, -1, url, false );

      // for remote URLs, we don't know if it's a file or directory, but
      // FileWidget::isImage() should correct in most cases.
      // For non-local non-images, we just assume directory.
      if ( FileWidget::isImage( &item ) )
	  showImage( &item, true ); // show in new window
      else {
	  if ( item.isDir() || !url.isLocalFile() ) {
	      startDir = url;
	      isDir = true;
	  }
      }
  }

  if ( args->isSet( "lastdir" ))
      startDir = kc->readEntry( "CurrentDirectory", startDir.url() );

  if ( s_viewers.isEmpty() || isDir ) {
      initGUI( startDir );
      show();
  }

  else { // don't show browser, when image on commandline
      hide();
  }
}


KuickShow::~KuickShow()
{
    if ( fileWidget )
	saveSettings();

    if ( viewer )
	viewer->close( true );

    delete id;
    kapp->quit();

    delete kdata; kdata = 0;
}


void KuickShow::initGUI( const KURL& startDir )
{
    m_accel = new KAccel( this );

    fileWidget = new FileWidget( startDir, this, "MainWidget" );
    setFocusProxy( fileWidget );

    KActionCollection *coll = fileWidget->actionCollection();

    connect( fileWidget, SIGNAL( fileSelected( const KFileItem * ) ),
	     this, SLOT( slotSelected( const KFileItem * ) ));

    connect( fileWidget, SIGNAL( fileHighlighted( const KFileItem * )),
	     this, SLOT( slotHighlighted( const KFileItem * ) ));

    connect( fileWidget, SIGNAL( urlEntered( const KURL&  )),
	     this, SLOT( dirSelected( const KURL& )) );

    // setup actions
    KAction *print = KStdAction::print( this, SLOT( slotPrint() ),
					coll, "kuick_print" );
    print->setText( i18n("Print image...") );

    // ### remove somewhen, is in kdelibs 2.2alpha2
    if ( !coll->action("delete") )
        (void) new KAction( i18n("Delete File"), "editdelete", 0,
                            this, SLOT( slotDelete()), coll, "kuick_delete" );

    KAction *configure = new KAction( i18n("Configuration"), "configure", 0,
				      this, SLOT( configuration() ),
				      coll, "kuick_configure" );
    KAction *slide = new KAction( i18n("Slideshow" ), "ksslide", Key_F2,
				  this, SLOT( startSlideShow() ),
				  coll, "kuick_slideshow" );
    KAction *about = new KAction( i18n( "About KuickShow" ), "about", 0,
				  this, SLOT( about() ), coll );
    KAction *help = KStdAction::help( this, SLOT( appHelpActivated() ),
				      coll, "kuick_help" );
    newWindowAction = new KToggleAction( i18n("Open only one image window"),
					 "window_new", CTRL+Key_N, coll,
					 "kuick_one window" );

    KAction *hidden = coll->action( "show hidden" );
    hidden->setIcon( "lock" );

    (void) new KAction( i18n("Show Image"), 0, this,
                        SLOT( slotShowInOtherWindow() ),
			coll, "kuick_showInOtherWindow" );
    (void) new KAction( i18n("Show Image in Active Window"), 0, this,
			SLOT( slotShowInSameWindow() ), coll,
			"kuick_showInSameWindow" );

    if ( !coll->action( "properties" ) ) { // maybe KDirOperator did it already
        (void) new KAction( i18n("Properties..."), ALT+Key_Return, this,
                            SLOT( slotShowProperties() ), coll, "properties" );
    }

    coll->action( "reload" )->setShortcut( KStdAccel::reload() );

    KAction *quit = KStdAction::quit( this, SLOT(slotQuit()), coll, "quit" );

    // plug in
    for ( uint i = 0; i < coll->count(); i++ )
	coll->action( i )->plugAccel( m_accel );

    m_accel->readSettings();

    KToolBar *tBar = toolBar();
    coll->action("up")->plug( tBar );
    coll->action("back")->plug( tBar );
    coll->action("forward")->plug( tBar );
    coll->action("home")->plug( tBar );
    coll->action("reload")->plug( tBar );

    tBar->insertSeparator();
    configure->plug ( tBar );
    slide->plug( tBar );
    tBar->insertSeparator();
    hidden->plug( tBar );
    newWindowAction->plug( tBar );
    print->plug( tBar );
    tBar->insertSeparator();
    quit->plug( tBar );
    about->plug( tBar );
    help->plug( tBar );


    KStatusBar* sBar = statusBar();
    sBar->insertItem( "                          ", SIZE_ITEM, 2 );
    sBar->insertItem( "           ", URL_ITEM, 10 );

    fileWidget->setFocus();

    KConfig *kc = KGlobal::config();
    kc->setGroup("SessionSettings");
    bool oneWindow = kc->readBoolEntry("OpenImagesInActiveWindow", true );
    newWindowAction->setChecked( oneWindow );

    tBar->show();

    fileWidget->initActions();
    fileWidget->clearHistory();
    dirSelected( fileWidget->url() );

    setCentralWidget( fileWidget );
    setAutoSaveSettings();
}


void KuickShow::viewerDeleted()
{
    s_viewers.remove( (ImageWindow*) sender() );
    viewer = 0L;

    if ( !haveBrowser() && s_viewers.isEmpty() ) {
	if ( fileWidget )
	    saveSettings();
	
	::exit(0);
    }

    else if ( haveBrowser() ) {
	setActiveWindow();
	fileWidget->setFocus();
    }
}


void KuickShow::slotHighlighted( const KFileItem *fi )
{
    QString size;
    //size.sprintf( " %.1f kb", (float) fi->size()/1024 );
    size = i18n("%1 kb").arg(KGlobal::locale()->formatNumber((float)fi->size()/1024, 1));
    statusBar()->changeItem( size, SIZE_ITEM );
    statusBar()->changeItem( fi->url().prettyURL(), URL_ITEM );

    bool image = FileWidget::isImage( fi );
    fileWidget->actionCollection()->action("kuick_print")->setEnabled( image );
}

void KuickShow::dirSelected( const KURL& url )
{
    if ( url.isLocalFile() )
	setCaption( url.path() );
    else
	setCaption( url.prettyURL() );
	
    statusBar()->changeItem( url.prettyURL(), URL_ITEM );
}

void KuickShow::slotSelected( const KFileItem *item )
{
    showImage( item, !newWindowAction->isChecked() );
}

// downloads item if necessary
void KuickShow::showFileItem( ImageWindow */*view*/,
			      const KFileItem */*item*/ )
{

}

void KuickShow::showImage( const KFileItem *fi, bool newWin )
{
    bool newWindow = !viewer || newWin;

    if ( FileWidget::isImage( fi ) ) {
	if ( newWindow ) {
	    viewer = new ImageWindow( kdata->idata, id, 0L, "image window" );
	    s_viewers.append( viewer );

	    connect( viewer, SIGNAL( destroyed() ), SLOT( viewerDeleted() ));
	    connect( viewer, SIGNAL( sigFocusWindow( ImageWindow *) ),
		     this, SLOT( slotSetActiveViewer( ImageWindow * ) ));
	    connect( viewer, SIGNAL( sigBadImage(const QString& ) ),
		     this, SLOT( messageCantLoadImage(const QString &) ));
            connect( viewer, SIGNAL( requestImage( ImageWindow *, int )), 
                     this, SLOT( slotAdvanceImage( ImageWindow *, int )));
	    if ( s_viewers.count() == 1 ) {
		// we have to move to 0x0 before showing _and_
		// after showing, otherwise we get some bogus geometry()
		viewer->move( Kuick::workArea().topLeft() );
	    }

	    viewer->setPopupMenu();
	    viewer->installEventFilter( this );
	}

	QString filename = fi->url().path();

	if ( !viewer->showNextImage( filename ) )
	    viewer->close( true ); // couldn't load image, close window
	else {
	    if ( newWindow ) {
		if ( kdata->fullScreen )
		    viewer->setFullscreen( true );

		viewer->show();
		
		if ( !kdata->fullScreen && s_viewers.count() == 1 ) {
		    // the WM might have moved us after showing -> strike back!
		    // move the first image to 0x0 workarea coord
		    viewer->move( Kuick::workArea().topLeft() );
		}
	    }

  	    if ( kdata->preloadImage && fileWidget ) {
  		KFileItem *item = 0L;                 // don't move cursor
  		item = fileWidget->getItem( FileWidget::Next, true );
  		if ( item )
  		    viewer->cacheImage( item->url().path() ); // FIXME
  	    }
	} // viewer created successfully
    } // isImage
}

void KuickShow::startSlideShow()
{
    KFileItem *item = fileWidget->gotoFirstImage();
    if ( item ) {
	fileWidget->actionCollection()->action("kuick_slideshow")->setEnabled( false );
	showImage( item, !kdata->showInOneWindow );
	QTimer::singleShot( kdata->slideDelay, this, SLOT( nextSlide() ) );
    }
}

void KuickShow::nextSlide()
{
    if ( !viewer ) {
	fileWidget->actionCollection()->action("kuick_slideshow")->setEnabled( true );
	return;
    }

    KFileItem *item = fileWidget->getNext( true );
    if ( !item ) {
	viewer->close( true );
	fileWidget->actionCollection()->action("kuick_slideshow")->setEnabled( true );
	return;
    }

    viewer->showNextImage( item->url().path() );
    QTimer::singleShot( kdata->slideDelay, this, SLOT( nextSlide() ) );
}


// prints the selected files in the filebrowser
// FIXME: maybe print with QPainter?
void KuickShow::slotPrint()
{
    const KFileItemList *items = fileWidget->selectedItems();
    if ( !items )
	return;

    KFileItemListIterator it( *items );

    // don't show the image, just print
    ImageWindow *iw = new ImageWindow( 0, id, this, "printing image" );
    while ( it.current() ) {
	if ( iw->loadImage( it.current()->url().path() ) )
	    iw->printImage();
	++it;
    }

    iw->close( true );
}

// ### fallback for kdelibs older than 2.2alpha2
// deletes the selected files in the filebrowser
void KuickShow::slotDelete()
{
    KFileItem *item = fileWidget->getCurrentItem( false );
    if ( item )
        KuickIO::self( this )->deleteFile( item->url(), false );
}

// ### remove this and the action somewhen, KDirOperator has it since 2.2 Beta1
void KuickShow::slotShowProperties()
{
    (void) new KPropertiesDialog( fileWidget->getCurrentItem( false ), this );
}

void KuickShow::slotShowInOtherWindow()
{
    showImage( fileWidget->getCurrentItem( false ), true );
}

void KuickShow::slotShowInSameWindow()
{
    showImage( fileWidget->getCurrentItem( false ), false );
}

void KuickShow::dropEvent( QDropEvent *e )
{
    KURL dir; // in case we get a directory dropped
    KURL::List urls;
    KURLDrag::decode( e, urls );
    bool hasRemote = false;

    KURL::List::Iterator it = urls.begin();
    while ( it != urls.end() ) {
	KURL u = *it;
	if ( u.isLocalFile() ) {
	    if ( !u.fileName().isEmpty() )
		dir = u;
	    else {
		KFileItem item( -1, -1, u, false );
		showImage( &item, true );
	    }
	}
	else
	    hasRemote = true;
    }


    if ( hasRemote ) {
	QString tmp( i18n("You can only drop local files "
		"onto the image viewer!"));
	KMessageBox::sorry( this, tmp, i18n("KuickShow Drop Error") );
    }
}

// try to init the WM border as it is 0,0 when the window is not shown yet.
void KuickShow::show()
{
    KMainWindow::show();
    (void) Kuick::frameSize( winId() );
}

void KuickShow::slotAdvanceImage( ImageWindow *view, int steps )
{
    KFileItem *item      = 0L; // to be shown
    KFileItem *item_next = 0L; // to be cached
    
    // the viewer might not be available yet. Factor this out somewhen.
    if ( !fileWidget ) {
        if ( m_delayedRepeatItem )
            return;

        m_delayedRepeatItem = new DelayedRepeatEvent( view, steps );

        KURL start;
        QFileInfo fi( view->filename() );
        start.setPath( fi.dirPath( true ) );
        initGUI( start );
        fileWidget->setInitialItem( fi.fileName() );

        connect( fileWidget, SIGNAL( finished() ),
                 SLOT( slotReplayAdvance() ));
        return;
    }

    if ( steps > 0 ) {
        for ( int i = 0; i < steps; i++ )
            item = fileWidget->getNext( true );
        item_next = fileWidget->getNext( false );
    }
    
    else if ( steps < 0 ) {
        for ( int i = steps; i < 0; i++ )
            item = fileWidget->getPrevious( true );
        item_next = fileWidget->getPrevious( false );
    }

    if ( FileWidget::isImage( item ) ) {
        view->showNextImage( item->url().path() ); // ###
		
        if ( kdata->preloadImage ) // preload next image
            if ( FileWidget::isImage( item_next ) )
                view->cacheImage( item_next->url().path() ); // ###
    }
}

bool KuickShow::eventFilter( QObject *o, QEvent *e )
{
    if ( m_delayedRepeatItem ) // we probably need to install an eventFilter over
	return true;    // kapp, to make it really safe

    bool ret = false;
    QKeyEvent *k = 0L;
    if ( e->type() == QEvent::KeyPress )
	k = static_cast<QKeyEvent *>( e );

    if ( k ) {
	if ( KStdAccel::isEqual( k, KStdAccel::quit() )) {
	    saveSettings();
	    exit(0);
	}
	
	else if ( KStdAccel::isEqual( k, KStdAccel::help() ) ) {
	    appHelpActivated();
	    return true;
	}
    }


    ImageWindow *window = dynamic_cast<ImageWindow*>( o );
    if ( window ) {
// 	KCursor::autoHideEventFilter( o, e );
	
	viewer = window;
	QString img;
	KFileItem *item = 0L;      // the image to be shown
	KFileItem *item_next = 0L; // the image to be cached

	if ( k ) { // keypress
	    ret = true;
	    int key = k->key();
	
	    // Key_Shift shouldn't load the browser in nobrowser mode, it
	    // is used for zooming in the imagewindow
	    if ( !fileWidget && key != Key_Escape && key != Key_Shift ) {
		KURL start;
		QFileInfo fi( viewer->filename() );
		start.setPath( fi.dirPath( true ) );
		initGUI( start );

		// the fileBrowser will list the start-directory asynchronously
		// so we can't immediately continue. There is no current-item
		// and no next-item (actually no item at all). So we tell the
		// browser the initial current-item and wait for it to tell us
		// when it's ready. Then we will replay this KeyEvent.
		fileWidget->setInitialItem( fi.fileName() );
		connect( fileWidget, SIGNAL( finished() ),
			 SLOT( slotReplayEvent() ));
		delayedRepeatEvent( viewer, k );
		return true;
	    }

 	    // FIXME: make all this stuff via KStdAccel and KAccel ->slots

	    switch( key ) {
	    case Key_Home: {
		item = fileWidget->gotoFirstImage();
		item_next = fileWidget->getNext( false );
		break;
	    }

	    case Key_End: {
		item = fileWidget->gotoLastImage();
		item_next = fileWidget->getPrevious( false );
		break;
	    }

	    case Key_Enter:
		item = fileWidget->getCurrentItem( true );
		break;

	    case Key_Delete: {
		KFileItem *cur = fileWidget->getCurrentItem( false );
		item = fileWidget->getNext( false ); // don't move
		if ( !item )
		    item = fileWidget->getPrevious( false );
		
		if ( KuickIO::self( viewer )->deleteFile( viewer->filename(),
						    k->state() & ShiftButton) )
		    fileWidget->setCurrentItem( item );
		else
		    item = cur; // restore old current item
		break;
	    }

	    case Key_Space: {
		if ( haveBrowser() )
		    hide();
		else {
		    if ( viewer->isFullscreen() )
			viewer->setFullscreen( false );
		    show();
		}

		return true; // don't pass keyEvent
	    }

	    default:
		ret = false;
	    }


	    if ( FileWidget::isImage( item ) ) {
		viewer->showNextImage( item->url().path() ); // ###
		
		if ( kdata->preloadImage ) // preload next image
		    if ( FileWidget::isImage( item_next ) )
			viewer->cacheImage( item_next->url().path() ); // ###

		ret = true; // don't pass keyEvent
	    }
	} // keyPressEvent on ImageWindow

    } // isA ImageWindow


    if ( ret )
	return true;

    return KMainWindow::eventFilter( o, e );
}


void KuickShow::configuration()
{
    if ( !m_accel ) {
	KURL start;
	start.setPath( QDir::homeDirPath() );
	initGUI( QDir::homeDirPath() );
    }

    dialog = new KuickConfigDialog( m_accel, 0L, "dialog", false );
    dialog->resize( 540, 510 );
    dialog->setIcon( kapp->miniIcon() );

    connect( dialog, SIGNAL( okClicked() ),
	     this, SLOT( slotConfigApplied() ) );
    connect( dialog, SIGNAL( applyClicked() ),
	     this, SLOT( slotConfigApplied() ) );
    connect( dialog, SIGNAL( finished() ),
	     this, SLOT( slotConfigClosed() ) );

    fileWidget->actionCollection()->action( "kuick_configure" )->setEnabled( false );
    dialog->show();
}


void KuickShow::slotConfigApplied()
{
    dialog->applyConfig();

    initImlib();
    kdata->save();

    ImageWindow *viewer;
    QValueListIterator<ImageWindow*> it = s_viewers.begin();
    while ( it != s_viewers.end() ) {
        viewer = *it;
	viewer->updateAccel();
        viewer->setBackgroundColor( kdata->backgroundColor );
	++it;
    }

    if ( kdata->fileFilter != fileWidget->nameFilter() )	{
	// reload directory contents if filefilter changed
	fileWidget->setNameFilter( kdata->fileFilter );
	fileWidget->rereadDir();
    }
}


void KuickShow::slotConfigClosed()
{
    dialog->delayedDestruct();
    fileWidget->actionCollection()->action( "kuick_configure" )->setEnabled( true );
}

void KuickShow::about()
{
    AboutWidget *aboutWidget = new AboutWidget( 0L, "about" );
    aboutWidget->adjustSize();
    QWidget *d = QApplication::desktop();
    QPoint p( d->width()/2 - aboutWidget->width()/2,
	      d->height()/2 - aboutWidget->height()/2 );
    aboutWidget->move( p );
    aboutWidget->show();
}


// ------ sessionmanagement - load / save current directory -----
void KuickShow::readProperties( KConfig *kc )
{
    assert( fileWidget ); // from SM, we should always have initGUI on startup
    QString dir = kc->readEntry( "CurrentDirectory" );
    if ( !dir.isEmpty() ) {
	fileWidget->setURL( dir, true );
	fileWidget->clearHistory();
    }

    QStringList images = kc->readListEntry( "Images shown" );
    QStringList::Iterator it;
    for ( it = images.begin(); it != images.end(); ++it ) {
	KFileItem item( -1, -1, *it, false );
	if ( item.isReadable() )
	    showImage( &item, true );
    }

    if ( !s_viewers.isEmpty() ) {
	bool visible = kc->readBoolEntry( "Browser visible", true );
	if ( !visible )
	    hide();
    }
}

void KuickShow::saveProperties( KConfig *kc )
{
    kc->writeEntry( "CurrentDirectory", fileWidget->url().url() );
    kc->writeEntry( "Browser visible", fileWidget->isVisible() );

    QStringList urls;
    QValueListIterator<ImageWindow*> it;
    for ( it = s_viewers.begin(); it != s_viewers.end(); ++it )
	urls.append( (*it)->filename() );
	
    kc->writeEntry( "Images shown", urls );
}

// --------------------------------------------------------------

void KuickShow::saveSettings()
{
    KConfig *kc = KGlobal::config();

    kc->setGroup("SessionSettings");
    kc->writeEntry( "OpenImagesInActiveWindow", newWindowAction->isChecked() );
    kc->writeEntry( "CurrentDirectory", fileWidget->url().url() );

    if ( fileWidget )
	fileWidget->writeConfig( kc, "Filebrowser" );

    kc->sync();
}


void KuickShow::messageCantLoadImage( const QString& filename )
{
    viewer->clearFocus();
    QString tmp = i18n("Sorry, I can't load the image %1.\n"
	    "Perhaps the file format is unsupported or "
                      "your Imlib is not installed properly.").arg(filename);
    KMessageBox::sorry( 0L, tmp, i18n("Image Error") );
}

void KuickShow::initImlib()
{
    ImData *idata = kdata->idata;
    ImlibInitParams par;
    initImlibParams( idata, &par );

    id = Imlib_init_with_params( x11Display(), &par );
    if ( !id ) {
	initImlibParams( idata, &par );

	qWarning("*** KuickShow: Whoops, can't initialize imlib, trying my own palettefile now.");
	QString paletteFile = locate( "data", "kuickshow/im_palette.pal" );
	// FIXME - does the qstrdup() cure the segfault in imlib eventually?
	char *file = qstrdup( paletteFile.local8Bit() );
	par.palettefile = file;
	par.flags |= PARAMS_PALETTEFILE;

	qWarning("Palettefile: %s", par.palettefile );

	id = Imlib_init_with_params( x11Display(), &par );

	if ( !id ) {
	    QString tmp = i18n("Can't initialize \"Imlib\".\n"
		    "Start kuickshow on the command line "
		    "and look for error messages.\n"
		    "I will quit now.");
	    KMessageBox::error( this, tmp, i18n("Fatal Imlib error") );

	    exit(1);
	}
    }
}


void KuickShow::initImlibParams( ImData *idata, ImlibInitParams *par )
{
    par->flags = ( PARAMS_REMAP |
		   PARAMS_FASTRENDER | PARAMS_HIQUALITY | PARAMS_DITHER |
		   PARAMS_IMAGECACHESIZE | PARAMS_PIXMAPCACHESIZE );

    par->paletteoverride = idata->ownPalette  ? 1 : 0;
    par->remap           = idata->fastRemap   ? 1 : 0;
    par->fastrender      = idata->fastRender  ? 1 : 0;
    par->hiquality       = idata->dither16bit ? 1 : 0;
    par->dither          = idata->dither8bit  ? 1 : 0;
    uint maxcache        = idata->maxCache;

    // 0 == no cache
    par->imagecachesize  = maxcache * 1024;
    par->pixmapcachesize = maxcache * 1024;
}

bool KuickShow::haveBrowser() const
{
    return fileWidget && fileWidget->isVisible();
}

void KuickShow::delayedRepeatEvent( ImageWindow *w, QKeyEvent *e )
{
    m_delayedRepeatItem = new DelayedRepeatEvent( w, new QKeyEvent( *e ) );
}

void KuickShow::slotReplayEvent()
{
    disconnect( fileWidget, SIGNAL( finished() ),
		this, SLOT( slotReplayEvent() ));

    DelayedRepeatEvent *e = m_delayedRepeatItem;
    m_delayedRepeatItem = 0L; // otherwise, eventFilter aborts

    eventFilter( e->viewer, e->event );
    delete e;
}

void KuickShow::slotReplayAdvance()
{
    if ( !m_delayedRepeatItem )
        return;

    disconnect( fileWidget, SIGNAL( finished() ),
                this, SLOT( slotReplayAdvance() ));

    DelayedRepeatEvent *e = m_delayedRepeatItem;
    m_delayedRepeatItem = 0L; // otherwise, eventFilter aborts

    slotAdvanceImage( e->viewer, e->steps );
    delete e;
}

#include "kuickshow.moc"
