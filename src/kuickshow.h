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

#include <KFileItem>
#include <KXmlGuiWindow>

#include <QDropEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QPointer>
#include <QString>
#include <QX11Info>

#include "imlib-wrapper.h"
#include "aboutwidget.h"

class KAccel;
class KConfig;
class KToggleAction;
class KUrlComboBox;

class QLabel;
class QUrl;

class AboutWidget;
class FileWidget;
class ImageWindow;
class ImData;
class KuickConfigDialog;
class KuickFile;


class DelayedRepeatEvent
{
public:
    DelayedRepeatEvent( ImageWindow *view, QKeyEvent *ev ) {
        viewer = view;
        event  = ev;
    }
    DelayedRepeatEvent( ImageWindow *view, int action, void *data ) {
        this->viewer = view;
        this->action = action;
        this->data   = data;
        this->event  = 0L;
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


class KuickShow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    KuickShow( const char *name=0 );
    ~KuickShow();

    virtual void 	show();
    static QList<ImageWindow*>  s_viewers;

    // overridden to make KDCOPActionProxy work -- all our actions are not
    // in the mainwindow's collection, but in the filewidget's.
    virtual KActionCollection* actionCollection() const;


protected:
    virtual void	readProperties( const KConfigGroup& kc );
    void 		initImlibParams( ImData *, ImlibInitParams * );
    void                tryShowNextImage();

private slots:
    void		toggleBrowser();
    void 		slotToggleInlinePreview( bool on );
    void 		slotQuit() { delete this; }
    void 		slotPrint();
    void 		slotConfigApplied();
    void 		slotConfigClosed();
    void 		messageCantLoadImage( const KuickFile *file, const QString& message );
    bool     	showImage(const KFileItem&, bool newWindow = false,
                          bool fullscreen = false, bool moveToTopLeft = true, bool ignoreFileType = false );
    void 		showFileItem( ImageWindow *, const KFileItem * );
    void		slotHighlighted( const KFileItem& );
    void 		slotSelected( const KFileItem& );
    void		dirSelected( const QUrl& );
    void		configuration();
    void	      	about();
    void 		startSlideShow();
    void                pauseSlideShow();
    void 		nextSlide();
    void                nextSlide( const KFileItem& item );
    void		viewerDeleted();
    void 		slotDropped( const KFileItem&, QDropEvent *, const QList<QUrl> &);
    void 		slotSetActiveViewer( ImageWindow *i ) { m_viewer = i; }
    void                slotAdvanceImage( ImageWindow *, int steps );

    void 		slotShowInSameWindow();
    void 		slotShowInOtherWindow();
    void                slotShowFullscreen();

    void		slotReplayEvent();
    void                slotOpenURL();
    void		slotSetURL( const QUrl& );
    void		slotURLComboReturnPressed();
//     void                invalidateImages( const KFileItemList& items );
    void		slotDeleteCurrentImage(ImageWindow *viewer);
    void		slotTrashCurrentImage(ImageWindow *viewer);
    void                slotDeleteCurrentImage();
    void                slotTrashCurrentImage();

    void                doReplay();

private:
    Display *		getX11Display() const { return QX11Info::display(); }
    int getX11Screen() const;
    void 		initGUI( const QUrl& startDir );
    bool	       	eventFilter( QObject *, QEvent * );
    void 		initImlib();
    void 		saveProperties( KConfigGroup& kc );
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
    ImlibData           *id;
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
