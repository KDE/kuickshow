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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <stdio.h>
#include <assert.h>
#include <qtextstream.h>
#include <qdir.h>
#include <qdesktopwidget.h>
#include <qdialog.h>
#include <qglobal.h>
#include <qnamespace.h>
#include <qlayout.h>
#include <qsize.h>
#include <qstring.h>
//Added by qt3to4:
#include <QKeyEvent>
#include <QEvent>
#include <QDropEvent>
#include <QLabel>
#include <Q3PopupMenu>
#include <QMouseEvent>
#include <QMenuItem>
#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kaction.h>
#include <kactionmenu.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kcursor.h>
#include <kdeversion.h>
#include <kactionmenu.h>
#include <kfiledialog.h>
#include <kfilemetainfo.h>
#include <kglobal.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <kmenu.h>
#include <kpropertiesdialog.h>
#include <kprotocolinfo.h>
#include <kstatusbar.h>
#include <kstandardaction.h>
#include <kstandarddirs.h>
#include <kstartupinfo.h>
#include <ktoolbar.h>
#include <kurlcombobox.h>
#include <kurlcompletion.h>
#include <kwm.h>
#include <KStandardGuiItem>
#include <kstandardshortcut.h>
#include <kconfiggroup.h>
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

QList<ImageWindow*> KuickShow::s_viewers;

KuickShow::KuickShow( const char *name )
    : KXmlGuiWindow( 0L, name ),
      m_slideshowCycle( 1 ),
      fileWidget( 0L ),
      dialog( 0L ),
      id( 0L ),
      m_viewer( 0L ),
      oneWindowAction( 0L ),
      m_accel( 0L ),
      m_delayedRepeatItem( 0L ),
      m_slideShowStopped(false)
{
    aboutWidget = 0L;
    kdata = new KuickData;
    kdata->load();

    initImlib();
    resize( 400, 500 );

    m_slideTimer = new QTimer( this );
    connect( m_slideTimer, SIGNAL( timeout() ), SLOT( nextSlide() ));


    KSharedConfig::Ptr kc = KGlobal::config();

    bool isDir = false; // true if we get a directory on the commandline

    // parse commandline options
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    // files to display
    // either a directory to display, an absolute path, a relative path, or a URL
    KUrl startDir;
    startDir.setPath( QDir::currentPath() + '/' );
    for ( int i = 0; i < args->count(); i++ ) {
        KUrl url = args->url( i );
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
            KMimeType::Ptr mime = KMimeType::findByUrl( url );
            QString name = mime->name();
            if ( name == "application/octet-stream" ) // unknown -> stat()
                name = KIO::NetAccess::mimetype( url, this );

	    // text/* is a hack for bugs.kde.org-attached-images urls.
	    // The real problem here is that NetAccess::mimetype does a HTTP HEAD, which doesn't
	    // always return the right mimetype. The rest of KDE start a get() instead....
            if ( name.startsWith( "image/" ) || name.startsWith( "text/" ) )
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

    free( id );
    kapp->quit();

    delete kdata;
}

// TODO convert to use xmlui file
void KuickShow::initGUI( const KUrl& startDir )
{
    fileWidget = new FileWidget( startDir, this, "MainWidget" );
    setFocusProxy( fileWidget );

    KActionCollection *coll = fileWidget->actionCollection();

    connect( fileWidget, SIGNAL( fileSelected( const KFileItem * ) ),
             this, SLOT( slotSelected( const KFileItem * ) ));

    connect( fileWidget, SIGNAL( fileHighlighted( const KFileItem * )),
             this, SLOT( slotHighlighted( const KFileItem * ) ));

    connect( fileWidget, SIGNAL( urlEntered( const KUrl&  )),
             this, SLOT( dirSelected( const KUrl& )) );


    fileWidget->setAcceptDrops(true);
    connect( fileWidget, SIGNAL( dropped( const KFileItem *, QDropEvent *, const KUrl::List & )),
             this, SLOT( slotDropped( const KFileItem *, QDropEvent *, const KUrl::List &)) );

    // setup actions
    QAction *open = KStandardAction::open( this, SLOT( slotOpenURL() ),
                                      coll );
    coll->addAction( "openURL", open );

    QAction *print = KStandardAction::print( this, SLOT( slotPrint() ),
                                        coll );
    coll->addAction( "kuick_print", print );
    print->setText( i18n("Print Image...") );

    QAction *configure = coll->addAction( "kuick_configure" );
    configure->setText( i18n("Configure %1...",KGlobal::mainComponent().aboutData()->programName() ) );
    configure->setIcon( KIcon( "configure" ) );
    connect( configure, SIGNAL( triggered() ), this, SLOT( configuration() ) );

    QAction *slide = coll->addAction( "kuick_slideshow" );
    slide->setText( i18n("Start Slideshow" ) );
    slide->setIcon( KIcon("ksslide" ));
    slide->setShortcut( Qt::Key_F2 );
    connect( slide, SIGNAL( triggered() ), this, SLOT( startSlideShow() ));

    QAction *about = coll->addAction( "about" );
    about->setText( i18n( "About KuickShow" ) );
    about->setIcon( KIcon("about") );
    connect( about, SIGNAL( triggered() ), this, SLOT( about() ) );

    oneWindowAction = coll->add<KToggleAction>( "kuick_one window" );
    oneWindowAction->setText( i18n("Open Only One Image Window") );
    oneWindowAction->setIcon( KIcon( "window-new" ) );
    oneWindowAction->setShortcut( Qt::CTRL+Qt::Key_N );

    m_toggleBrowserAction = coll->add<KToggleAction>( "toggleBrowser" );
    m_toggleBrowserAction->setText( i18n("Show File Browser") );
    m_toggleBrowserAction->setShortcut( Qt::Key_Space );
    m_toggleBrowserAction->setCheckedState(KGuiItem(i18n("Hide File Browser")));
    connect( m_toggleBrowserAction, SIGNAL( toggled( bool ) ),
             SLOT( toggleBrowser() ));

    QAction *showInOther = coll->addAction( "kuick_showInOtherWindow" );
    showInOther->setText( i18n("Show Image") );
    connect( showInOther, SIGNAL( triggered() ), SLOT( slotShowInOtherWindow() ));

    QAction *showInSame = coll->addAction( "kuick_showInSameWindow" );
    showInSame->setText( i18n("Show Image in Active Window") );
    connect( showInSame, SIGNAL( triggered() ), this, SLOT( slotShowInSameWindow() ) );

    QAction *showFullscreen = coll->addAction( "kuick_showFullscreen" );
    showFullscreen->setText( i18n("Show Image in Fullscreen Mode") );
    connect( showFullscreen, SIGNAL( triggered() ), this, SLOT( slotShowFullscreen() ) );

    QAction *quit = KStandardAction::quit( this, SLOT(slotQuit()), coll);
    coll->addAction( "quit", quit );

    // remove QString::null parameter -- ellis
//    coll->readShortcutSettings( QString::null );
//    m_accel = coll->accel();

    // menubar
    KMenuBar *mBar = menuBar();
    Q3PopupMenu *fileMenu = new Q3PopupMenu( mBar, "file" );
    fileMenu->addAction(open);
    fileMenu->addAction(showInOther);
    fileMenu->addAction(showInSame);
    fileMenu->addAction(showFullscreen);
    fileMenu->addSeparator();
    fileMenu->addAction(slide);
    fileMenu->addAction(print);
    fileMenu->addSeparator();
    fileMenu->addAction(quit);

    Q3PopupMenu *editMenu = new Q3PopupMenu( mBar, "edit" );
    editMenu->addAction(coll->action("mkdir"));
    editMenu->addAction(coll->action("delete"));
    editMenu->addSeparator();
    editMenu->addAction(coll->action("properties"));


    // remove the Sorting submenu (and the separator below)
    // from the main contextmenu
    KActionMenu *sortingMenu = static_cast<KActionMenu*>( coll->action("sorting menu"));
    KActionMenu *mainActionMenu = static_cast<KActionMenu*>( coll->action("popupMenu"));
    QMenu *mainPopup = mainActionMenu->popupMenu();
/*
    int sortingIndex = mainPopup->indexOf( sortingMenu->itemId( 0 ) );
    int separatorId = mainPopup->idAt( sortingIndex + 1 );
    QMenuItem *separatorItem = mainPopup->findItem( separatorId );
    if ( separatorItem && separatorItem->isSeparator() )
        mainPopup->removeItem( separatorId );
    mainActionMenu->remove( sortingMenu );
*/

    // add the sorting menu and a separator into the View menu
    KActionMenu *viewActionMenu = static_cast<KActionMenu*>( coll->action("view menu"));
    viewActionMenu->popupMenu()->addSeparator();
    viewActionMenu->popupMenu()->addAction(sortingMenu); //, 0 ); // on top of the menu


    Q3PopupMenu *settingsMenu = new Q3PopupMenu( mBar, "settings" );
    settingsMenu->addAction(configure);

    mBar->insertItem( i18n("&File"), fileMenu );
    mBar->insertItem( i18n("&Edit"), editMenu );
    mBar->addAction(viewActionMenu);
    mBar->insertItem( i18n("&Settings"), settingsMenu );

    // toolbar
    KToolBar *tBar = toolBar(i18n("Main Toolbar"));

    tBar->addAction(coll->action("up"));
    tBar->addAction(coll->action("back"));
    tBar->addAction(coll->action("forward"));
    tBar->addAction(coll->action("home"));
    tBar->addAction(coll->action("reload"));

    tBar->addSeparator();

    tBar->addAction(coll->action( "short view" ));
    tBar->addAction(coll->action( "detailed view" ));
    tBar->addAction(coll->action( "preview"));

    tBar->addSeparator();
    tBar->addAction(configure);
    tBar->addAction(slide);
    tBar->addSeparator();
    tBar->addAction(oneWindowAction);
    tBar->addAction(print);
    tBar->addSeparator();
    tBar->addAction(about);

    KMenu *help = helpMenu( QString::null, false );
    mBar->insertItem( KStandardGuiItem::help().text() , help );


    KStatusBar* sBar = statusBar();
    sBar->insertItem( "           ", URL_ITEM, 10 );
    sBar->insertItem( "                          ", META_ITEM, 2 );
    sBar->setItemAlignment(URL_ITEM, Qt::AlignVCenter | Qt::AlignLeft);

    fileWidget->setFocus();

    KConfigGroup kc(KGlobal::config(), "SessionSettings");
    bool oneWindow = kc.readEntry("OpenImagesInActiveWindow", true );
    oneWindowAction->setChecked( oneWindow );

    tBar->show();

    // Address box in address tool bar
    KToolBar *addressToolBar = toolBar( "address_bar" );
    const int ID_ADDRESSBAR = 1;

    cmbPath = new KUrlComboBox( KUrlComboBox::Directories,
                                true, addressToolBar );
    KUrlCompletion *cmpl = new KUrlCompletion( KUrlCompletion::DirCompletion );
    cmbPath->setCompletionObject( cmpl );
    cmbPath->setAutoDeleteCompletionObject( true );

//    addressToolBar->addWidget( ID_ADDRESSBAR, 1, cmbPath);
//    addressToolBar->setItemAutoSized( ID_ADDRESSBAR );

    connect( cmbPath, SIGNAL( urlActivated( const KUrl& )),
             this, SLOT( slotSetURL( const KUrl& )));
    connect( cmbPath, SIGNAL( returnPressed()),
             this, SLOT( slotURLComboReturnPressed()));


    fileWidget->initActions();
    fileWidget->clearHistory();
    dirSelected( fileWidget->url() );

    setCentralWidget( fileWidget );

    setupGUI( KXmlGuiWindow::Save );

    qobject_cast<KAction *>(coll->action( "reload" ))->setShortcut( KStandardShortcut::reload() );
    qobject_cast<KAction *>(coll->action( "short view" ))->setShortcut(Qt::Key_F6);
    qobject_cast<KAction *>(coll->action( "detailed view" ))->setShortcut(Qt::Key_F7);
    qobject_cast<KAction *>(coll->action( "show hidden" ))->setShortcut(Qt::Key_F8);
    qobject_cast<KAction *>(coll->action( "mkdir" ))->setShortcut(Qt::Key_F10);
    qobject_cast<KAction *>(coll->action( "preview" ))->setShortcut(Qt::Key_F11);
    qobject_cast<KAction *>(coll->action( "separate dirs" ))->setShortcut(Qt::Key_F12);
}

void KuickShow::slotSetURL( const KUrl& url )
{
    fileWidget->setUrl( url, true );
}

void KuickShow::slotURLComboReturnPressed()
{
    KUrl where = KUrl::fromPathOrUrl( cmbPath->currentText() );
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
            meta = info.item("sizeurl").value().toString();
            const QString bpp = info.item( "BitDepth" ).value().toString();
            if ( !bpp.isEmpty() )
                meta.append( ", " ).append( bpp );
        }
    }
    statusBar()->changeItem( meta, META_ITEM );

    fileWidget->actionCollection()->action("kuick_print")->setEnabled( image );
    fileWidget->actionCollection()->action("kuick_showInSameWindow")->setEnabled( image );
    fileWidget->actionCollection()->action("kuick_showInOtherWindow")->setEnabled( image );
    fileWidget->actionCollection()->action("kuick_showFullscreen")->setEnabled( image );
}

void KuickShow::dirSelected( const KUrl& url )
{
    if ( url.isLocalFile() )
        setCaption( url.path() );
    else
        setCaption( url.prettyUrl() );

    cmbPath->setUrl( url );
    statusBar()->changeItem( url.prettyUrl(), URL_ITEM );
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
            m_viewer = new ImageWindow( kdata->idata, id, 0L, "image window" );
            s_viewers.append( m_viewer );

	    connect( m_viewer, SIGNAL( nextSlideRequested() ), this, SLOT( nextSlide() ));
            connect( m_viewer, SIGNAL( destroyed() ), SLOT( viewerDeleted() ));
            connect( m_viewer, SIGNAL( sigFocusWindow( ImageWindow *) ),
                     this, SLOT( slotSetActiveViewer( ImageWindow * ) ));
            connect( m_viewer, SIGNAL( sigBadImage(const QString& ) ),
                     this, SLOT( messageCantLoadImage(const QString &) ));
            connect( m_viewer, SIGNAL( requestImage( ImageWindow *, int )),
                     this, SLOT( slotAdvanceImage( ImageWindow *, int )));
	    connect( m_viewer, SIGNAL( pauseSlideShowSignal() ),
		     this, SLOT( pauseSlideShow() ) );
            connect( m_viewer, SIGNAL (deleteImage ()),
                     this, SLOT (slotDeleteImage ()));
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
                    safeViewer->cacheImage( item->url().path() ); // FIXME
            }

            m_viewer = safeViewer;
        } // m_viewer created successfully
    } // isImage
}

void KuickShow::slotDeleteImage()
{
    KFileItemList list;
    KFileItem *item = fileWidget->getCurrentItem(false);
    list.append (item);
    KFileItem *next = fileWidget->getNext(true);
    if (!next)
        next = fileWidget->getPrevious(true);
        if (next)
        showImage(next, false);
    else
        m_viewer->close(true);
    fileWidget->del(list, false,false);
}

void KuickShow::startSlideShow()
{
    KFileItem *item = kdata->slideshowStartAtFirst ?
                      fileWidget->gotoFirstImage() :
                      fileWidget->getCurrentItem(false);

    if ( item ) {
        m_slideshowCycle = 1;
        fileWidget->actionCollection()->action("kuick_slideshow")->setEnabled( false );
        showImage( item, !oneWindowAction->isChecked(),
                   kdata->slideshowFullscreen );
	if(kdata->slideDelay)
            m_slideTimer->start( kdata->slideDelay );
    }
}

void KuickShow::pauseSlideShow()
{
    if(m_slideShowStopped) {
	if(kdata->slideDelay)
	    m_slideTimer->start( kdata->slideDelay );
	m_slideShowStopped = false;
    }
    else {
	m_slideTimer->stop();
	m_slideShowStopped = true;
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
    if(kdata->slideDelay)
        m_slideTimer->start( kdata->slideDelay );
}


// prints the selected files in the filebrowser
void KuickShow::slotPrint()
{
    const KFileItemList *items = fileWidget->selectedItems();
    if ( !items )
        return;

	KFileItemList::const_iterator it = items->begin();
	const KFileItemList::const_iterator end = items->end();

    // don't show the image, just print
    ImageWindow *iw = new ImageWindow( 0, id, this, "printing image" );
    KFileItem *item;
	for ( ; it != end; ++it ) {
		item = (*it);
        if (FileWidget::isImage( item ) && iw->loadImage( item->url().path()))
            iw->printImage();
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

void KuickShow::slotShowFullscreen()
{
    showImage( fileWidget->getCurrentItem( false ), false, true );
}

void KuickShow::slotDropped( const KFileItem *, QDropEvent *, const KUrl::List &urls)
{
    KUrl::List::ConstIterator it = urls.begin();
    for ( ; it != urls.end(); ++it )
    {
        KFileItem item( KFileItem::Unknown, KFileItem::Unknown, *it );
        if ( FileWidget::isImage( &item ) )
            showImage( &item, true );
        else
            fileWidget->setUrl( *it, true );
    }
}

// try to init the WM border as it is 0,0 when the window is not shown yet.
void KuickShow::show()
{
    KXmlGuiWindow::show();
    (void) Kuick::frameSize( winId() );
}

void KuickShow::slotAdvanceImage( ImageWindow *view, int steps )
{
    KFileItem *item      = 0L; // to be shown
    KFileItem *item_next = 0L; // to be cached

    if ( steps == 0 )
        return;

    // the viewer might not be available yet. Factor this out somewhen.
    if ( !fileWidget ) {
        if ( m_delayedRepeatItem )
            return;

        m_delayedRepeatItem = new DelayedRepeatEvent( view, steps );

        KUrl start;
        QFileInfo fi( view->filename() );
        start.setPath( fi.absolutePath() );
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
        if (m_slideTimer->isActive() && kdata->slideDelay)
            m_slideTimer->start( kdata->slideDelay );

        if ( kdata->preloadImage && item_next && item_next->url().isLocalFile() ) // preload next image
            if ( FileWidget::isImage( item_next ) )
                view->cacheImage( item_next->url().path() ); // ###
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
        if ( KStandardShortcut::quit().contains( k->key() ) ) {
            saveSettings();
            deleteAllViewers();
            ::exit(0);
        }
        else if ( KStandardShortcut::help().contains( k->key() ) ) {
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

            // Qt::Key_Shift shouldn't load the browser in nobrowser mode, it
            // is used for zooming in the imagewindow
            // Qt::Key_Alt shouldn't either - otherwise Alt+F4 doesn't work, the
            // F4 gets eaten (by NetAccess' modal dialog maybe?)
            if ( !fileWidget )
            {
                if ( key != Qt::Key_Escape && key != Qt::Key_Shift && key != Qt::Key_Alt )
                {
                    KUrl start;
                    QFileInfo fi( m_viewer->filename() );
                    start.setPath( fi.absolutePath() );
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

                return KXmlGuiWindow::eventFilter( o, e );
            }

            // we definitely have a fileWidget here!

            if ( key == Qt::Key_Home || KStandardShortcut::home().contains( k->key() ) )
            {
                item = fileWidget->gotoFirstImage();
                item_next = fileWidget->getNext( false );
            }

            else if ( key == Qt::Key_End || KStandardShortcut::end().contains( k->key() ) )
            {
                item = fileWidget->gotoLastImage();
                item_next = fileWidget->getPrevious( false );
            }

            else if ( fileWidget->actionCollection()->action("delete")->shortcuts().contains( key ))
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
                                     (k->state() & Qt::ShiftModifier) == 0) == 0L )
                    return true; // aborted deletion

                // ### check failure asynchronously and restore old item?
                fileWidget->setCurrentItem( item );
            }

            else if ( m_toggleBrowserAction->shortcuts().contains( key ) )
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
                        m_viewer->cacheImage( item_next->url().path() ); // ###

                ret = true; // don't pass keyEvent
            }
        } // keyPressEvent on ImageWindow


        // doubleclick closes image window
        // and shows browser when last window closed via doubleclick
        else if ( eventType == QEvent::MouseButtonDblClick )
        {
            QMouseEvent *ev = static_cast<QMouseEvent*>( e );
            if ( ev->button() == Qt::LeftButton )
            {
                if ( s_viewers.count() == 1 )
                {
                    if ( !fileWidget )
                    {
                        KUrl start;
                        QFileInfo fi( m_viewer->filename() );
                        start.setPath( fi.absolutePath() );
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

    return KXmlGuiWindow::eventFilter( o, e );
}


void KuickShow::configuration()
{
    if ( !m_accel ) {
        KUrl start;
        start.setPath( QDir::homePath() );
        initGUI( KUrl::fromPathOrUrl( QDir::homePath() ) );
    }

    dialog = new KuickConfigDialog( fileWidget->actionCollection(), 0L,
                                    "dialog", false );
    dialog->resize( 540, 510 );
    dialog->setIcon( qApp->windowIcon().pixmap(IconSize(K3Icon::Small),IconSize(K3Icon::Small)) );

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
    QList<ImageWindow*>::Iterator it = s_viewers.begin();
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

    KDialog::centerOnScreen( aboutWidget );

    aboutWidget->show();
}

// ------ sessionmanagement - load / save current directory -----
void KuickShow::readProperties( KConfig *kc )
{
    assert( fileWidget ); // from SM, we should always have initGUI on startup
    QString dir = kc->readPathEntry( "CurrentDirectory" );
    if ( !dir.isEmpty() ) {
        fileWidget->setUrl( KUrl::fromPathOrUrl( dir ), true );
        fileWidget->clearHistory();
    }

    QStringList images = kc->readPathListEntry( "Images shown" );
    QStringList::Iterator it;
    for ( it = images.begin(); it != images.end(); ++it ) {
        KFileItem item( KFileItem::Unknown, KFileItem::Unknown, KUrl::fromPathOrUrl( *it ), false );
        if ( item.isReadable() )
            showImage( &item, true );
    }

    if ( !s_viewers.isEmpty() ) {
        bool visible = kc->readEntry( "Browser visible", true );
        if ( !visible )
            hide();
    }
}

void KuickShow::saveProperties( KConfig *kc )
{
    kc->writePathEntry( "CurrentDirectory", fileWidget->url().url() );
    kc->writeEntry( "Browser visible", fileWidget->isVisible() );

    QStringList urls;
    QList<ImageWindow*>::Iterator it;
    for ( it = s_viewers.begin(); it != s_viewers.end(); ++it )
        urls.append( (*it)->filename() );

    kc->writePathEntry( "Images shown", urls );
}

// --------------------------------------------------------------

void KuickShow::saveSettings()
{
    KSharedConfig::Ptr kc = KGlobal::config();

    kc->setGroup("SessionSettings");
    if ( oneWindowAction )
        kc->writeEntry( "OpenImagesInActiveWindow", oneWindowAction->isChecked() );

    if ( fileWidget ) {
        KConfigGroup group( kc, "Filebrowser" );
        kc->writePathEntry( "CurrentDirectory", fileWidget->url().url() );
        fileWidget->writeConfig( group);
    }

    kc->sync();
}


void KuickShow::messageCantLoadImage( const QString& filename )
{
    m_viewer->clearFocus();
    QString tmp = i18n("Unable to load the image %1.\n"
                       "Perhaps the file format is unsupported or "
                       "your Imlib is not installed properly.", filename);
    KMessageBox::sorry( m_viewer, tmp, i18n("Image Error") );
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
        QString paletteFile = KStandardDirs::locate( "data", "kuickshow/im_palette.pal" );
        // FIXME - does the qstrdup() cure the segfault in imlib eventually?
        char *file = qstrdup( paletteFile.toLocal8Bit() );
        par.palettefile = file;
        par.flags |= PARAMS_PALETTEFILE;

        qWarning("Palettefile: %s", par.palettefile );

        id = Imlib_init_with_params( x11Display(), &par );

        if ( !id ) {
            QString tmp = i18n("Unable to initialize \"Imlib\".\n"
                               "Start kuickshow from the command line "
                               "and look for error messages.\n"
                               "The program will now quit.");
            KMessageBox::error( this, tmp, i18n("Fatal Imlib Error") );

            ::exit(1);
        }
    }
}


void KuickShow::initImlibParams( ImData *idata, ImlibInitParams *par )
{
    par->flags = ( PARAMS_REMAP | PARAMS_VISUALID |
                   PARAMS_FASTRENDER | PARAMS_HIQUALITY | PARAMS_DITHER |
                   PARAMS_IMAGECACHESIZE | PARAMS_PIXMAPCACHESIZE );

    Visual* defaultvis = DefaultVisual(x11Display(), x11Screen());

    par->paletteoverride = idata->ownPalette  ? 1 : 0;
    par->remap           = idata->fastRemap   ? 1 : 0;
    par->fastrender      = idata->fastRender  ? 1 : 0;
    par->hiquality       = idata->dither16bit ? 1 : 0;
    par->dither          = idata->dither8bit  ? 1 : 0;
    par->visualid	 = defaultvis->visualid;
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
        KWM::activateWindow( winId() ); // ### this should not be necessary
//         setFocus();
    }
    else if ( !s_viewers.isEmpty() )
        hide();
}

void KuickShow::slotOpenURL()
{
    KFileDialog dlg(KUrl(), kdata->fileFilter, this);
    dlg.setMode( KFile::Files | KFile::Directory );
    dlg.setCaption( i18n("Select Files or Folder to Open") );

    if ( dlg.exec() == QDialog::Accepted )
    {
        KUrl::List urls = dlg.selectedUrls();
        KUrl::List::ConstIterator it = urls.begin();
        for ( ; it != urls.end(); ++it )
        {
            KFileItem item( KFileItem::Unknown, KFileItem::Unknown, *it );
            if ( FileWidget::isImage( &item ) )
                showImage( &item, true );
            else
                fileWidget->setUrl( *it, true );
        }
    }
}

void KuickShow::deleteAllViewers()
{
    QList<ImageWindow*>::Iterator it = s_viewers.begin();
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

    return KXmlGuiWindow::actionCollection();
}

#include "kuickshow.moc"
