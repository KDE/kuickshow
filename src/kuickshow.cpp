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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <qdir.h>
#include <qdesktopwidget.h>
#include <qdialog.h>
#include <qglobal.h>
#include <qkeycode.h>
#include <qlayout.h>
#include <qsize.h>
#include <qstring.h>

#include <kaboutdata.h>
#include <kaccel.h>
#include <kaction.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kcursor.h>
#include <kdeversion.h>
#include <kfiledialog.h>
#include <kfilemetainfo.h>
#include <kglobal.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kpropertiesdialog.h>
#include <kprotocolinfo.h>
#include <kstatusbar.h>
#include <kstdaction.h>
#include <kstandarddirs.h>
#include <kstartupinfo.h>
#include <ktoolbar.h>
#include <kurlcombobox.h>
#include <kurlcompletion.h>
#include <kurldrag.h>
#include <kwin.h>
#include <kstdguiitem.h>
#include <kimageio.h>

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

static const int URL_ITEM  = 0;
static const int META_ITEM = 1;

QValueList<ImageWindow*> KuickShow::s_viewers;

KuickShow::KuickShow( const char *name )
    : KMainWindow( 0L, name ),
      m_slideshowCycle( 1 ),
      fileWidget( 0L ),
      dialog( 0L ),
      m_viewer( 0L ),
      oneWindowAction( 0L ),
      m_accel( 0L ),
      m_delayedRepeatItem( 0L )
{
    aboutWidget = 0L;
    kdata = new KuickData;
    kdata->load();

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
        {
            showImage( &item, true, false, true ); // show in new window, not fullscreen-forced and move to 0,0
//    showImage( &item, true, false, false ); // show in new window, not fullscreen-forced and not moving to 0,0
        }
        else if ( item.isDir() )
        {
            startDir = url;
            isDir = true;
        }

        // need to check remote files
        else if ( !url.isLocalFile() )
        {
            KMimeType::Ptr mime = KMimeType::findByURL( url );
            QString name = mime->name();
#if KDE_VERSION >= 310
            if ( name == "application/octet-stream" ) // unknown -> stat()
                name = KIO::NetAccess::mimetype( url, this );
#endif

            if ( name.startsWith( "image/" ) )
            {
                FileWidget::setImage( item, true );
                showImage( &item, true, false, true );
            }
            else // assume directory, KDirLister will tell us if we can't list
            {
                startDir = url;
                isDir = true;
            }
        }
        // else // we don't handle local non-images
    }

    if ( (kdata->startInLastDir && args->count() == 0) || args->isSet( "lastfolder" )) {
        kc->setGroup( "SessionSettings");
        startDir = kc->readPathEntry( "CurrentDirectory", startDir.url() );
    }

    if ( s_viewers.isEmpty() || isDir ) {
        initGUI( startDir );
        show();
    }

    else { // don't show browser, when image on commandline
        hide();
        KStartupInfo::appStarted();
    }
}


KuickShow::~KuickShow()
{
    saveSettings();

    if ( m_viewer )
        m_viewer->close( true );

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


    fileWidget->setAcceptDrops(true);
    connect( fileWidget, SIGNAL( dropped( const KFileItem *, QDropEvent *, const KURL::List & )),
             this, SLOT( slotDropped( const KFileItem *, QDropEvent *, const KURL::List &)) );

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

    oneWindowAction = new KToggleAction( i18n("Open Only One Image Window"),
                                         "window_new",
                                         KShortcut( CTRL+Key_N ), coll,
                                         "kuick_one window" );

    m_toggleBrowserAction = new KToggleAction( i18n("Show/Hide File Browser"), KShortcut( Key_Space ), coll, "toggleBrowser" );
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
    //tBar->insertSeparator();
    //about->plug( tBar );

    QPopupMenu *help = helpMenu( QString::null, false );
    mBar->insertItem( KStdGuiItem::help().text() , help );


    KStatusBar* sBar = statusBar();
    sBar->insertItem( "           ", URL_ITEM, 10 );
    sBar->insertItem( "                          ", META_ITEM, 2 );
    sBar->setItemAlignment(URL_ITEM, QLabel::AlignVCenter | QLabel::AlignLeft);

    fileWidget->setFocus();

    KConfig *kc = KGlobal::config();
    kc->setGroup("SessionSettings");
    bool oneWindow = kc->readBoolEntry("OpenImagesInActiveWindow", true );
    oneWindowAction->setChecked( oneWindow );

    tBar->show();

    // Address box in address tool bar
    KToolBar *addressToolBar = toolBar( "address_bar" );
    const int ID_ADDRESSBAR = 1;

    cmbPath = new KURLComboBox( KURLComboBox::Directories,
                                true, addressToolBar, "address_combo_box" );
    KURLCompletion *cmpl = new KURLCompletion( KURLCompletion::DirCompletion );
    cmbPath->setCompletionObject( cmpl );
    cmbPath->setAutoDeleteCompletionObject( true );

    addressToolBar->insertWidget( ID_ADDRESSBAR, 1, cmbPath);
    addressToolBar->setItemAutoSized( ID_ADDRESSBAR );

    connect( cmbPath, SIGNAL( urlActivated( const KURL& )),
             this, SLOT( slotSetURL( const KURL& )));
    connect( cmbPath, SIGNAL( returnPressed()),
             this, SLOT( slotURLComboReturnPressed()));


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

    // ### somehow the filewidget isn't resized at all sometimes, when initGUI
    // is called lazily, not from this c'tor. (just a tiny little widget in
    // the upper left corner of the mainwindow. Work around that.
    fileWidget->resize( size() );
}

void KuickShow::slotSetURL( const KURL& url )
{
    fileWidget->setURL( url, true );
}

void KuickShow::slotURLComboReturnPressed()
{
    KURL where = KURL::fromPathOrURL( cmbPath->currentText() );
    slotSetURL( where );
}

void KuickShow::viewerDeleted()
{
    ImageWindow *viewer = (ImageWindow*) sender();
    s_viewers.remove( viewer );
    if ( viewer == m_viewer )
        m_viewer = 0L;

    if ( !haveBrowser() && s_viewers.isEmpty() ) {
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
    KFileItem *item = const_cast<KFileItem *>( fi );
    statusBar()->changeItem( item->getStatusBarInfo(), URL_ITEM );
    bool image = FileWidget::isImage( fi );

    QString meta;
    if ( image )
    {
        KFileMetaInfo info = item->metaInfo();
        if ( info.isValid() )
        {
            meta = info.item( KFileMimeTypeInfo::Size ).string();
            KFileMetaInfoGroup group = info.group( "Technical" );
            if ( group.isValid() )
            {
                QString bpp = group.item( "BitDepth" ).string();
                if ( !bpp.isEmpty() )
                    meta.append( ", " ).append( bpp );
            }
        }
    }
    statusBar()->changeItem( meta, META_ITEM );

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

    cmbPath->setURL( url );
    statusBar()->changeItem( url.prettyURL(), URL_ITEM );
}

void KuickShow::slotSelected( const KFileItem *item )
{
    showImage( item, !oneWindowAction->isChecked() );
}

// downloads item if necessary
void KuickShow::showFileItem( ImageWindow * /*view*/,
                              const KFileItem * /*item*/ )
{

}

void KuickShow::showImage( const KFileItem *fi,
                           bool newWindow, bool fullscreen, bool moveToTopLeft )
{
    newWindow  |= !m_viewer;
    fullscreen |= (newWindow && kdata->fullScreen);
    if ( FileWidget::isImage( fi ) ) {

        if ( newWindow ) {
            m_viewer = new ImageWindow( kdata->idata, 0L, "ImageWindow" );
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
        KIO::NetAccess::download(fi->url(), filename, this);

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
                    ((ImlibWidget *) safeViewer->centralWidget())->cacheImage( item->url().path() ); // FIXME
            }

            m_viewer = safeViewer;
        } // m_viewer created successfully
    } // isImage
}

void KuickShow::startSlideShow()
{
    if (kdata->slideshowPlayRandom) {
    	startSlideShowRandom();
    } else {
	KFileItem *item = kdata->slideshowStartAtFirst ?
			fileWidget->gotoFirstImage() :
			fileWidget->getCurrentItem(false);
	
	if ( item ) {
		m_slideshowCycle = 1;
		fileWidget->actionCollection()->action("kuick_slideshow")->setEnabled( false );
		showImage( item, !oneWindowAction->isChecked(),
			kdata->slideshowFullscreen );
		m_slideTimer->start( kdata->slideDelay );
	}
    }
}

void KuickShow::startSlideShowRandom()
{
    m_randomItem.clear();
    KFileItem *item = NULL;
    fileWidget->hide();
    
    // Adds all item in a vector
    m_randomItem = fileWidget->getCompleteFileList();
   
    // Finished!
    fileWidget->show();
    // Shuffle vector
    size_t n = m_randomItem.size();

    for (size_t i = 0; i < n; ++i) {
	size_t j = kapp->random() % n;

	KFileItem *it = m_randomItem[i];
	m_randomItem[i] = m_randomItem[j];
	m_randomItem[j] = it;
    }
    
    // Starts slide show
    if ( n ) {
	item = m_randomItem.front();
	m_randomSlideIndex = 1;
	
        m_slideshowCycle = 1;
	fileWidget->actionCollection()->action("kuick_slideshow")->setEnabled( false );
	
	showImage( item, !oneWindowAction->isChecked(),
                   kdata->slideshowFullscreen );
	
        m_slideTimer->start( kdata->slideDelay );
    }
}

void KuickShow::nextRandomSlide()
{
    if ( !m_viewer ) {
        m_slideshowCycle = 1;
        fileWidget->actionCollection()->action("kuick_slideshow")->setEnabled( true );
        return;
    }

    if (m_randomItem.size() == m_randomSlideIndex) {
	m_viewer->close( true );
        fileWidget->actionCollection()->action("kuick_slideshow")->setEnabled( true );
	m_slideTimer->stop();
	return;
    }
    
    KFileItem *item = m_randomItem[m_randomSlideIndex ++];

    if ( item ) {
	nextRandomSlide( item );
        m_slideshowCycle++;
        return;
    }    

    nextRandomSlide( item );
}

void KuickShow::nextRandomSlide( KFileItem *item )
{
    m_viewer->showNextImage( item->url().path() );
    fileWidget->setCurrentItem( item );
    m_slideTimer->start( kdata->slideDelay );
}

void KuickShow::nextSlide()
{
    if (kdata->slideshowPlayRandom) {
    	nextRandomSlide();
    } else {
	if ( !m_viewer ) {
		m_slideshowCycle = 1;
		fileWidget->actionCollection()->action("kuick_slideshow")->setEnabled( true );
		return;
	}
	KFileItem *item= fileWidget->getNext( true );
	
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
    ImageWindow *iw = new ImageWindow( 0, this, "PrintingImage" );
    KFileItem *item;
    while ( (item = it.current()) ) {
        if (FileWidget::isImage( item ) && iw->showNextImage( item->url().path()))
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

void KuickShow::slotDropped( const KFileItem *, QDropEvent *, const KURL::List &urls)
{
    KURL::List::ConstIterator it = urls.begin();
    for ( ; it != urls.end(); ++it )
    {
        KFileItem item( KFileItem::Unknown, KFileItem::Unknown, *it );
        if ( FileWidget::isImage( &item ) )
            showImage( &item, true );
        else
            fileWidget->setURL( *it, true );
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
        QString filename;
        KIO::NetAccess::download(item->url(), filename, this);
        view->showNextImage( filename );
        if (m_slideTimer->isActive())
            m_slideTimer->start( kdata->slideDelay );

        if ( kdata->preloadImage && item_next && item_next->url().isLocalFile() ) // preload next image
            if ( FileWidget::isImage( item_next ) )
                ((ImlibWidget *)view->centralWidget())->cacheImage( item_next->url().path() ); // ###
    }
}

bool KuickShow::eventFilter( QObject *o, QEvent *e )
{
    if ( m_delayedRepeatItem ) // we probably need to install an eventFilter over
        return true;    // kapp, to make it really safe

    bool ret = false;
    int eventType = e->type();
    QKeyEvent *k = 0L;
    if ( eventType == QEvent::KeyPress )
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
        // The XWindow used to display Imlib's image is being resized when
        // switching images, causing enter- and leaveevents for this
        // ImageWindow, leading to the cursor being unhidden. So we simply
        // don't pass those events to KCursor to prevent that.
        if ( eventType != QEvent::Leave && eventType != QEvent::Enter )
            KCursor::autoHideEventFilter( o, e );

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
//      KFileItem *cur = fileWidget->getCurrentItem( false );
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
                QString filename;
                KIO::NetAccess::download(item->url(), filename, this);
                m_viewer->showNextImage( filename );

                if ( kdata->preloadImage && item_next && item_next->url().isLocalFile() ) // preload next image
                    if ( FileWidget::isImage( item_next ) )
                        ((ImlibWidget *) m_viewer->centralWidget())->cacheImage( item_next->url().path() ); // ###

                ret = true; // don't pass keyEvent
            }
        } // keyPressEvent on ImageWindow


        // doubleclick closes image window
        // and shows browser when last window closed via doubleclick
        else if ( eventType == QEvent::MouseButtonDblClick )
        {
            QMouseEvent *ev = static_cast<QMouseEvent*>( e );
            if ( ev->button() == LeftButton )
            {
                if ( s_viewers.count() == 1 )
                {
                    if ( !fileWidget )
                    {
                        KURL start;
                        QFileInfo fi( m_viewer->filename() );
                        start.setPath( fi.dirPath( true ) );
                        initGUI( start );
                    }
                    show();
                    raise();
                }

                window->close( true );

                ev->accept();
                ret = true;
            }
        }

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
        initGUI( KURL::fromPathOrURL( QDir::homeDirPath() ) );
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
    if ( !aboutWidget )
        aboutWidget = new AboutWidget( 0L, "about" );

    aboutWidget->adjustSize();

#if KDE_VERSION >= 310
    KDialog::centerOnScreen( aboutWidget );
#else
// Not fixed because it must be dead code now.
    QDesktopWidget *desktop = QApplication::desktop();
    int screen = desktop->screenNumber( aboutWidget );
    if ( screen == -1 )
        screen = desktop->primaryScreen();

    QRect r = desktop->screenGeometry( screen );
    aboutWidget->move( r.center().x() - aboutWidget->width()/2,
                       r.center().y() - aboutWidget->height()/2 );
#endif

    aboutWidget->show();
}

// ------ sessionmanagement - load / save current directory -----
void KuickShow::readProperties( KConfig *kc )
{
    assert( fileWidget ); // from SM, we should always have initGUI on startup
    QString dir = kc->readPathEntry( "CurrentDirectory" );
    if ( !dir.isEmpty() ) {
        fileWidget->setURL( KURL::fromPathOrURL( dir ), true );
        fileWidget->clearHistory();
    }

    QStringList images = kc->readPathListEntry( "Images shown" );
    QStringList::Iterator it;
    for ( it = images.begin(); it != images.end(); ++it ) {
        KFileItem item( KFileItem::Unknown, KFileItem::Unknown, KURL::fromPathOrURL( *it ), false );
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
    kc->writePathEntry( "CurrentDirectory", fileWidget->url().url() );
    kc->writeEntry( "Browser visible", fileWidget->isVisible() );

    QStringList urls;
    QValueListIterator<ImageWindow*> it;
    for ( it = s_viewers.begin(); it != s_viewers.end(); ++it )
        urls.append( (*it)->filename() );

    kc->writePathEntry( "Images shown", urls );
}

// --------------------------------------------------------------

void KuickShow::saveSettings()
{
    KConfig *kc = KGlobal::config();

    kc->setGroup("SessionSettings");
    if ( oneWindowAction )
        kc->writeEntry( "OpenImagesInActiveWindow", oneWindowAction->isChecked() );

    if ( fileWidget ) {
        kc->writePathEntry( "CurrentDirectory", fileWidget->url().url() );
        fileWidget->writeConfig( kc, "Filebrowser" );
    }

    kc->sync();
}


void KuickShow::messageCantLoadImage( const QString& filename )
{
    m_viewer->clearFocus();
    QString tmp = i18n("<html><pP>Unable to load the image %1.</p>"
                       "<p>Perhaps the file format is unsupported.</p><p>Supported Types:</p><ul><li>").arg(filename);
		       
    tmp.append(KImageIO::mimeTypes(KImageIO::Reading).join("</li><li>"));
    tmp.append("</li></ul>");
    KMessageBox::sorry( m_viewer, tmp, i18n("Image Error") );
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
        KWin::activateWindow( winId() ); // ### this should not be necessary
//         setFocus();
    }
    else if ( !s_viewers.isEmpty() )
        hide();
}

void KuickShow::slotOpenURL()
{
    KFileDialog dlg(QString::null, kdata->fileFilter, this, "filedialog", true);
    dlg.setMode( KFile::Files | KFile::Directory );
    dlg.setCaption( i18n("Select Files or Folder to Open") );

    if ( dlg.exec() == QDialog::Accepted )
    {
        KURL::List urls = dlg.selectedURLs();
        KURL::List::ConstIterator it = urls.begin();
        for ( ; it != urls.end(); ++it )
        {
            KFileItem item( KFileItem::Unknown, KFileItem::Unknown, *it );
            if ( FileWidget::isImage( &item ) )
                showImage( &item, true );
            else
                fileWidget->setURL( *it, true );
        }
    }
}

void KuickShow::deleteAllViewers()
{
    QValueListIterator<ImageWindow*> it = s_viewers.begin();
    for ( ; it != s_viewers.end(); ++it ) {
        (*it)->disconnect( SIGNAL( destroyed() ), this, SLOT( viewerDeleted() ));
        (*it)->close( true );
    }

    s_viewers.clear();
    m_viewer = 0L;
}

KActionCollection * KuickShow::actionCollection() const
{
    if ( fileWidget )
        return fileWidget->actionCollection();

    return KMainWindow::actionCollection();
}

#include "kuickshow.moc"
