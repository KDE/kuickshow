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

#ifndef KUICKSHOW_H
#define KUICKSHOW_H

#include <KXmlGuiWindow>
#include <QPointer>

class KFileItem;
class KToggleAction;
class KUrlComboBox;
class QDropEvent;
class QEvent;
class QLabel;
class QString;
class QUrl;
class AboutWidget;
class FileWidget;
class ImageWindow;
class KuickConfigDialog;
class KuickFile;
class DelayedRepeatEvent;


class KuickShow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit KuickShow(const char *name = nullptr);
    virtual ~KuickShow();

    virtual void 	show();

    // overridden to make KDCOPActionProxy work -- all our actions are not
    // in the mainwindow's collection, but in the filewidget's.
    virtual KActionCollection* actionCollection() const override;


protected:
    virtual void	readProperties( const KConfigGroup& kc ) override;
    void                tryShowNextImage();

public:
    enum ShowFlag
    {
        ShowDefault = 0x00,
        NewWindow = 0x01,
        FullScreen = 0x02,
        NoMoveToTopLeft = 0x04,
        IgnoreFileType = 0x08
    };
    Q_DECLARE_FLAGS(ShowFlags, ShowFlag)

private slots:
    void		toggleBrowser();
    void 		slotToggleInlinePreview( bool on );
    void 		slotPrint();
    void 		slotConfigApplied();
    void 		slotConfigClosed();
    void 		messageCantLoadImage( const KuickFile *file, const QString& message );
    bool		showImage(const KFileItem&, KuickShow::ShowFlags flags);
    void 		showFileItem( ImageWindow *, const KFileItem * );
    void		slotHighlighted( const KFileItem& );
    void 		slotSelected( const KFileItem& );
    void		dirSelected( const QUrl& );
    void		configuration();
    void	      	about();
    void		slotDuplicateWindow(const QUrl &url);
    void 		startSlideShow();
    void                pauseSlideShow();
    void 		nextSlide();
    void                nextSlide( const KFileItem& item );
    void		viewerDeleted();
    void 		slotDropped( const KFileItem&, QDropEvent *, const QList<QUrl> &);
    void 		slotSetActiveViewer( ImageWindow *i ) { m_viewer = i; }
    void                slotAdvanceImage( ImageWindow *, int steps );
    void                slotShowWithUrl(const QUrl &url);

    void 		slotShowInSameWindow();
    void 		slotShowInOtherWindow();
    void                slotShowFullscreen();

    void		slotReplayEvent();
    void                slotOpenURL();
    void		slotSetURL( const QUrl& );
    void		slotURLComboReturnPressed();
    void		slotDeleteCurrentImage(ImageWindow *viewer);
    void		slotTrashCurrentImage(ImageWindow *viewer);
    void                slotDeleteCurrentImage();
    void                slotTrashCurrentImage();

    void                doReplay();

private:
    void 		initGUI( const QUrl& startDir );
    bool	       	eventFilter( QObject *, QEvent * ) override;
    bool 		initImlib();
    void 		saveProperties( KConfigGroup& kc ) override;
    void 		saveSettings();
    bool 		haveBrowser() const;
    void 		delayedRepeatEvent( ImageWindow *, QKeyEvent * );
    void		abortDelayedEvent();
    void                deleteAllViewers();
    void                redirectDeleteAndTrashActions(KActionCollection *coll);

    void                delayAction(DelayedRepeatEvent *event);
    void                replayAdvance(DelayedRepeatEvent *event);

    void                performDeleteCurrentImage(QWidget *parent);
    void                performTrashCurrentImage(QWidget *parent);

    uint 		viewItem, renameItem, deleteItem, printItem;
    uint                m_slideshowCycle;

    FileWidget   	*fileWidget;
    KUrlComboBox	*cmbPath;
    KuickConfigDialog 	*dialog;

    // This variable identifies the currently active image viewer window:
    // that is, either a newly created one or the last ImageWindow that a
    // filtered event was received on, see eventFilter().
    //
    // TODO: maybe it would be better renamed to 'm_activeViewer'.
    ImageWindow 	*m_viewer;

    KToggleAction 	*oneWindowAction;
    DelayedRepeatEvent  *m_delayedRepeatItem;
    QTimer              *m_slideTimer;
    bool                m_slideShowStopped;
    KToggleAction       *m_toggleBrowserAction;
    QPointer<AboutWidget> aboutWidget;

    QLabel* sblblUrlInfo;
    QLabel* sblblMetaInfo;
    QLabel* createStatusBarLabel(int stretch);
};

#endif
