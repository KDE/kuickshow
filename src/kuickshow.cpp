/****************************************************************************
** $Id$
**
** KuickShow - a fast and comfortable image viewer based on Rasterman's Imlib
**
** Created : 98
**
** Copyright (C) 1998-2002 by Carsten Pfeiffer.  All rights reserved.
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

#include <kaboutdata.h>
#include <kaccel.h>
#include <kaction.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kcursor.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kpropertiesdialog.h>
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
      m_slideshowCycle( 1 ),
      fileWidget( 0L ),
      dialog( 0L ),
      id( 0L ),
      m_viewer( 0L ),
      oneWindowAction( 0L ),
      m_accel( 0L ),
      m_delayedRepeatItem( 0L )
{
  kdata = new KuickData;
  kdata->load();

  initImlib();
  resize( 400, 500 );

  m_slideTimer = new QTimer( this );
  connect( m_slideTimer, SIGNAL( timeout() ), SLOT( nextSlide() ));


  KConfig *kc = KGlobal::config();

  bool isDir = false; // true if we get a directory on the commandline

  // parse commandline options
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		
  // files to display
  // either a directory to display, an absolute path, a relative path, or a URL
  KURL startDir;
  startDir.setPath( QDir::currentDirPath() + '/' );
  for ( int i = 0; i < args->count(); i++ ) {
      KURL url = args->url( i );
      KFileItem item( KFileItem::Unknown, KFileItem::Unknown, url, false );

      // for remote URLs, we don't know if it's a file or directory, but
      // FileWidget::isImage() should correct in most cases.
      // For non-local non-images, we just assume directory.
      if ( FileWidget::isImage( &item ) )
	  showImage( &item, true, false, true ); // show in new window, not fullscreen-forced and move to 0,0
// 	  showImage( &item, true, false, false ); // show in new window, not fullscreen-forced and not moving to 0,0
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

    if ( m_viewer )
	m_viewer->close( true );

    free( id );
    kapp->quit();

    delete kdata;
}


void KuickShow::initGUI( const KURL& startDir )
{
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
    KAction *open = KStdAction::open( this, SLOT( slotOpenURL() ),
                                      coll, "openURL" );

    KAction *print = KStdAction::print( this, SLOT( slotPrint() ),
					coll, "kuick_print" );
    print->setText( i18n("Print Image...") );

    KAction *configure = new KAction( i18n("Configure %1...").arg( KGlobal::instance()->aboutData()->programName() ), "configure",
                                      KShortcut(),
				      this, SLOT( configuration() ),
				      coll, "kuick_configure" );
    KAction *slide = new KAction( i18n("Start Slideshow" ), "ksslide",
                                  KShortcut( Key_F2 ),
				  this, SLOT( startSlideShow() ),
				  coll, "kuick_slideshow" );
    KAction *about = new KAction( i18n( "About KuickShow" ), "about",
                                  KShortcut(),
				  this, SLOT( about() ), coll, "about" );

    oneWindowAction = new KToggleAction( i18n("Open only one Image Window"),
					 "window_new",
                                         KShortcut( CTRL+Key_N ), coll,
					 "kuick_one window" );

    m_toggleBrowserAction = new KToggleAction( i18n("Show/Hide the File Browser"), KShortcut( Key_Space ), coll, "toggleBrowser" );
    connect( m_toggleBrowserAction, SIGNAL( toggled( bool ) ),
             SLOT( toggleBrowser() ));

    KAction *showInOther = new KAction( i18n("Show Image"), KShortcut(),
                                        this, SLOT( slotShowInOtherWindow() ),
                                        coll, "kuick_showInOtherWindow" );
    KAction *showInSame = new KAction( i18n("Show Image in Active Window"),
                                       KShortcut(),
                                       this, SLOT( slotShowInSameWindow() ),
                                       coll, "kuick_showInSameWindow" );

    KAction *quit = KStdAction::quit( this, SLOT(slotQuit()), coll, "quit");

    // remove QString::null parameter -- ellis
    coll->readShortcutSettings( QString::null );
    m_accel = coll->accel();

    // menubar
    KMenuBar *mBar = menuBar();
    QPopupMenu *fileMenu = new QPopupMenu( mBar, "file" );
    open->plug( fileMenu );
    showInOther->plug( fileMenu );
    showInSame->plug( fileMenu );
    fileMenu->insertSeparator();
    slide->plug( fileMenu );
    print->plug( fileMenu );
    fileMenu->insertSeparator();
    quit->plug( fileMenu );

    QPopupMenu *editMenu = new QPopupMenu( mBar, "edit" );
    coll->action("mkdir")->plug( editMenu );
    coll->action("delete")->plug( editMenu );
    editMenu->insertSeparator();
    coll->action("properties")->plug( editMenu );


    // remove the Sorting submenu (and the separator below)
    // from the main contextmenu
    KActionMenu *sortingMenu = static_cast<KActionMenu*>( coll->action("sorting menu"));
    KActionMenu *mainActionMenu = static_cast<KActionMenu*>( coll->action("popupMenu"));
    QPopupMenu *mainPopup = mainActionMenu->popupMenu();
    int sortingIndex = mainPopup->indexOf( sortingMenu->itemId( 0 ) );
    int separatorId = mainPopup->idAt( sortingIndex + 1 );
    QMenuItem *separatorItem = mainPopup->findItem( separatorId );
    if ( separatorItem && separatorItem->isSeparator() )
        mainPopup->removeItem( separatorId );
    mainActionMenu->remove( sortingMenu );

    // add the sorting menu and a separator into the View menu
    KActionMenu *viewActionMenu = static_cast<KActionMenu*>( coll->action("view menu"));
    viewActionMenu->popupMenu()->insertSeparator( 0 );
    sortingMenu->plug( viewActionMenu->popupMenu(), 0 ); // on top of the menu


    QPopupMenu *settingsMenu = new QPopupMenu( mBar, "settings" );
    configure->plug( settingsMenu );

    mBar->insertItem( i18n("&File"), fileMenu );
    mBar->insertItem( i18n("&Edit"), editMenu );
    viewActionMenu->plug( mBar );
    mBar->insertItem( i18n("&Settings"), settingsMenu );

    // toolbar
    KToolBar *tBar = toolBar();
    tBar->setText( i18n( "Main Toolbar" ) );

    coll->action("up")->plug( tBar );
    coll->action("back")->plug( tBar );
    coll->action("forward")->plug( tBar );
    coll->action("home")->plug( tBar );
    coll->action("reload")->plug( tBar );

    tBar->insertSeparator();

    coll->action( "short view" )->plug( tBar );
    coll->action( "detailed view" )->plug( tBar );
    coll->action( "preview")->plug( tBar );

    tBar->insertSeparator();
    configure->plug( tBar );
    slide->plug( tBar );
    tBar->insertSeparator();
    oneWindowAction->plug( tBar );
    print->plug( tBar );
    tBar->insertSeparator();
    about->plug( tBar );

    QPopupMenu *help = helpMenu( QString::null, false );
    mBar->insertItem( i18n("&Help"), help );


    KStatusBar* sBar = statusBar();
    sBar->insertItem( "                          ", SIZE_ITEM, 2 );
    sBar->insertItem( "           ", URL_ITEM, 10 );
    sBar->setItemAlignment(URL_ITEM, QLabel::AlignVCenter | QLabel::AlignLeft);

    fileWidget->setFocus();

    KConfig *kc = KGlobal::config();
    kc->setGroup("SessionSettings");
    bool oneWindow = kc->readBoolEntry("OpenImagesInActiveWindow", true );
    oneWindowAction->setChecked( oneWindow );

    tBar->show();

    fileWidget->initActions();
    fileWidget->clearHistory();
    dirSelected( fileWidget->url() );

    setCentralWidget( fileWidget );
    setAutoSaveSettings();

    coll->action( "reload" )->setShortcut( KStdAccel::reload() );
    coll->action( "short view" )->setShortcut(Key_F6);
    coll->action( "detailed view" )->setShortcut(Key_F7);
    coll->action( "show hidden" )->setShortcut(Key_F8);
    coll->action( "mkdir" )->setShortcut(Key_F10);
    coll->action( "preview" )->setShortcut(Key_F11);
    coll->action( "separate dirs" )->setShortcut(Key_F12);
}


void KuickShow::viewerDeleted()
{
    ImageWindow *viewer = (ImageWindow*) sender();
    s_viewers.remove( viewer );
    if ( viewer == m_viewer )
        m_viewer = 0L;

    if ( !haveBrowser() && s_viewers.isEmpty() ) {
	if ( fileWidget )
	    saveSettings();

	::exit(0);
    }

    else if ( haveBrowser() ) {
	setActiveWindow();
        // This setFocus() call causes problems in the combiview (always the
        // directory view on the left gets the focus, which is not desired)
        // fileWidget->setFocus();
    }

    if ( fileWidget )
        // maybe a slideshow was stopped --> enable the action again
        fileWidget->actionCollection()->action("kuick_slideshow")->setEnabled( true );

    m_slideTimer->stop();
}


void KuickShow::slotHighlighted( const KFileItem *fi )
{
    QString size;
    size = i18n("%1 kb").arg(KGlobal::locale()->formatNumber((float)fi->size()/1024, 1));
    statusBar()->changeItem( size, SIZE_ITEM );
    statusBar()->changeItem( fi->url().prettyURL(), URL_ITEM );

    bool image = FileWidget::isImage( fi );
    fileWidget->actionCollection()->action("kuick_print")->setEnabled( image );
    fileWidget->actionCollection()->action("kuick_showInSameWindow")->setEnabled( image );
    fileWidget->actionCollection()->action("kuick_showInOtherWindow")->setEnabled( image );
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
    showImage( item, !oneWindowAction->isChecked() );
}

// downloads item if necessary
void KuickShow::showFileItem( ImageWindow */*view*/,
			      const KFileItem */*item*/ )
{

}

void KuickShow::showImage( const KFileItem *fi,
                           bool newWindow, bool fullscreen, bool moveToTopLeft )
{
    newWindow  |= !m_viewer;
    fullscreen |= (newWindow && kdata->fullScreen);

    if ( FileWidget::isImage( fi ) ) {
	if ( newWindow ) {
	    m_viewer = new ImageWindow( kdata->idata, id, 0L, "image window" );
	    s_viewers.append( m_viewer );

	    connect( m_viewer, SIGNAL( destroyed() ), SLOT( viewerDeleted() ));
	    connect( m_viewer, SIGNAL( sigFocusWindow( ImageWindow *) ),
		     this, SLOT( slotSetActiveViewer( ImageWindow * ) ));
	    connect( m_viewer, SIGNAL( sigBadImage(const QString& ) ),
		     this, SLOT( messageCantLoadImage(const QString &) ));
            connect( m_viewer, SIGNAL( requestImage( ImageWindow *, int )),
                     this, SLOT( slotAdvanceImage( ImageWindow *, int )));
	    if ( s_viewers.count() == 1 && moveToTopLeft ) {
		// we have to move to 0x0 before showing _and_
		// after showing, otherwise we get some bogus geometry()
		m_viewer->move( Kuick::workArea().topLeft() );
	    }

	    m_viewer->installEventFilter( this );
	}

        // for some strange reason, m_viewer sometimes changes during the
        // next few lines of code, so as a workaround, we use safeViewer here.
        // This happens when calling KuickShow with two or more remote-url
        // arguments on the commandline, where the first one is loaded properly
        // and the second isn't (e.g. because it is a pdf or something else,
        // Imlib can't load).
        ImageWindow *safeViewer = m_viewer;

	QString filename;
	KIO::NetAccess::download(fi->url(), filename);

	if ( !safeViewer->showNextImage( filename ) ) {
            m_viewer = safeViewer;
	    safeViewer->close( true ); // couldn't load image, close window
        }
	else {
            safeViewer->setFullscreen( fullscreen );

	    if ( newWindow ) {
		safeViewer->show();
		
		if ( !fullscreen && s_viewers.count() == 1 && moveToTopLeft ) {
		    // the WM might have moved us after showing -> strike back!
		    // move the first image to 0x0 workarea coord
		    safeViewer->move( Kuick::workArea().topLeft() );
		}
	    }

  	    if ( kdata->preloadImage && fileWidget ) {
  		KFileItem *item = 0L;                 // don't move cursor
  		item = fileWidget->getItem( FileWidget::Next, true );
  		if ( item )
  		    safeViewer->cacheImage( item->url().path() ); // FIXME
  	    }

            m_viewer = safeViewer;
	} // m_viewer created successfully
    } // isImage
}

void KuickShow::startSlideShow()
{
    KFileItem *item = fileWidget->gotoFirstImage();

    if ( item ) {
        m_slideshowCycle = 1;
	fileWidget->actionCollection()->action("kuick_slideshow")->setEnabled( false );
	showImage( item, !oneWindowAction->isChecked(),
                   kdata->slideshowFullscreen );
        m_slideTimer->start( kdata->slideDelay );
    }
}

void KuickShow::nextSlide()
{
    if ( !m_viewer ) {
        m_slideshowCycle = 1;
	fileWidget->actionCollection()->action("kuick_slideshow")->setEnabled( true );
	return;
    }

    KFileItem *item = fileWidget->getNext( true );
    if ( !item ) { // last image
        if ( m_slideshowCycle < kdata->slideshowCycles
           || kdata->slideshowCycles == 0 ) {
            item = fileWidget->gotoFirstImage();
            if ( item ) {
                nextSlide( item );
                m_slideshowCycle++;
                return;
            }
        }

	m_viewer->close( true );
	fileWidget->actionCollection()->action("kuick_slideshow")->setEnabled( true );
	return;
    }

    nextSlide( item );
}

void KuickShow::nextSlide( KFileItem *item )
{
    m_viewer->showNextImage( item->url().path() );
    m_slideTimer->start( kdata->slideDelay );
}


// prints the selected files in the filebrowser
void KuickShow::slotPrint()
{
    const KFileItemList *items = fileWidget->selectedItems();
    if ( !items )
	return;

    KFileItemListIterator it( *items );

    // don't show the image, just print
    ImageWindow *iw = new ImageWindow( 0, id, this, "printing image" );
    KFileItem *item;
    while ( (item = it.current()) ) {
	if (FileWidget::isImage( item ) && iw->loadImage( item->url().path()))
	    iw->printImage();
	++it;
    }

    iw->close( true );
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
		KFileItem item( KFileItem::Unknown, KFileItem::Unknown, u, false );
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

        // see eventFilter() for explanation and similar code
        if ( fileWidget->dirLister()->isFinished() &&
             fileWidget->dirLister()->rootItem() )
        {
            fileWidget->setCurrentItem( fi.fileName() );
            QTimer::singleShot( 0, this, SLOT( slotReplayAdvance()));
        }
        else
        {
            fileWidget->setInitialItem( fi.fileName() );
            connect( fileWidget, SIGNAL( finished() ),
                     SLOT( slotReplayAdvance() ));
        }

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
        if (m_slideTimer->isActive())
	  m_slideTimer->start( kdata->slideDelay );

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
        if ( KStdAccel::quit().contains( KKey( k ) ) ) {
	    saveSettings();
            deleteAllViewers();
	    ::exit(0);
        }
        else if ( KStdAccel::help().contains( KKey( k ) ) ) {
	    appHelpActivated();
	    return true;
	}
    }


    ImageWindow *window = dynamic_cast<ImageWindow*>( o );
    if ( window ) {
//  	KCursor::autoHideEventFilter( o, e );

	m_viewer = window;
	QString img;
	KFileItem *item = 0L;      // the image to be shown
	KFileItem *item_next = 0L; // the image to be cached

	if ( k ) { // keypress
	    ret = true;
	    int key = k->key();

	    // Key_Shift shouldn't load the browser in nobrowser mode, it
	    // is used for zooming in the imagewindow
	    if ( !fileWidget )
            {
                if ( key != Key_Escape && key != Key_Shift )
                {
                    KURL start;
                    QFileInfo fi( m_viewer->filename() );
                    start.setPath( fi.dirPath( true ) );
                    initGUI( start );

                    // the fileBrowser will list the start-directory
                    // asynchronously so we can't immediately continue. There
                    // is no current-item and no next-item (actually no item
                    // at all). So we tell the browser the initial
                    // current-item and wait for it to tell us when it's ready.
                    // Then we will replay this KeyEvent.
                    delayedRepeatEvent( m_viewer, k );

                    // OK, once again, we have a problem with the now async and
                    // sync KDirLister :( If the startDir is already cached by
                    // KDirLister, we won't ever get that finished() signal
                    // because it is emitted before we can connect(). So if
                    // our dirlister has a rootFileItem, we assume the
                    // directory is read already and simply call
                    // slotReplayEvent() without the need for the finished()
                    // signal.

                    // see slotAdvanceImage() for similar code
                    if ( fileWidget->dirLister()->isFinished() &&
                         fileWidget->dirLister()->rootItem() )
                    {
                        fileWidget->setCurrentItem( fi.fileName() );
                        QTimer::singleShot( 0, this, SLOT( slotReplayEvent()));
                    }
                    else
                    {
                        fileWidget->setInitialItem( fi.fileName() );
                        connect( fileWidget, SIGNAL( finished() ),
                                 SLOT( slotReplayEvent() ));
                    }

                    return true;
                }

                return KMainWindow::eventFilter( o, e );
	    }

            // we definitely have a fileWidget here!

            KKey kkey( k );
            if ( key == Key_Home || KStdAccel::home().contains( kkey ) )
            {
		item = fileWidget->gotoFirstImage();
		item_next = fileWidget->getNext( false );
	    }

            else if ( key == Key_End || KStdAccel::end().contains( kkey ) )
            {
		item = fileWidget->gotoLastImage();
		item_next = fileWidget->getPrevious( false );
	    }

            else if ( fileWidget->actionCollection()->action("delete")->shortcut().contains( key ))
            {
// 		KFileItem *cur = fileWidget->getCurrentItem( false );
 		(void) fileWidget->getCurrentItem( false );
		item = fileWidget->getNext( false ); // don't move
		if ( !item )
		    item = fileWidget->getPrevious( false );
                KFileItem it( KFileItem::Unknown, KFileItem::Unknown,
                              m_viewer->url() );
                KFileItemList list;
                list.append( &it );
                if ( fileWidget->del(list, window,
                                     (k->state() & ShiftButton) == 0) == 0L )
                    return true; // aborted deletion

                // ### check failure asynchronously and restore old item?
                fileWidget->setCurrentItem( item );
	    }

	    else if ( m_toggleBrowserAction->shortcut().contains( key ) )
            {
                toggleBrowser();
		return true; // don't pass keyEvent
	    }

            else
		ret = false;


	    if ( FileWidget::isImage( item ) ) {
		m_viewer->showNextImage( item->url().path() ); // ###
		
		if ( kdata->preloadImage ) // preload next image
		    if ( FileWidget::isImage( item_next ) )
			m_viewer->cacheImage( item_next->url().path() ); // ###

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

    dialog = new KuickConfigDialog( fileWidget->actionCollection(), 0L,
                                    "dialog", false );
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
	viewer->updateActions();
	++it;
    }

    fileWidget->reloadConfiguration();
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
    KDialog::centerOnScreen( aboutWidget );
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
	KFileItem item( KFileItem::Unknown, KFileItem::Unknown, *it, false );
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
    kc->writeEntry( "OpenImagesInActiveWindow", oneWindowAction->isChecked() );
    kc->writeEntry( "CurrentDirectory", fileWidget->url().url() );

    if ( fileWidget )
	fileWidget->writeConfig( kc, "Filebrowser" );

    kc->sync();
}


void KuickShow::messageCantLoadImage( const QString& filename )
{
    m_viewer->clearFocus();
    QString tmp = i18n("Unable to load the image %1.\n"
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

	    ::exit(1);
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

    // ### WORKAROUND for QIconView bug in Qt <= 3.0.3 at least
    if ( fileWidget && fileWidget->view() ) {
        QWidget *widget = fileWidget->view()->widget();
        if ( widget->inherits( "QIconView" ) || widget->child(0, "QIconView" ) ){
            fileWidget->setSorting( fileWidget->sorting() );
        }
    }
    // --------------------------------------------------------------
}

void KuickShow::slotReplayAdvance()
{
    if ( !m_delayedRepeatItem )
        return;

    disconnect( fileWidget, SIGNAL( finished() ),
                this, SLOT( slotReplayAdvance() ));

    DelayedRepeatEvent *e = m_delayedRepeatItem;
    m_delayedRepeatItem = 0L; // otherwise, eventFilter aborts

    // ### WORKAROUND for QIconView bug in Qt <= 3.0.3 at least
    // Sigh. According to qt-bugs, they won't fix this bug ever. So you can't
    // rely on sorting to be correct before the QIconView has been show()n.
    if ( fileWidget && fileWidget->view() ) {
        QWidget *widget = fileWidget->view()->widget();
        if ( widget->inherits( "QIconView" ) || widget->child(0, "QIconView" ) ){
            fileWidget->setSorting( fileWidget->sorting() );
        }
    }
    // --------------------------------------------------------------

    slotAdvanceImage( e->viewer, e->steps );
    delete e;
}

void KuickShow::toggleBrowser()
{
    if ( !haveBrowser() ) {
        if ( m_viewer && m_viewer->isFullscreen() )
            m_viewer->setFullscreen( false );
        fileWidget->resize( size() ); // ### somehow fileWidget isn't resized!?
        show();
        raise();
        KWin::setActiveWindow( winId() ); // ### this should not be necessary
//         setFocus();
    }
    else if ( !s_viewers.isEmpty() )
        hide();
}

void KuickShow::slotOpenURL()
{
    KFileDialog dlg(QString::null, kdata->fileFilter, this, "filedialog", true);
    dlg.setMode( KFile::Files | KFile::Directory );
    dlg.setCaption( i18n("Select Files or Directory to Open") );

    if ( dlg.exec() == QDialog::Accepted )
    {
        KURL::List urls = dlg.selectedURLs();
        KURL::List::ConstIterator it = urls.begin();
        for ( ; it != urls.end(); ++it )
        {
            KFileItem item( KFileItem::Unknown, KFileItem::Unknown, *it );
            if ( item.isDir() )
                fileWidget->setURL( *it, true );
            else
                showImage( &item, true );
        }
    }
}

void KuickShow::deleteAllViewers()
{
    QValueListIterator<ImageWindow*> it = s_viewers.begin();
    for ( ; it != s_viewers.end(); ++it ) {
        (*it)->blockSignals( true ); // don't call viewerDeleted()
        (*it)->close( true );
    }

    s_viewers.clear();
    m_viewer = 0L;
}

#include "kuickshow.moc"
