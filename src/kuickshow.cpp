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
#include <QMenu>
#include <QMouseEvent>
#include <QMenuItem>
#include <QAbstractItemView>
#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kaction.h>
#include <kactionmenu.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kcursor.h>
#include <kdebug.h>
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
#include <kprotocolmanager.h>
#include <kstatusbar.h>
#include <kstandardaction.h>
#include <kstandarddirs.h>
#include <KStandardGuiItem>
#include <kstartupinfo.h>
#include <ktoolbar.h>
#include <kurlcombobox.h>
#include <kurlcompletion.h>
#include <kwindowsystem.h>
#include <kstandardshortcut.h>
#include <kconfiggroup.h>
#include "aboutwidget.h"
#include "filecache.h"
#include "filewidget.h"
#include "imdata.h"
#include "imagewindow.h"
#include "imlibwidget.h"
#include "kuick.h"

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
    : KXmlGuiWindow( 0L ),
      m_slideshowCycle( 1 ),
      fileWidget( 0L ),
      dialog( 0L ),
      id( 0L ),
      m_viewer( 0L ),
      oneWindowAction( 0L ),
      m_delayedRepeatItem( 0L ),
      m_slideShowStopped(false)
{
    setObjectName(name);
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

    int numArgs = args->count();
    if ( numArgs >= 10 )
    {
        // Even though the 1st i18n string will never be used, it needs to exist for plural handling - mhunter
        if ( KMessageBox::warningYesNo(
                 this,
                 i18np("Do you really want to display this 1 image at the same time? This might be quite resource intensive and could overload your computer.<br>If you choose %1, only the first image will be shown.",
                      "Do you really want to display these %n images at the same time? This might be quite resource intensive and could overload your computer.<br>If you choose %1, only the first image will be shown.", numArgs).arg(KStandardGuiItem::no().plainText()),
                 i18n("Display Multiple Images?"))
             != KMessageBox::Yes )
        {
            numArgs = 1;
        }
    }

    for ( int i = 0; i < numArgs; i++ ) {
        KUrl url = args->url( i );
        KFileItem item( KFileItem::Unknown, KFileItem::Unknown, url, false );

        // for remote URLs, we don't know if it's a file or directory, but
        // FileWidget::isImage() should correct in most cases.
        // For non-local non-images, we just assume directory.

        if ( FileWidget::isImage( item ) )
        {
            showImage( item, true, false, true ); // show in new window, not fullscreen-forced and move to 0,0
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
                showImage( item, true, false, true );
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
        KConfigGroup sessGroup(kc, "SessionSettings");
        startDir = sessGroup.readPathEntry( "CurrentDirectory", startDir.url() );
    }

    if ( s_viewers.isEmpty() || isDir ) {
        initGUI( startDir );
	if (!kapp->isSessionRestored()) // during session management, readProperties() will show()
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

    delete m_viewer;

    FileCache::shutdown();
    free( id );
    kapp->quit();

    delete kdata;
}

// TODO convert to use xmlui file
void KuickShow::initGUI( const KUrl& startDir )
{
	KUrl startURL( startDir );
	if ( !KProtocolManager::supportsListing( startURL ) )
		startURL = KUrl();

    fileWidget = new FileWidget( startDir, this );
    fileWidget->setObjectName( QString::fromLatin1( "MainWidget" ) );
    setFocusProxy( fileWidget );

    KActionCollection *coll = fileWidget->actionCollection();

    redirectDeleteAndTrashActions(coll);

    connect( fileWidget, SIGNAL( fileSelected( const KFileItem& ) ),
             this, SLOT( slotSelected( const KFileItem& ) ));

    connect( fileWidget, SIGNAL( fileHighlighted( const KFileItem& )),
             this, SLOT( slotHighlighted( const KFileItem& ) ));

    connect( fileWidget, SIGNAL( urlEntered( const KUrl&  )),
             this, SLOT( dirSelected( const KUrl& )) );


    fileWidget->setAcceptDrops(true);
    connect( fileWidget, SIGNAL( dropped( const KFileItem&, QDropEvent *, const KUrl::List & )),
             this, SLOT( slotDropped( const KFileItem&, QDropEvent *, const KUrl::List &)) );

    // setup actions
    KAction *open = KStandardAction::open( this, SLOT( slotOpenURL() ),
                                      coll );
    coll->addAction( "openURL", open );

    KAction *print = KStandardAction::print( this, SLOT( slotPrint() ),
                                        coll );
    coll->addAction( "kuick_print", print );
    print->setText( i18n("Print Image...") );

    KAction *configure = coll->addAction( "kuick_configure" );
    configure->setText( i18n("Configure %1...",KGlobal::mainComponent().aboutData()->programName() ) );
    configure->setIcon( KIcon( "configure" ) );
    connect( configure, SIGNAL( triggered() ), this, SLOT( configuration() ) );

    KAction *slide = coll->addAction( "kuick_slideshow" );
    slide->setText( i18n("Start Slideshow" ) );
    slide->setIcon( KIcon("ksslide" ));
    slide->setShortcut( Qt::Key_F2 );
    connect( slide, SIGNAL( triggered() ), this, SLOT( startSlideShow() ));

    KAction *about = coll->addAction( "about" );
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

    KAction *showInOther = coll->addAction( "kuick_showInOtherWindow" );
    showInOther->setText( i18n("Show Image") );
    connect( showInOther, SIGNAL( triggered() ), SLOT( slotShowInOtherWindow() ));

    KAction *showInSame = coll->addAction( "kuick_showInSameWindow" );
    showInSame->setText( i18n("Show Image in Active Window") );
    connect( showInSame, SIGNAL( triggered() ), this, SLOT( slotShowInSameWindow() ) );

    KAction *showFullscreen = coll->addAction( "kuick_showFullscreen" );
    showFullscreen->setText( i18n("Show Image in Fullscreen Mode") );
    connect( showFullscreen, SIGNAL( triggered() ), this, SLOT( slotShowFullscreen() ) );

    KAction *quit = KStandardAction::quit( this, SLOT(slotQuit()), coll);
    coll->addAction( "quit", quit );

    // remove QString::null parameter -- ellis
//    coll->readShortcutSettings( QString::null );

    // menubar
    KMenuBar *mBar = menuBar();
    QMenu *fileMenu = new QMenu( i18n("&File"), mBar );
    fileMenu->setObjectName( QString::fromLatin1( "file" ) );
    fileMenu->addAction(open);
    fileMenu->addAction(showInOther);
    fileMenu->addAction(showInSame);
    fileMenu->addAction(showFullscreen);
    fileMenu->addSeparator();
    fileMenu->addAction(slide);
    fileMenu->addAction(print);
    fileMenu->addSeparator();
    fileMenu->addAction(quit);

    QMenu *editMenu = new QMenu( i18n("&Edit"), mBar );
    editMenu->setObjectName( QString::fromLatin1( "edit" ) );
    editMenu->addAction(coll->action("mkdir"));
    editMenu->addAction(coll->action("delete"));
    editMenu->addSeparator();
    editMenu->addAction(coll->action("properties"));


    // remove the Sorting submenu (and the separator below)
    // from the main contextmenu
    KActionMenu *sortingMenu = static_cast<KActionMenu*>( coll->action("sorting menu"));
    KActionMenu *mainActionMenu = static_cast<KActionMenu*>( coll->action("popupMenu"));

/*
    QMenu *mainPopup = mainActionMenu->menu();
    int sortingIndex = mainPopup->indexOf( sortingMenu->itemId( 0 ) );
    int separatorId = mainPopup->idAt( sortingIndex + 1 );
    QMenuItem *separatorItem = mainPopup->findItem( separatorId );
    if ( separatorItem && separatorItem->isSeparator() )
        mainPopup->removeItem( separatorId );
    mainActionMenu->remove( sortingMenu );
*/

    // add the sorting menu and a separator into the View menu
    KActionMenu *viewActionMenu = static_cast<KActionMenu*>( coll->action("view menu"));
    viewActionMenu->menu()->addSeparator();
    viewActionMenu->menu()->addAction(sortingMenu); //, 0 ); // on top of the menu


    QMenu *settingsMenu = new QMenu( i18n("&Settings"), mBar );
    settingsMenu->setObjectName( QString::fromLatin1( "settings" ) );
    settingsMenu->addAction(configure);

    mBar->addMenu( fileMenu );
    mBar->addMenu( editMenu );
    mBar->addAction( viewActionMenu );
    mBar->addMenu( settingsMenu );

    // toolbar
    KToolBar *tBar = toolBar(i18n("Main Toolbar"));

    tBar->addAction(coll->action("up"));
    tBar->addAction(coll->action("back"));
    tBar->addAction(coll->action("forward"));
    tBar->addAction(coll->action("home"));
    tBar->addAction(coll->action("reload"));

    tBar->addSeparator();

    // Address box in address tool bar
    KToolBar *addressToolBar = toolBar( "address_bar" );
    const int ID_ADDRESSBAR = 1;

    cmbPath = new KUrlComboBox( KUrlComboBox::Directories,
                                true, addressToolBar );
    KUrlCompletion *cmpl = new KUrlCompletion( KUrlCompletion::DirCompletion );
    cmbPath->setCompletionObject( cmpl );
    cmbPath->setAutoDeleteCompletionObject( true );

    addressToolBar->addWidget( cmbPath);
//    addressToolBar->setItemAutoSized( ID_ADDRESSBAR );

    connect( cmbPath, SIGNAL( urlActivated( const KUrl& )),
             this, SLOT( slotSetURL( const KUrl& )));
    connect( cmbPath, SIGNAL( returnPressed()),
             this, SLOT( slotURLComboReturnPressed()));

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
    mBar->addMenu( help );


    KStatusBar* sBar = statusBar();
    sBar->insertItem( "           ", URL_ITEM, 10 );
    sBar->insertItem( "                          ", META_ITEM, 2 );
    sBar->setItemAlignment(URL_ITEM, Qt::AlignVCenter | Qt::AlignLeft);

    fileWidget->setFocus();

    KConfigGroup kc(KGlobal::config(), "SessionSettings");
    bool oneWindow = kc.readEntry("OpenImagesInActiveWindow", true );
    oneWindowAction->setChecked( oneWindow );

    tBar->show();

    fileWidget->initActions();
    fileWidget->clearHistory();
    dirSelected( fileWidget->url() );

    setCentralWidget( fileWidget );

    setupGUI( KXmlGuiWindow::Save );

    qobject_cast<KAction *>(coll->action( "reload" ))->setShortcut( KStandardShortcut::reload() );
    qobject_cast<KAction *>(coll->action( "short view" ))->setShortcut(Qt::Key_F6);
    qobject_cast<KAction *>(coll->action( "detailed view" ))->setShortcut(Qt::Key_F7);
    //qobject_cast<KAction *>(coll->action( "show hidden" ))->setShortcut(Qt::Key_F8);
    qobject_cast<KAction *>(coll->action( "mkdir" ))->setShortcut(Qt::Key_F10);
    qobject_cast<KAction *>(coll->action( "preview" ))->setShortcut(Qt::Key_F11);
    //qobject_cast<KAction *>(coll->action( "separate dirs" ))->setShortcut(Qt::Key_F12);
}

void KuickShow::redirectDeleteAndTrashActions(KActionCollection *coll)
{
    QAction *action = coll->action("delete");
    if (action)
    {
        action->disconnect(fileWidget);
        connect(action, SIGNAL(activated()), this, SLOT(slotDeleteCurrentImage()));
    }

    action = coll->action("trash");
    if (action)
    {
        action->disconnect(fileWidget);
        connect(action, SIGNAL(activated()), this, SLOT(slotTrashCurrentImage()));
    }
}

void KuickShow::slotSetURL( const KUrl& url )
{
    fileWidget->setUrl( url, true );
}

void KuickShow::slotURLComboReturnPressed()
{
    KUrl where( cmbPath->currentText() );
    slotSetURL( where );
}

void KuickShow::viewerDeleted()
{
    ImageWindow *viewer = (ImageWindow*) sender();
    s_viewers.removeAll( viewer );
    if ( viewer == m_viewer )
        m_viewer = 0L;

    if ( !haveBrowser() && s_viewers.isEmpty() ) {
        saveSettings();
        FileCache::shutdown();
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


void KuickShow::slotHighlighted( const KFileItem& item )
{
    QString statusBarInfo = item.isNull() ? QString() : item.getStatusBarInfo();

    statusBar()->changeItem( statusBarInfo, URL_ITEM );
    bool image = FileWidget::isImage( item );

    QString meta;
    if ( image )
    {
        KFileMetaInfo info = item.metaInfo();
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

void KuickShow::slotSelected( const KFileItem& item )
{
    showImage( item, !oneWindowAction->isChecked() );
}

// downloads item if necessary
void KuickShow::showFileItem( ImageWindow * /*view*/,
                              const KFileItem * /*item*/ )
{

}

bool KuickShow::showImage( const KFileItem& fi,
                           bool newWindow, bool fullscreen, bool moveToTopLeft )
{
    newWindow  |= !m_viewer;
    fullscreen |= (newWindow && kdata->fullScreen);
    if ( FileWidget::isImage( fi ) ) {

        if ( newWindow ) {
            m_viewer = new ImageWindow( kdata->idata, id, 0L );
            m_viewer->setObjectName( QString::fromLatin1("image window") );
            m_viewer->setFullscreen( fullscreen );
            s_viewers.append( m_viewer );

	    connect( m_viewer, SIGNAL( nextSlideRequested() ), this, SLOT( nextSlide() ));
            connect( m_viewer, SIGNAL( destroyed() ), SLOT( viewerDeleted() ));
            connect( m_viewer, SIGNAL( sigFocusWindow( ImageWindow *) ),
                     this, SLOT( slotSetActiveViewer( ImageWindow * ) ));
            connect( m_viewer, SIGNAL( sigImageError(const KuickFile *, const QString& ) ),
                     this, SLOT( messageCantLoadImage(const KuickFile *, const QString &) ));
            connect( m_viewer, SIGNAL( requestImage( ImageWindow *, int )),
                     this, SLOT( slotAdvanceImage( ImageWindow *, int )));
	    connect( m_viewer, SIGNAL( pauseSlideShowSignal() ),
		     this, SLOT( pauseSlideShow() ) );
        connect( m_viewer, SIGNAL (deleteImage (ImageWindow *)),
                 this, SLOT (slotDeleteCurrentImage (ImageWindow *)));
        connect( m_viewer, SIGNAL (trashImage (ImageWindow *)),
                 this, SLOT (slotTrashCurrentImage (ImageWindow *)));
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


        if ( !safeViewer->showNextImage( fi.url() ) ) {
            m_viewer = safeViewer;
            delete safeViewer; // couldn't load image, close window
        }
        else {
            // safeViewer->setFullscreen( fullscreen );

            if ( newWindow ) {
//                safeViewer->show();

                if ( !fullscreen && s_viewers.count() == 1 && moveToTopLeft ) {
                    // the WM might have moved us after showing -> strike back!
                    // move the first image to 0x0 workarea coord
                    safeViewer->move( Kuick::workArea().topLeft() );
                }
            }

            if ( kdata->preloadImage && fileWidget ) {
                // don't move cursor
                KFileItem item = fileWidget->getItem( FileWidget::Next, true );
                if ( !item.isNull() )
                    safeViewer->cacheImage( item.url() );
            }

            m_viewer = safeViewer;
            return true;
        } // m_viewer created successfully
    } // isImage

    return false;
}

void KuickShow::slotDeleteCurrentImage()
{
    performDeleteCurrentImage(fileWidget);
}

void KuickShow::slotTrashCurrentImage()
{
    performTrashCurrentImage(fileWidget);
}

void KuickShow::slotDeleteCurrentImage(ImageWindow *viewer)
{
    if (!fileWidget) {
        delayAction(new DelayedRepeatEvent(viewer, DelayedRepeatEvent::DeleteCurrentFile, 0L));
        return;
    }
    performDeleteCurrentImage(viewer);
}

void KuickShow::slotTrashCurrentImage(ImageWindow *viewer)
{
    if (!fileWidget) {
        delayAction(new DelayedRepeatEvent(viewer, DelayedRepeatEvent::TrashCurrentFile, 0L));
        return;
    }
    performTrashCurrentImage(viewer);
}

void KuickShow::performDeleteCurrentImage(QWidget *parent)
{
    assert(fileWidget != 0L);

    KFileItemList list;
    const KFileItem &item = fileWidget->getCurrentItem(false);
    list.append (item);

    if (KMessageBox::warningContinueCancel(
            parent,
            i18n("<qt>Do you really want to delete\n <b>'%1'</b>?</qt>").arg(item.url().pathOrUrl()),
            i18n("Delete File"),
            KStandardGuiItem::del(),
            KStandardGuiItem::cancel(),
            "Kuick_delete_current_image")
        != KMessageBox::Continue)
    {
        return;
    }

    tryShowNextImage();
    fileWidget->del(list, false, false);
}

void KuickShow::performTrashCurrentImage(QWidget *parent)
{
    assert(fileWidget != 0L);

    KFileItemList list;
    const KFileItem& item = fileWidget->getCurrentItem(false);
    if (item.isNull()) return;

    list.append (item);

    if (KMessageBox::warningContinueCancel(
            parent,
            i18n("<qt>Do you really want to trash\n <b>'%1'</b>?</qt>").arg(item.url().pathOrUrl()),
            i18n("Trash File"),
            KGuiItem(i18nc("to trash", "&Trash"),"edittrash"),
            KStandardGuiItem::cancel(),
            "Kuick_trash_current_image")
        != KMessageBox::Continue)
    {
        return;
    }

    tryShowNextImage();
    fileWidget->trash(list, parent, false, false);
}

void KuickShow::tryShowNextImage()
{
    // move to next file item even if we have no viewer
    KFileItem next = fileWidget->getNext(true);
    if (next.isNull())
        next = fileWidget->getPrevious(true);

    // ### why is this necessary at all? Why does KDirOperator suddenly re-read the
    // entire directory after a file was deleted/trashed!? (KDirNotify is the reason)
    if (!m_viewer)
        return;

        if (!next.isNull())
        showImage(next, false);
    else
    {
        if (!haveBrowser())
        {
            // ### when simply calling toggleBrowser(), this main window is completely messed up
            QTimer::singleShot(0, this, SLOT(toggleBrowser()));
        }
        m_viewer->deleteLater();
    }
}

void KuickShow::startSlideShow()
{
    KFileItem item = kdata->slideshowStartAtFirst ?
                      fileWidget->gotoFirstImage() :
                      fileWidget->getCurrentItem(false);

    if ( !item.isNull() ) {
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

    KFileItem item = fileWidget->getNext( true );
    if ( item.isNull() ) { // last image
        if ( m_slideshowCycle < kdata->slideshowCycles
             || kdata->slideshowCycles == 0 ) {
            item = fileWidget->gotoFirstImage();
            if ( !item.isNull() ) {
                nextSlide( item );
                m_slideshowCycle++;
                return;
            }
        }

        delete m_viewer;
        fileWidget->actionCollection()->action("kuick_slideshow")->setEnabled( true );
        return;
    }

    nextSlide( item );
}

void KuickShow::nextSlide( const KFileItem& item )
{
    m_viewer->showNextImage( item.url() );
    if(kdata->slideDelay)
        m_slideTimer->start( kdata->slideDelay );
}


// prints the selected files in the filebrowser
void KuickShow::slotPrint()
{
    const KFileItemList items = fileWidget->selectedItems();
    if ( !items.isEmpty() )
        return;

	KFileItemList::const_iterator it = items.constBegin();
	const KFileItemList::const_iterator end = items.constEnd();

    // don't show the image, just print
    ImageWindow *iw = new ImageWindow( 0, id, this );
    iw->setObjectName( QString::fromLatin1("printing image"));
    KFileItem item;
	for ( ; it != end; ++it ) {
		item = (*it);
        if (FileWidget::isImage( item ) && iw->loadImage( item.url()))
            iw->printImage();
    }

    delete iw;
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

void KuickShow::slotDropped( const KFileItem&, QDropEvent *, const KUrl::List &urls)
{
    KUrl::List::ConstIterator it = urls.constBegin();
    for ( ; it != urls.constEnd(); ++it )
    {
        KFileItem item( KFileItem::Unknown, KFileItem::Unknown, *it );
        if ( FileWidget::isImage( item ) )
            showImage( item, true );
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
    KFileItem item; // to be shown
    KFileItem item_next; // to be cached

    if ( steps == 0 )
        return;

    // the viewer might not be available yet. Factor this out somewhen.
    if ( !fileWidget ) {
        if ( m_delayedRepeatItem )
            return;

        delayAction(new DelayedRepeatEvent( view, DelayedRepeatEvent::AdvanceViewer, new int(steps) ));
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
//        QString filename;
//        KIO::NetAccess::download(item->url(), filename, this);
        view->showNextImage( item.url() );
        if (m_slideTimer->isActive() && kdata->slideDelay)
            m_slideTimer->start( kdata->slideDelay );

        if ( kdata->preloadImage && !item_next.isNull() ) // preload next image
            if ( FileWidget::isImage( item_next ) )
                view->cacheImage( item_next.url() );
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
            FileCache::shutdown();
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
        KFileItem item;      // the image to be shown
        KFileItem item_next; // the image to be cached

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
                    KuickFile *file = m_viewer->currentFile();
                    initGUI( file->url().upUrl() );

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
                    if ( fileWidget->dirLister()->isFinished() )
                    {
                        if ( !fileWidget->dirLister()->rootItem().isNull() )
                        {
                        	fileWidget->setCurrentItem( file->url().fileName() );
                        	QTimer::singleShot( 0, this, SLOT( slotReplayEvent()));
                        }
                        else // finished, but no root-item -- probably an error, kill repeat-item!
                        {
                        	abortDelayedEvent();
                        }
                }
                else // not finished yet
                {
                        fileWidget->setInitialItem( file->url() );
                        connect( fileWidget, SIGNAL( finished() ),
                                 SLOT( slotReplayEvent() ));
                    }

                    return true;
                }

                return KXmlGuiWindow::eventFilter( o, e );
            }

            // we definitely have a fileWidget here!

            if ( key == Qt::Key_Home || KStandardShortcut::begin().contains( k->key() ) )
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
                if ( item.isNull() )
                    item = fileWidget->getPrevious( false );
                KFileItem it( KFileItem::Unknown, KFileItem::Unknown,
                              m_viewer->url() );
                KFileItemList list;
                list.append( it );
                if ( fileWidget->del(list, window,
                                     (k->modifiers() & Qt::ShiftModifier) == 0) == 0L )
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
            {
                ret = false;
            }

            if ( FileWidget::isImage( item ) ) {
                m_viewer->showNextImage( item.url() );

                if ( kdata->preloadImage && !item_next.isNull() ) // preload next image
                    if ( FileWidget::isImage( item_next ) )
                        m_viewer->cacheImage( item_next.url() );

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
                        initGUI( window->currentFile()->url().fileName() );
                    }
                    show();
                    raise();
                }

                delete window;

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
    if ( !fileWidget ) {
        KUrl start;
        start.setPath( QDir::homePath() );
        initGUI( start );
    }

    dialog = new KuickConfigDialog( fileWidget->actionCollection(), 0L, false );
    dialog->setObjectName(QString::fromLatin1("dialog"));
    dialog->setWindowIcon( qApp->windowIcon() );

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
    QList<ImageWindow*>::ConstIterator it = s_viewers.constBegin();
    while ( it != s_viewers.constEnd() ) {
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
    if ( !aboutWidget ) {
        aboutWidget = new AboutWidget( 0L );
        aboutWidget->setObjectName( QString::fromLatin1( "about" ) );
    }

    aboutWidget->adjustSize();

    KDialog::centerOnScreen( aboutWidget );

    aboutWidget->show();
}

// ------ sessionmanagement - load / save current directory -----
void KuickShow::readProperties( const KConfigGroup& kc )
{
    assert( fileWidget ); // from SM, we should always have initGUI on startup
    QString dir = kc.readPathEntry( "CurrentDirectory", QString() );
    if ( !dir.isEmpty() ) {
        fileWidget->setUrl( KUrl( dir ), true );
        fileWidget->clearHistory();
    }

    const KUrl& listedURL = fileWidget->url();
    const QStringList images = kc.readPathEntry( "Images shown", QStringList() );
    QStringList::const_iterator it;
    bool hasCurrentURL = false;

    for ( it = images.constBegin(); it != images.constEnd(); ++it ) {
        KFileItem item( KFileItem::Unknown, KFileItem::Unknown, KUrl( *it ), false );
        if ( item.isReadable() ) {
            if (showImage( item, true )) {
				// Set the current URL in the file widget, if possible
				if ( !hasCurrentURL && listedURL.isParentOf( item.url() )) {
					fileWidget->setInitialItem( item.url() );
					hasCurrentURL = true;
				}
            }
		}
    }

    if ( !s_viewers.isEmpty() ) {
        bool visible = kc.readEntry( "Browser visible", true );
        if ( !visible )
            hide();
    }
}

void KuickShow::saveProperties( KConfigGroup& kc )
{
    kc.writeEntry( "Browser visible", fileWidget && fileWidget->isVisible() );
    if (fileWidget)
    kc.writePathEntry( "CurrentDirectory", fileWidget->url().url() );

    QStringList urls;
    QList<ImageWindow*>::ConstIterator it;
    for ( it = s_viewers.constBegin(); it != s_viewers.constEnd(); ++it ) {
        const KUrl& url = (*it)->currentFile()->url();
        if ( url.isLocalFile() )
            urls.append( url.path() );
        else
            urls.append( url.prettyUrl() ); // ### check if writePathEntry( prettyUrl ) works!
    }

    kc.writePathEntry( "Images shown", urls );
}

// --------------------------------------------------------------

void KuickShow::saveSettings()
{
    KSharedConfig::Ptr kc = KGlobal::config();
    KConfigGroup sessGroup(kc, "SessionSettings");
    if ( oneWindowAction )
        sessGroup.writeEntry( "OpenImagesInActiveWindow", oneWindowAction->isChecked() );

    if ( fileWidget ) {
        sessGroup.writePathEntry( "CurrentDirectory", fileWidget->url().prettyUrl() );
        KConfigGroup group( kc, "Filebrowser" );
        fileWidget->writeConfig( group);
    }

    kc->sync();
}


void KuickShow::messageCantLoadImage( const KuickFile *, const QString& message )
{
    m_viewer->clearFocus();
    KMessageBox::sorry( m_viewer, message, i18n("Image Error") );
}

void KuickShow::initImlib()
{
    ImData *idata = kdata->idata;
    ImlibInitParams par;
    initImlibParams( idata, &par );

    id = Imlib_init_with_params( getX11Display(), &par );
    if ( !id ) {
        initImlibParams( idata, &par );

        qWarning("*** KuickShow: Whoops, can't initialize imlib, trying my own palettefile now.");
        QString paletteFile = KStandardDirs::locate( "data", "kuickshow/im_palette.pal" );
        // FIXME - does the qstrdup() cure the segfault in imlib eventually?
        char *file = qstrdup( paletteFile.toLocal8Bit() );
        par.palettefile = file;
        par.flags |= PARAMS_PALETTEFILE;

        qWarning("Palettefile: %s", par.palettefile );

        id = Imlib_init_with_params( getX11Display(), &par );

        if ( !id ) {
            QString tmp = i18n("Unable to initialize \"Imlib\".\n"
                               "Start kuickshow from the command line "
                               "and look for error messages.\n"
                               "The program will now quit.");
            KMessageBox::error( this, tmp, i18n("Fatal Imlib Error") );

            FileCache::shutdown();
            ::exit(1);
        }
    }
}


void KuickShow::initImlibParams( ImData *idata, ImlibInitParams *par )
{
    par->flags = ( PARAMS_REMAP | PARAMS_VISUALID | PARAMS_SHAREDMEM | PARAMS_SHAREDPIXMAPS |
                   PARAMS_FASTRENDER | PARAMS_HIQUALITY | PARAMS_DITHER |
                   PARAMS_IMAGECACHESIZE | PARAMS_PIXMAPCACHESIZE );

    Visual* defaultvis = DefaultVisual(getX11Display(), x11Info().screen());

    par->paletteoverride = idata->ownPalette  ? 1 : 0;
    par->remap           = idata->fastRemap   ? 1 : 0;
    par->fastrender      = idata->fastRender  ? 1 : 0;
    par->hiquality       = idata->dither16bit ? 1 : 0;
    par->dither          = idata->dither8bit  ? 1 : 0;
    par->sharedmem       = 1;
    par->sharedpixmaps   = 1;
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

void KuickShow::abortDelayedEvent()
{
    delete m_delayedRepeatItem;
    m_delayedRepeatItem = 0L;
}

void KuickShow::slotReplayEvent()
{
    disconnect( fileWidget, SIGNAL( finished() ),
                this, SLOT( slotReplayEvent() ));

    DelayedRepeatEvent *e = m_delayedRepeatItem;
    m_delayedRepeatItem = 0L; // otherwise, eventFilter aborts

    eventFilter( e->viewer, e->event );
    delete e;
    // --------------------------------------------------------------
}

void KuickShow::replayAdvance(DelayedRepeatEvent *event)
{
#if 0
    // ### WORKAROUND for QIconView bug in Qt <= 3.0.3 at least
    // Sigh. According to qt-bugs, they won't fix this bug ever. So you can't
    // rely on sorting to be correct before the QIconView has been show()n.
    if ( fileWidget && fileWidget->view() ) {
        QWidget *widget = fileWidget->view()->widget();
        if ( widget->inherits( "QIconView" ) || widget->child(0, "QIconView" ) ){
            fileWidget->setSorting( fileWidget->sorting() );
        }
    }
#endif
    // --------------------------------------------------------------
    slotAdvanceImage( event->viewer, *(int *) (event->data) );
}

void KuickShow::delayAction(DelayedRepeatEvent *event)
{
    if (m_delayedRepeatItem)
        return;

    m_delayedRepeatItem = event;

    KUrl url = event->viewer->currentFile()->url();
//    QFileInfo fi( event->viewer->filename() );
//    start.setPath( fi.dirPath( true ) );
    initGUI( url.upUrl() );

    // see eventFilter() for explanation and similar code
    if ( fileWidget->dirLister()->isFinished() &&
         !fileWidget->dirLister()->rootItem().isNull() )
    {
        fileWidget->setCurrentItem( url.fileName() );
        QTimer::singleShot( 0, this, SLOT( doReplay()));
    }
    else
    {
        fileWidget->setInitialItem( url );
        connect( fileWidget, SIGNAL( finished() ),
                 SLOT( doReplay() ));
    }
}

void KuickShow::doReplay()
{
    if (!m_delayedRepeatItem)
        return;

    disconnect( fileWidget, SIGNAL( finished() ),
                this, SLOT( doReplay() ));

    switch (m_delayedRepeatItem->action)
    {
        case DelayedRepeatEvent::DeleteCurrentFile:
            performDeleteCurrentImage((QWidget *) m_delayedRepeatItem->data);
            break;
        case DelayedRepeatEvent::TrashCurrentFile:
            performTrashCurrentImage((QWidget *) m_delayedRepeatItem->data);
            break;
        case DelayedRepeatEvent::AdvanceViewer:
            replayAdvance(m_delayedRepeatItem);
            break;
        default:
            kWarning() << "doReplay: unknown action -- ignoring: " << m_delayedRepeatItem->action << endl;
            break;
    }

    delete m_delayedRepeatItem;
    m_delayedRepeatItem = 0L;
}

void KuickShow::toggleBrowser()
{
    if ( !haveBrowser() ) {
        if ( m_viewer && m_viewer->isFullscreen() )
            m_viewer->setFullscreen( false );
        fileWidget->resize( size() ); // ### somehow fileWidget isn't resized!?
        show();
        raise();
        KWindowSystem::activateWindow( winId() ); // ### this should not be necessary
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
        KUrl::List::ConstIterator it = urls.constBegin();
        for ( ; it != urls.constEnd(); ++it )
        {
            KFileItem item( KFileItem::Unknown, KFileItem::Unknown, *it );
            if ( FileWidget::isImage( item ) )
                showImage( item, true );
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
        delete (*it);
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
