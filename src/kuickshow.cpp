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

#include "kuickshow.h"

#include <KAboutData>
#include <KActionCollection>
#include <KActionMenu>
#include <KConfigGroup>
#include <KCursor>
#include <KDirLister>
#include <KHelpMenu>
#include <KIconLoader>
#include <KIO/MimetypeJob>
#include <KIO/DeleteOrTrashJob>
#include <KIO/AskUserActionInterface>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KProtocolManager>
#include <KSharedConfig>
#include <KStandardAction>
#include <KStandardGuiItem>
#include <KStandardShortcut>
#include <KStartupInfo>
#include <KToolBar>
#include <KUrlComboBox>
#include <KUrlCompletion>

#include <QAbstractItemView>
#include <QGuiApplication>
#include <QCommandLineParser>
#include <QDialog>
#include <QDir>
#include <QDropEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMimeDatabase>
#include <QMouseEvent>
#include <QSize>
#include <QStandardPaths>
#include <QStatusBar>
#include <QString>
#include <QScreen>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QtGlobal>

#include "aboutwidget.h"
#include "filecache.h"
#include "filewidget.h"
#include "imagewindow.h"
#include "imlib.h"
#include "imlibwidget.h"
#include "kuick.h"
#include "kuickconfig.h"
#include "kuickconfigdlg.h"
#include "kuickfile.h"
#include "kuickshow_debug.h"
#include "openfilesanddirsdialog.h"


Q_DECLARE_OPERATORS_FOR_FLAGS(KuickShow::ShowFlags)


// TODO: why is this static?  A member would be more sensible
// (although there never exists more than one KuickShow).
static QList<ImageWindow *> s_viewers;


// -----------------------------------------------------------------

class DelayedRepeatEvent
{
public:
    DelayedRepeatEvent( ImageWindow *view, QKeyEvent *ev ) {
        viewer = view;
        event  = ev;
    }
    DelayedRepeatEvent( ImageWindow *view, int act, void *d ) {
        this->viewer = view;
        this->action = act;
        this->data   = d;
        this->event  = nullptr;
    }

    ~DelayedRepeatEvent() {
        delete event;
    }

    enum Action
    {
        DeleteCurrentFile,
        TrashCurrentFile,
        AdvanceViewer
    };

    ImageWindow *viewer;
    QKeyEvent *event;
    int action;
    void *data;
};

// -----------------------------------------------------------------


KuickShow::KuickShow( const char *objName )
    : KXmlGuiWindow(nullptr),
      m_slideshowCycle( 1 ),
      fileWidget(nullptr),
      dialog(nullptr),
      m_viewer(nullptr),
      m_delayedRepeatItem(nullptr),
      m_slideShowStopped(false)
{
    setObjectName(objName);
    aboutWidget = nullptr;

    KuickConfig& config = KuickConfig::get();
    config.load();

    // This will report the build time version, there appears to
    // be no way to obtain the run time Imlib version.
    const auto imlib = ImageLibrary::get();
    qDebug("Compiled with image library: %s %s", qPrintable(imlib->getLibraryName()), qPrintable(imlib->getLibraryVersion()));

    if (!initImlib()) return;

    resize( 400, 500 );

    m_slideTimer = new QTimer( this );
    connect(m_slideTimer, &QTimer::timeout, this, QOverload<>::of(&KuickShow::nextSlide));

    KSharedConfig::Ptr kc = KSharedConfig::openConfig();

    bool isDir = false; // true if we get a directory on the commandline

    // parse commandline options
    QCommandLineParser *parser = static_cast<QCommandLineParser *>(qApp->property("cmdlineParser").value<void *>());

    // files to display
    // either a directory to display, an absolute path, a relative path, or a URL
    QUrl startDir = QUrl::fromLocalFile(QDir::currentPath() + '/');

    QStringList args = parser->positionalArguments();
    int numArgs = args.count();
    if ( numArgs >= 10 )
    {
        // Even though the 1st i18n string will never be used, it needs to exist for plural handling - mhunter
        if (KMessageBox::questionTwoActions(
                this,
                xi18ncp("@info",
                        "Do you really want to display this 1 image at the same time? This might be quite resource intensive and could overload your computer.<br/>If you choose <interface>%2</interface>, only the first image will be shown.",
                        "Do you really want to display these %1 images at the same time? This might be quite resource intensive and could overload your computer.<br/>If you choose <interface>%2</interface>, only the first image will be shown.", numArgs, KStandardGuiItem::cancel().plainText()),
                i18n("Display Multiple Images?"),
                KGuiItem(i18n("Display"), KStandardGuiItem::guiItem(KStandardGuiItem::Ok).icon()),
                KStandardGuiItem::cancel()) != KMessageBox::PrimaryAction)
        {
            numArgs = 1;
        }
    }

    QMimeDatabase mimedb;
    for ( int i = 0; i < numArgs; i++ ) {
        QUrl url = QUrl::fromUserInput(args.value(i), QDir::currentPath(), QUrl::AssumeLocalFile);
        KFileItem item( url );

        // for remote URLs, we don't know if it's a file or directory, but
        // FileWidget::isImage() should correct in most cases.
        // For non-local non-images, we just assume directory.

        if ( FileWidget::isImage( item ) )
        {
            // show in new window, not fullscreen-forced, and move to 0,0
            showImage(item, NewWindow);
        }
        else if ( item.isDir() )
        {
            startDir = url;
            isDir = true;
        }

        // need to check remote files
        else if ( !url.isLocalFile() )
        {
            QMimeType mime = mimedb.mimeTypeForUrl( url );
            QString name = mime.name();
            if ( name == "application/octet-stream" ) { // unknown -> stat()
                KIO::MimetypeJob* job = KIO::mimetype(url);
                KJobWidgets::setWindow(job, this);
                connect(job, &KIO::MimetypeJob::result, [job, &name]()
                {
                    if (!job->error()) name = job->mimetype();
                });
                job->exec();
            }

	    // text/* is a hack for bugs.kde.org-attached-images urls.
	    // The real problem here is that NetAccess::mimetype does a HTTP HEAD, which doesn't
	    // always return the right mimetype. The rest of KDE start a get() instead....
            if ( name.startsWith( "image/" ) || name.startsWith( "text/" ) )
            {
                showImage(item, NewWindow|IgnoreFileType);
            }
            else // assume directory, KDirLister will tell us if we can't list
            {
                startDir = url;
                isDir = true;
            }
        }
        // else // we don't handle local non-images
    }

    if ( (config.startInLastDir && args.count() == 0) || parser->isSet( "lastfolder" )) {
        KConfigGroup sessGroup(kc, "SessionSettings");
        startDir = QUrl(sessGroup.readPathEntry( "CurrentDirectory", startDir.url() ));
    }

    if ( s_viewers.isEmpty() || isDir ) {
        initGUI( startDir );
	if (!qApp->isSessionRestored()) // during session management, readProperties() will show()
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
    if (m_viewer!=nullptr) m_viewer->deleteLater();
    FileCache::shutdown();
}

// TODO convert to use xmlui file
void KuickShow::initGUI( const QUrl& startDir )
{
    QUrl startURL( startDir );
	if ( !KProtocolManager::supportsListing( startURL ) )
		startURL = QUrl();

    fileWidget = new FileWidget( startDir, this );
    fileWidget->setObjectName( QString::fromLatin1( "MainWidget" ) );
    setFocusProxy( fileWidget );

	setupKuickActions();
	redirectDeleteAndTrashActions();

    fileWidget->setAcceptDrops(true);
    connect(fileWidget, &KDirOperator::fileSelected, this, &KuickShow::slotSelected);
    connect(fileWidget, &KDirOperator::fileHighlighted, this, &KuickShow::slotHighlighted);
    connect(fileWidget, &KDirOperator::urlEntered, this, &KuickShow::dirSelected);
    connect(fileWidget, &KDirOperator::dropped, this, &KuickShow::slotDropped);

    // Provided by KDirOperator, but no icon as standard
	if(auto action = fileWidget->action(KDirOperator::ShowPreviewPanel))
		action->setIcon(QIcon::fromTheme("document-preview"));

    // menubar
    QMenuBar *mBar = menuBar();

    QMenu *fileMenu = new QMenu( i18n("&File"), mBar );
    fileMenu->setObjectName( QString::fromLatin1( "file" ) );
    fileMenu->addAction(kuickAction(KuickActionType::OpenUrl));
    fileMenu->addAction(fileWidget->action(KDirOperator::NewFolder));
    fileMenu->addAction(fileWidget->action(KDirOperator::Trash));
    fileMenu->addSeparator();
    fileMenu->addAction(kuickAction(KuickActionType::ShowImageInNewWindow));
    fileMenu->addAction(kuickAction(KuickActionType::ShowImageInActiveWindow));
    fileMenu->addAction(kuickAction(KuickActionType::ShowImageFullScreen));
    fileMenu->addAction(kuickAction(KuickActionType::SlideShow));
    fileMenu->addAction(kuickAction(KuickActionType::PrintImage));
    fileMenu->addSeparator();
    fileMenu->addAction(kuickAction(KuickActionType::Quit));

    QMenu *editMenu = new QMenu( i18n("&Edit"), mBar );
    editMenu->setObjectName( QString::fromLatin1( "edit" ) );
    editMenu->addAction(fileWidget->action(KDirOperator::Properties));

    KActionMenu* viewActionMenu = dynamic_cast<KActionMenu*>(fileWidget->action(KDirOperator::ViewModeMenu));
    // Ensure that the menu bar shows "View"
    viewActionMenu->setText(i18n("View"));
    viewActionMenu->setIcon(QIcon());

    QMenu *settingsMenu = new QMenu( i18n("&Settings"), mBar );
    settingsMenu->setObjectName( QString::fromLatin1( "settings" ) );
    settingsMenu->addAction(kuickAction(KuickActionType::OneImageWindow));
    settingsMenu->addSeparator();
    settingsMenu->addAction(kuickAction(KuickActionType::Configure));

    mBar->addMenu( fileMenu );
    mBar->addMenu( editMenu );
    mBar->addAction( viewActionMenu );
    mBar->addMenu( settingsMenu );
    mBar->addSeparator();

    KHelpMenu* help = new KHelpMenu(this, QString(), false);
    mBar->addMenu(help->menu());

    // toolbar
    KToolBar *tBar = toolBar(i18n("Main Toolbar"));
    tBar->addAction(fileWidget->action(KDirOperator::Up));
    tBar->addAction(fileWidget->action(KDirOperator::Back));
    tBar->addAction(fileWidget->action(KDirOperator::Forward));
    tBar->addAction(fileWidget->action(KDirOperator::Home));
    tBar->addAction(fileWidget->action(KDirOperator::Reload));
    tBar->addSeparator();
    tBar->addAction(fileWidget->action(KDirOperator::ShortView));
    tBar->addAction(fileWidget->action(KDirOperator::DetailedView));
    tBar->addAction(kuickAction(KuickActionType::InlinePreview));
    tBar->addAction(fileWidget->action(KDirOperator::ShowPreviewPanel));
    tBar->addSeparator();
    tBar->addAction(kuickAction(KuickActionType::SlideShow));
    tBar->addSeparator();
    tBar->addAction(kuickAction(KuickActionType::OneImageWindow));
    tBar->addAction(kuickAction(KuickActionType::PrintImage));
    tBar->addSeparator();
    tBar->addAction(kuickAction(KuickActionType::About));

    // Address box in address tool bar
    KToolBar *addressToolBar = toolBar( "address_bar" );
    cmbPath = new KUrlComboBox( KUrlComboBox::Directories,
                                true, addressToolBar );
    KUrlCompletion *cmpl = new KUrlCompletion( KUrlCompletion::DirCompletion );
    cmbPath->setCompletionObject( cmpl );
    cmbPath->setAutoDeleteCompletionObject( true );
    connect(cmbPath, &KUrlComboBox::urlActivated, this, &KuickShow::slotSetURL);
    connect(cmbPath, QOverload<const QString &>::of(&KComboBox::returnPressed), this, &KuickShow::slotURLComboReturnPressed);
    addressToolBar->addWidget( cmbPath );

    sblblUrlInfo = createStatusBarLabel(10);
    sblblMetaInfo = createStatusBarLabel(2);

    fileWidget->setFocus();

    KConfigGroup kc(KSharedConfig::openConfig(), "SessionSettings");
    bool oneWindow = kc.readEntry("OpenImagesInActiveWindow", true );
    kuickAction(KuickActionType::OneImageWindow)->setChecked(oneWindow);

    tBar->show();

    fileWidget->clearHistory();
    dirSelected( fileWidget->url() );

    setCentralWidget( fileWidget );

    setupGUI( KXmlGuiWindow::Save );
}

QLabel* KuickShow::createStatusBarLabel(int stretch)
{
    QLabel* label = new QLabel(this);
    label->setFixedHeight(fontMetrics().height() + 2);  // copied from KStatusBar::insertItem
    statusBar()->addWidget(label, stretch);
    return label;
}

void KuickShow::redirectDeleteAndTrashActions()
{
    if(auto action = fileWidget->action(KDirOperator::Delete)) {
        action->disconnect(fileWidget);
        connect(action, &QAction::triggered, this, QOverload<>::of(&KuickShow::slotDeleteCurrentImage));
    }

    if(auto action = fileWidget->action(KDirOperator::Trash)) {
        action->disconnect(fileWidget);
        connect(action, &QAction::triggered, this, QOverload<>::of(&KuickShow::slotTrashCurrentImage));
    }
}

void KuickShow::slotSetURL( const QUrl& url )
{
    fileWidget->setUrl( url, true );
}

void KuickShow::slotURLComboReturnPressed()
{
    QUrl where = QUrl::fromUserInput( cmbPath->currentText(), QString(), QUrl::AssumeLocalFile );
    slotSetURL( where );
}

void KuickShow::viewerDeleted()
{
    // Do not use qobject_cast here, it will return NULL
    ImageWindow *viewer = static_cast<ImageWindow *>(sender());
    if (viewer==nullptr) return;

    s_viewers.removeAll( viewer );
    if ( viewer == m_viewer )
        m_viewer = nullptr;

    if ( !haveBrowser() && s_viewers.isEmpty() ) {
        QCoreApplication::quit();
        deleteLater();
    }

    else if ( haveBrowser() ) {
        activateWindow();
        // This setFocus() call causes problems in the combiview (always the
        // directory view on the left gets the focus, which is not desired)
        // fileWidget->setFocus();
    }

    if ( fileWidget )
        // maybe a slideshow was stopped --> enable the action again
        kuickAction(KuickActionType::SlideShow)->setEnabled(true);

    m_slideTimer->stop();
}


void KuickShow::slotHighlighted( const KFileItem& item )
{
    QString statusBarInfo = item.isNull() ? QString() : item.getStatusBarInfo();

    sblblUrlInfo->setText(statusBarInfo);
    bool image = FileWidget::isImage( item );

    QString meta;
    if ( image )
    {
        // TODO: find alternative to KFileMetaInfo (disappered in KF5)
    }
    sblblMetaInfo->setText(meta);

	kuickAction(KuickActionType::PrintImage)->setEnabled(image);
	kuickAction(KuickActionType::ShowImageInActiveWindow)->setEnabled(image);
	kuickAction(KuickActionType::ShowImageInNewWindow)->setEnabled(image);
	kuickAction(KuickActionType::ShowImageFullScreen)->setEnabled(image);
}

void KuickShow::dirSelected( const QUrl& url )
{
    if ( url.isLocalFile() )
        setCaption( url.path() );
    else
        setCaption( url.toDisplayString() );

    cmbPath->setUrl( url );
    sblblUrlInfo->setText(url.toDisplayString());
}

void KuickShow::slotSelected( const KFileItem& item )
{
    showImage(item, kuickAction(KuickActionType::OneImageWindow)->isChecked() ? ShowDefault : NewWindow);
}

// downloads item if necessary
void KuickShow::showFileItem( ImageWindow * /*view*/,
                              const KFileItem * /*item*/ )
{

}

void KuickShow::slotShowWithUrl(const QUrl &url)
{
    QUrl u = url.adjusted(QUrl::RemoveFilename);
    if (fileWidget==nullptr) initGUI(u);
    else slotSetURL(u);
    show();
    raise();
}


// TODO: this is never called without NoMoveToTopLeft, but it is not clear why
// always moving the window to (0,0) is good from a UI point of view.  Perhaps a
// better window placement strategy would be:
//
// Opening a new window: accept the window manager's placement.
//
// Reusing an existing window: centre the new window at the same place as
// the centre of the old one, if allowed by its size and screen bounds.

bool KuickShow::showImage(const KFileItem &fi, KuickShow::ShowFlags flags)
{
    const KuickConfig& config = KuickConfig::get();
    if (m_viewer==nullptr) flags |= NewWindow;
    if ((flags & NewWindow) && config.fullScreen) flags |= FullScreen;
    if ((flags & IgnoreFileType) || FileWidget::isImage(fi))
    {
        if (flags & NewWindow)
        {
            m_viewer = new ImageWindow(nullptr);
            m_viewer->setObjectName( QString::fromLatin1("image window") );
            m_viewer->setFullscreen(flags & FullScreen);
            s_viewers.append( m_viewer );

	    connect(m_viewer, &ImageWindow::nextSlideRequested, this, QOverload<>::of(&KuickShow::nextSlide));
	    connect(m_viewer, &ImageWindow::duplicateWindow, this, &KuickShow::slotDuplicateWindow);
            connect(m_viewer, &QObject::destroyed, this, &KuickShow:: viewerDeleted);
            connect(m_viewer, &ImageWindow::sigFocusWindow, this, &KuickShow::slotSetActiveViewer);
            connect(m_viewer, &ImlibWidget::sigImageError, this, &KuickShow::messageCantLoadImage);
            connect(m_viewer, &ImageWindow::requestImage, this, &KuickShow:: slotAdvanceImage);
	    connect(m_viewer, &ImageWindow::pauseSlideShowSignal, this, &KuickShow::pauseSlideShow);
            connect(m_viewer, &ImageWindow::deleteImage, this, QOverload<ImageWindow *>::of(&KuickShow::slotDeleteCurrentImage));
            connect(m_viewer, &ImageWindow::trashImage, this, QOverload<ImageWindow *>::of(&KuickShow::slotTrashCurrentImage));
            connect(m_viewer, &ImageWindow::showFileBrowser, this, &KuickShow::slotShowWithUrl);

            if ( s_viewers.count() == 1 && !(flags & NoMoveToTopLeft))
            {
                // we have to move to 0x0 before showing _and_
                // after showing, otherwise we get some bogus geometry()
                m_viewer->safeMoveWindow( Kuick::workArea().topLeft() );
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
            safeViewer->deleteLater(); // couldn't load image, close window
        }
        else {
            // safeViewer->setFullscreen( fullscreen );
            if (flags & NewWindow)
            {
                if (!(flags & FullScreen) && s_viewers.count()==1 && !(flags & NoMoveToTopLeft))
                {
                    // the WM might have moved us after showing -> strike back!
                    // move the first image to 0x0 workarea coord
                    safeViewer->safeMoveWindow( Kuick::workArea().topLeft() );
                }
            }

            if ( config.preloadImage && fileWidget!=nullptr) {
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
        delayAction(new DelayedRepeatEvent(viewer, DelayedRepeatEvent::DeleteCurrentFile, nullptr));
        return;
    }
    performDeleteCurrentImage(viewer);
}

void KuickShow::slotTrashCurrentImage(ImageWindow *viewer)
{
    if (!fileWidget) {
        delayAction(new DelayedRepeatEvent(viewer, DelayedRepeatEvent::TrashCurrentFile, nullptr));
        return;
    }
    performTrashCurrentImage(viewer);
}

void KuickShow::performDeleteCurrentImage(QWidget *parent)
{
    Q_ASSERT(fileWidget != nullptr);

    const QList<QUrl> urls = { fileWidget->getCurrentItem(false).url() };

    using Iface = KIO::AskUserActionInterface;
    auto deleteJob = new KIO::DeleteOrTrashJob(urls, Iface::Delete, Iface::DefaultConfirmation, parent);
    connect(deleteJob, &KIO::DeleteOrTrashJob::finished, this, [this, deleteJob](KJob *) {
        if (deleteJob->error() != KJob::NoError) {
            return;
        }

        tryShowNextImage();
    });
    deleteJob->start();
}

void KuickShow::performTrashCurrentImage(QWidget *parent)
{
    Q_ASSERT(fileWidget != nullptr);

    const QList<QUrl> urls = { fileWidget->getCurrentItem(false).url() };

    using Iface = KIO::AskUserActionInterface;
    auto deleteJob = new KIO::DeleteOrTrashJob(urls, Iface::Trash, Iface::DefaultConfirmation, parent);
    connect(deleteJob, &KIO::DeleteOrTrashJob::finished, this, [this, deleteJob](KJob *) {
        if (deleteJob->error() != KJob::NoError) {
            return;
        }

        tryShowNextImage();
    });
    deleteJob->start();
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

    if (!next.isNull()) showImage(next, ShowDefault);
    else
    {
        if (!haveBrowser())
        {
            // ### when simply calling toggleBrowser(), this main window is completely messed up
            QTimer::singleShot(0, this, &KuickShow::toggleBrowser);
        }
        m_viewer->deleteLater();
    }
}

void KuickShow::startSlideShow()
{
    const KuickConfig& config = KuickConfig::get();
    KFileItem item = config.slideshowStartAtFirst ?
                      fileWidget->gotoFirstImage() :
                      fileWidget->getCurrentItem(false);

    if ( !item.isNull() ) {
        m_slideshowCycle = 1;
        kuickAction(KuickActionType::SlideShow)->setEnabled(false);
        showImage(item, (!kuickAction(KuickActionType::OneImageWindow)->isChecked() ? NewWindow : ShowDefault) |
                         (config.slideshowFullscreen ? FullScreen : ShowDefault));
	if(config.slideDelay)
            m_slideTimer->start( config.slideDelay );
    }
}

void KuickShow::pauseSlideShow()
{
    if(m_slideShowStopped) {
        uint slideDelay = KuickConfig::get().slideDelay;
        if(slideDelay)
            m_slideTimer->start( slideDelay );
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
        kuickAction(KuickActionType::SlideShow)->setEnabled(true);
        return;
    }

    KFileItem item = fileWidget->getNext( true );
    if ( item.isNull() ) { // last image
        uint slideshowCycles = KuickConfig::get().slideshowCycles;
        if ( m_slideshowCycle < slideshowCycles || slideshowCycles == 0 ) {
            item = fileWidget->gotoFirstImage();
            if ( !item.isNull() ) {
                nextSlide( item );
                m_slideshowCycle++;
                return;
            }
        }

        delete m_viewer;
        kuickAction(KuickActionType::SlideShow)->setEnabled(true);
        return;
    }

    nextSlide( item );
}

void KuickShow::nextSlide( const KFileItem& item )
{
    m_viewer->showNextImage( item.url() );
    uint slideDelay = KuickConfig::get().slideDelay;
    if(slideDelay)
        m_slideTimer->start( slideDelay );
}


// prints the selected files in the filebrowser
void KuickShow::slotPrint()
{
    const KFileItemList items = fileWidget->selectedItems();
    if (items.isEmpty()) return;

    // don't show the image, just print
    ImageWindow *iw = new ImageWindow(this);
    iw->setObjectName( QString::fromLatin1("printing image"));
    for (const KFileItem &item : std::as_const(items))
    {
        if (FileWidget::isImage(item) && iw->loadImage(item.url())) iw->printImage();
    }

    delete iw;
}

void KuickShow::slotShowInOtherWindow()
{
    showImage(fileWidget->getCurrentItem(false), NewWindow);
}

void KuickShow::slotShowInSameWindow()
{
    showImage(fileWidget->getCurrentItem(false), ShowDefault);
}

void KuickShow::slotShowFullscreen()
{
    showImage(fileWidget->getCurrentItem(false), FullScreen);
}

void KuickShow::slotDropped( const KFileItem&, QDropEvent *, const QList<QUrl> &urls)
{
    for (const QUrl &url : std::as_const(urls))
    {
        KFileItem item(url);
        if (FileWidget::isImage(item)) showImage(item, NewWindow);
        else fileWidget->setUrl(url, true);
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
        // TODO: Urgh, third parameter = memory leak
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
        const KuickConfig& config = KuickConfig::get();
        view->showNextImage( item.url() );
        if (m_slideTimer->isActive() && config.slideDelay)
            m_slideTimer->start( config.slideDelay );

        if ( config.preloadImage && !item_next.isNull() ) // preload next image
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

    QKeyEvent *k = nullptr;
    if (eventType == QEvent::KeyPress) k = static_cast<QKeyEvent *>(e);

    if (k!=nullptr)
    {
        // Forward the key shortcuts for "Quit" and "Handbook" from
        // an image viewer window, to the main window, where they
        // will be actioned.

        QKeySequence seq(k->key()|k->modifiers());
        if (KStandardShortcut::quit().contains(seq))
        {
            deleteAllViewers();
            deleteLater();
            return true;
        }
        else if (KStandardShortcut::help().contains(seq))
        {
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
                    initGUI( KIO::upUrl(file->url()) );

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
                        	fileWidget->setCurrentItem( file->url() );
                        	QTimer::singleShot(0, this, &KuickShow::slotReplayEvent);
                        }
                        else // finished, but no root-item -- probably an error, kill repeat-item!
                        {
                        	abortDelayedEvent();
                        }
                }
                else // not finished yet
                {
                        fileWidget->setInitialItem( file->url() );
                        connect(fileWidget, &FileWidget::finished, this, &KuickShow::slotReplayEvent);
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

            else if (fileWidget->action(KDirOperator::Delete)->shortcuts().contains( key ))
            {
                performDeleteCurrentImage(fileWidget);
            }

            else if (kuickAction(KuickActionType::ToggleBrowser)->shortcuts().contains(key))
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

                if ( KuickConfig::get().preloadImage && !item_next.isNull() ) // preload next image
                    if ( FileWidget::isImage( item_next ) )
                        m_viewer->cacheImage( item_next.url() );

                ret = true; // don't pass keyEvent
            }

        } // keyPressEvent on ImageWindow


        // doubleclick closes image window
        // and shows browser when last window closed via doubleclick
        //
        // TODO: this is obscure, undiscoverable (unless you are reading
        // this comment now), and not a standard user interaction pattern.
        else if ( eventType == QEvent::MouseButtonDblClick )
        {
            QMouseEvent *ev = static_cast<QMouseEvent*>( e );
            if ( ev->button() == Qt::LeftButton )
            {
                if (s_viewers.count() == 1) slotShowWithUrl(window->currentFile()->url());
                window->deleteLater();

                ev->accept();
                ret = true;
            }
        }

    } // isA ImageWindow

    if (ret) return true;
    return KXmlGuiWindow::eventFilter(o, e);
}

void KuickShow::configuration()
{
    if ( !fileWidget ) {
        initGUI( QUrl::fromLocalFile(QDir::homePath()) );
    }

    auto collection = new KActionCollection(nullptr, QString());
    initializeBrowserActionCollection(collection);
    dialog = new KuickConfigDialog(collection, this, false);
    dialog->setObjectName(QString::fromLatin1("dialog"));
    dialog->setWindowIcon( qApp->windowIcon() );
    collection->setParent(dialog);

    connect(dialog, &KuickConfigDialog::okClicked, this, &KuickShow::slotConfigApplied);
    connect(dialog, &KuickConfigDialog::applyClicked, this, &KuickShow::slotConfigApplied);
    connect(dialog, &QDialog::finished, this, &KuickShow::slotConfigClosed);

    kuickAction(KuickActionType::Configure)->setEnabled(false);
    dialog->show();
}


void KuickShow::slotConfigApplied()
{
    dialog->applyConfig();

    KuickConfig::get().save();
    if (!initImlib()) return;

    for (ImageWindow *viewer : std::as_const(s_viewers))
    {
        viewer->updateActions();
    }

    fileWidget->reloadConfiguration();
}


void KuickShow::slotConfigClosed()
{
    dialog->deleteLater();
    kuickAction(KuickActionType::Configure)->setEnabled(true);
}

void KuickShow::about()
{
    if ( !aboutWidget ) {
        aboutWidget = new AboutWidget(nullptr);
        aboutWidget->setObjectName( QString::fromLatin1( "about" ) );
    }

    aboutWidget->adjustSize();

    QScreen *screen = windowHandle()->screen();
    const QRect screenRect = screen->geometry();
    aboutWidget->move(screenRect.center().x() - aboutWidget->width() / 2, screenRect.center().y() - aboutWidget->height() / 2);

    aboutWidget->show();
}

// ------ sessionmanagement - load / save current directory -----
void KuickShow::readProperties( const KConfigGroup& kc )
{
    assert( fileWidget ); // from SM, we should always have initGUI on startup
    QString dir = kc.readPathEntry( "CurrentDirectory", QString() );
    if ( !dir.isEmpty() ) {
        fileWidget->setUrl( QUrl( dir ), true );
        fileWidget->clearHistory();
    }

    QUrl listedURL = fileWidget->url();
    bool hasCurrentURL = false;
    const QStringList images = kc.readPathEntry( "Images shown", QStringList() );
    for (const QString &img : std::as_const(images))
    {
        const QUrl url(img);
        KFileItem item(url);
        if ( item.isReadable() ) {
            if (showImage(item, NewWindow))
            {
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
    for (const ImageWindow *viewer : std::as_const(s_viewers))
    {
        const QUrl url = viewer->url();			// checks currentFile() internally
        if (!url.isValid()) continue;			// no current file, ignore
        if ( url.isLocalFile() )
            urls.append( url.path() );
        else
            urls.append( url.toDisplayString() ); // ### check if writePathEntry( prettyUrl ) works!
    }

    // TODO: can config read/write a list of URls directly?
    kc.writePathEntry( "Images shown", urls );
}

// --------------------------------------------------------------

void KuickShow::saveSettings()
{
    KSharedConfig::Ptr kc = KSharedConfig::openConfig();
    KConfigGroup sessGroup(kc, "SessionSettings");
    if(auto oneWindowAction = kuickAction(KuickActionType::OneImageWindow))
        sessGroup.writeEntry( "OpenImagesInActiveWindow", oneWindowAction->isChecked() );

    if ( fileWidget ) {
        sessGroup.writePathEntry( "CurrentDirectory", fileWidget->url().toDisplayString() );
        KConfigGroup group( kc, "Filebrowser" );
        fileWidget->writeConfig( group);
    }

    kc->sync();
}


void KuickShow::messageCantLoadImage( const KuickFile *, const QString& message )
{
    m_viewer->clearFocus();
    KMessageBox::error( m_viewer, message, i18n("Image Error") );
}


bool KuickShow::initImlib()
{
    ImageLibrary::reset();
    auto imlib = ImageLibrary::get();
    if (imlib->isInitialized()) return true;

    KMessageBox::error(nullptr,
                       i18n("Unable to initialize the image library \"%2\".\n\n"
                            "More information may be available in the session log file,\n"
                            "or can be obtained by starting %1 from a terminal.",
                            QGuiApplication::applicationDisplayName(), imlib->getLibraryName()),
                       i18n("Fatal Initialization Error"));

    QCoreApplication::exit(1);
    deleteLater();					// to actually exit event loop
    return false;
}


bool KuickShow::haveBrowser() const
{
    return fileWidget && fileWidget->isVisible();
}

void KuickShow::delayedRepeatEvent( ImageWindow *w, QKeyEvent *e )
{
	auto clonedEvent = new QKeyEvent(e->type(), e->key(), e->modifiers(),
			e->nativeScanCode(), e->nativeVirtualKey(), e->nativeModifiers(),
			e->text(), e->isAutoRepeat(), e->count(), e->device());
	m_delayedRepeatItem = new DelayedRepeatEvent(w, clonedEvent);
}

void KuickShow::abortDelayedEvent()
{
    delete m_delayedRepeatItem;
    m_delayedRepeatItem = nullptr;
}

void KuickShow::slotReplayEvent()
{
    disconnect(fileWidget, &FileWidget::finished, this, &KuickShow::slotReplayEvent);

    DelayedRepeatEvent *e = m_delayedRepeatItem;
    m_delayedRepeatItem = nullptr; // otherwise, eventFilter aborts

    eventFilter( e->viewer, e->event );
    delete e;
    // --------------------------------------------------------------
}

void KuickShow::replayAdvance(DelayedRepeatEvent *event)
{
    slotAdvanceImage(event->viewer, *static_cast<int *>(event->data));
}

void KuickShow::delayAction(DelayedRepeatEvent *event)
{
    if (m_delayedRepeatItem)
        return;

    m_delayedRepeatItem = event;

    QUrl url = event->viewer->currentFile()->url();
    initGUI( KIO::upUrl(url) );

    // see eventFilter() for explanation and similar code
    if ( fileWidget->dirLister()->isFinished() &&
         !fileWidget->dirLister()->rootItem().isNull() )
    {
        fileWidget->setCurrentItem( url );
        QTimer::singleShot(0, this, &KuickShow::doReplay);
    }
    else
    {
        fileWidget->setInitialItem( url );
        connect(fileWidget, &FileWidget::finished, this, &KuickShow::doReplay);
    }
}

void KuickShow::doReplay()
{
    if (!m_delayedRepeatItem)
        return;

    disconnect(fileWidget, &FileWidget::finished, this, &KuickShow::doReplay);

    switch (m_delayedRepeatItem->action)
    {
        case DelayedRepeatEvent::DeleteCurrentFile:
            performDeleteCurrentImage(static_cast<QWidget *>(m_delayedRepeatItem->data));
            break;
        case DelayedRepeatEvent::TrashCurrentFile:
            performTrashCurrentImage(static_cast<QWidget *>(m_delayedRepeatItem->data));
            break;
        case DelayedRepeatEvent::AdvanceViewer:
            replayAdvance(m_delayedRepeatItem);
            break;
        default:
            qWarning("doReplay: unknown action -- ignoring: %d", m_delayedRepeatItem->action);
            break;
    }

    delete m_delayedRepeatItem;
    m_delayedRepeatItem = nullptr;
}

void KuickShow::toggleBrowser()
{
    if ( !haveBrowser() ) {
        if ( m_viewer && m_viewer->isFullscreen() )
            m_viewer->setFullscreen( false );
        fileWidget->resize( size() ); // ### somehow fileWidget isn't resized!?
        show();
        raise();
    }
    else if (!s_viewers.isEmpty()) hide();
}

void KuickShow::slotOpenURL()
{
    OpenFilesAndDirsDialog dlg(this, i18n("Select Files or Folder to Open"));
    dlg.setNameFilter(i18n("Image Files (%1)", KuickConfig::get().fileFilter));
    if(dlg.exec() != QDialog::Accepted) return;

    QList<QUrl> urls = dlg.selectedUrls();
    if(urls.isEmpty()) return;

    for (const QUrl &url : urls)
    {
        KFileItem item(url);
        if (FileWidget::isImage(item)) showImage(item, NewWindow);
        else fileWidget->setUrl( url, true );
    }
}

void KuickShow::slotToggleInlinePreview(bool on)
{
	int iconSize;
	if (on) {
		iconSize = KIconLoader::SizeEnormous;
	} else {
		iconSize = KIconLoader::SizeSmall;
	}
	fileWidget->setIconSize( iconSize );
	fileWidget->setInlinePreviewShown(on);
    QAction *defaultInlinePreview = fileWidget->action(KDirOperator::ShowPreview);
    defaultInlinePreview->setChecked(on);
}

void KuickShow::slotDuplicateWindow(const QUrl &url)
{
    qDebug() << url;
    KFileItem item(url);
    showImage(item, NewWindow);
}

void KuickShow::deleteAllViewers()
{
    for (ImageWindow *viewer : std::as_const(s_viewers))
    {
        disconnect(viewer, &QObject::destroyed, this, &KuickShow::viewerDeleted);
        delete viewer;
    }

    s_viewers.clear();
    m_viewer = nullptr;
}


/*!
 * \brief Creates all KuickShow-specific QAction objects and stores them in KuickShow::kuickActions.
 *
 * <p>All created actions have their parent set to the current KuickShow instance.
 */
void KuickShow::setupKuickActions()
{
	QAction* action;
	KToggleAction* toggleAction;

	// application actions
	kuickActions[KuickActionType::SlideShow] = action = new QAction(QIcon::fromTheme("ksslide"),
			i18n("Start Slideshow"), this);
	connect(action, &QAction::triggered, this, &KuickShow::startSlideShow);

	kuickActions[KuickActionType::ToggleBrowser] = toggleAction = new KToggleAction(i18n("Show File Browser"), this);
	toggleAction->setCheckedState(KGuiItem(i18n("Hide File Browser")));
	connect(toggleAction, &QAction::toggled, this, &KuickShow::toggleBrowser);

	kuickActions[KuickActionType::Configure] = action = new QAction(QIcon::fromTheme("configure"),
			i18n("Configure %1...", QGuiApplication::applicationDisplayName()), this);
	connect(action, &QAction::triggered, this, &KuickShow::configuration);

	kuickActions[KuickActionType::About] = action = new QAction(QIcon::fromTheme("about"),
			i18n("About KuickShow"), this);
	connect(action, &QAction::triggered, this, &KuickShow::about);

	kuickActions[KuickActionType::Quit] = KStandardAction::quit(this, &QObject::deleteLater, this);

	// application settings
	kuickActions[KuickActionType::OneImageWindow] = new KToggleAction(QIcon::fromTheme("window-new"),
			i18n("Open Only One Image Window"), this);

	const QAction* defaultInlinePreview = fileWidget->action(KDirOperator::ShowPreview);
	kuickActions[KuickActionType::InlinePreview] = toggleAction = new KToggleAction(defaultInlinePreview->icon(),
			defaultInlinePreview->text(), this);
	connect(toggleAction, &QAction::toggled, this, &KuickShow::slotToggleInlinePreview);

	// image actions
	kuickActions[KuickActionType::OpenUrl] = KStandardAction::open(this, &KuickShow::slotOpenURL, this);

	kuickActions[KuickActionType::ShowImageInNewWindow] = action = new QAction(QIcon::fromTheme("window-new"),
			i18n("Show Image"), this);
	connect(action, &QAction::triggered, this, &KuickShow::slotShowInOtherWindow);

	kuickActions[KuickActionType::ShowImageInActiveWindow] = action = new QAction(QIcon::fromTheme("viewimage"),
			i18n("Show Image in Active Window"), this);
	connect(action, &QAction::triggered, this, &KuickShow::slotShowInSameWindow);

	kuickActions[KuickActionType::ShowImageFullScreen] = action = new QAction(QIcon::fromTheme("view-fullscreen"),
			i18n("Show Image in Fullscreen Mode"), this);
	connect(action, &QAction::triggered, this, &KuickShow::slotShowFullscreen);

	kuickActions[KuickActionType::PrintImage] = action = KStandardAction::print(this, &KuickShow::slotPrint, this);
	action->setText(i18n("Print Image..."));

	// Manually fetch the configured shortcuts for *all* actions:
	// 1) The actions created above aren't part of any KActionCollection and therefore not automatically initialized.
	// 2) We define default shortcuts for some of FileWidget's actions, which must be taken into account when loading
	//    the shortcuts from the config file.
	KActionCollection collection(nullptr, QString());
	initializeBrowserActionCollection(&collection);
	collection.readSettings();
}

/*!
 * \brief Fills an existing KActionCollection with the actions of fileWidget and KuickShow's own actions.
 *
 * <p>The function fills the supplied collection with KuickShow's own actions (initialized in
 * KuickShow::setupKuickActions()), as well as all actions of fileWidget. It also applies a default
 * shortcut to some of those actions.
 *
 * <p>Note: Any preexisting actions in the collection are removed.
 *
 * @param collection The action collection to initialize.
 */
void KuickShow::initializeBrowserActionCollection(KActionCollection* collection) const
{
	collection->clear();
	collection->addActions(fileWidget->allActions());
	// Note: the keys are used in the config file to identify the actions; do not modify!
	collection->addAction(QStringLiteral("kuick_slideshow"), kuickAction(KuickActionType::SlideShow));
	collection->addAction(QStringLiteral("toggleBrowser"), kuickAction(KuickActionType::ToggleBrowser));
	collection->addAction(QStringLiteral("kuick_configure"), kuickAction(KuickActionType::Configure));
	collection->addAction(QStringLiteral("about"), kuickAction(KuickActionType::About));
	collection->addAction(QStringLiteral("quit"), kuickAction(KuickActionType::Quit));
	collection->addAction(QStringLiteral("kuick_one window"), kuickAction(KuickActionType::OneImageWindow));
	collection->addAction(QStringLiteral("kuick_inlinePreview"), kuickAction(KuickActionType::InlinePreview));
	collection->addAction(QStringLiteral("openURL"), kuickAction(KuickActionType::OpenUrl));
	collection->addAction(QStringLiteral("kuick_showInOtherWindow"), kuickAction(KuickActionType::ShowImageInNewWindow));
	collection->addAction(QStringLiteral("kuick_showInSameWindow"), kuickAction(KuickActionType::ShowImageInActiveWindow));
	collection->addAction(QStringLiteral("kuick_showFullscreen"), kuickAction(KuickActionType::ShowImageFullScreen));
	collection->addAction(QStringLiteral("kuick_print"), kuickAction(KuickActionType::PrintImage));

	auto setDefaultShortcuts = [&collection](QAction* action, const QList<QKeySequence>& defaultShortcuts) {
		auto currentShortcuts = action->shortcuts();
		collection->setDefaultShortcuts(action, defaultShortcuts);
		action->setShortcuts(currentShortcuts);
	};

	setDefaultShortcuts(fileWidget->action(KDirOperator::Reload), KStandardShortcut::reload());
	setDefaultShortcuts(fileWidget->action(KDirOperator::ShortView), { Qt::Key_F6 });
	setDefaultShortcuts(fileWidget->action(KDirOperator::DetailedView), { Qt::Key_F7 });
	setDefaultShortcuts(fileWidget->action(KDirOperator::NewFolder), { Qt::Key_F10 });
	setDefaultShortcuts(fileWidget->action(KDirOperator::ShowPreviewPanel), { Qt::Key_F11 });
	setDefaultShortcuts(kuickAction(KuickActionType::SlideShow), { Qt::Key_F2 });
	setDefaultShortcuts(kuickAction(KuickActionType::ToggleBrowser), { Qt::Key_Space });
	setDefaultShortcuts(kuickAction(KuickActionType::OneImageWindow), { Qt::CTRL | Qt::Key_N });
}
