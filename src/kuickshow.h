/****************************************************************************
 ** $Id$
 **
 ** KuickShow - a fast and comfortable image viewer based on Rasterman's Imlib
 **
 ** Created : 98
 **
 ** Copyright (C) 1998, 1999 by Carsten Pfeiffer.  All rights reserved.
 **
****************************************************************************/

#ifndef KUICKSHOW_H
#define KUICKSHOW_H

#include <qevent.h>
#include <qstring.h>
#include <qvaluelist.h>

#include <kmainwindow.h>
#include <kurl.h>

#include <Imlib.h>

class FileWidget;
class ImageWindow;
class ImData;
class KFileItem;
class KuickConfigDialog;

class KAccel;
class KConfig;
class KToggleAction;

class DelayedRepeatEvent
{
public:
    DelayedRepeatEvent( ImageWindow *view, QKeyEvent *ev ) {
        viewer = view;
        event  = ev;
    }
    DelayedRepeatEvent( ImageWindow *view, int step ) {
        viewer = view;
        steps  = step;
        event  = 0L;
    }
    ~DelayedRepeatEvent() {
        delete event;
    }

    ImageWindow *viewer;
    QKeyEvent *event;
    int steps;
};


class KuickShow : public KMainWindow
{
    Q_OBJECT

public:
    KuickShow( const char *name=0 );
    ~KuickShow();

    virtual void 	show();
    static QValueList<ImageWindow*>  s_viewers;

protected:
    virtual void	readProperties( KConfig * );
    void 		initImlibParams( ImData *, ImlibInitParams * );

private slots:
    void 		slotQuit() { delete this; }
    void 		slotPrint();
    void 		slotConfigApplied();
    void 		slotConfigClosed();
    void 		messageCantLoadImage( const QString& );
    void         	showImage(const KFileItem *, bool newWindow = false,
                                  bool fullscreen = false );
    void 		showFileItem( ImageWindow *, const KFileItem * );
    void		slotHighlighted( const KFileItem * );
    void 		slotSelected( const KFileItem * );
    void		dirSelected( const KURL& );
    void		configuration();
    void	      	about();
    void 		startSlideShow();
    void 		nextSlide();
    void                nextSlide( KFileItem *item );
    void		viewerDeleted();
    void 		dropEvent( QDropEvent * );
    void 		slotSetActiveViewer( ImageWindow *i ) { m_viewer = i; }
    void                slotAdvanceImage( ImageWindow *, int steps );

    void 		slotShowInSameWindow();
    void 		slotShowInOtherWindow();

    void		slotReplayEvent();
    void        slotReplayAdvance();

private:
    void 		initGUI( const KURL& startDir );
    bool	       	eventFilter( QObject *, QEvent * );
    void 		initImlib();
    void 		saveProperties( KConfig * );
    void 		saveSettings();
    bool 		haveBrowser() const;
    void 		delayedRepeatEvent( ImageWindow *, QKeyEvent * );
    void                toggleBrowser( bool show );

    uint 		viewItem, renameItem, deleteItem, printItem;
    uint                m_slideshowCycle;

    FileWidget   	*fileWidget;
    KuickConfigDialog 	*dialog;
    ImlibData           *id;
    ImageWindow 	*m_viewer;
    KToggleAction 	*oneWindowAction;
    KAccel 		*m_accel;
    DelayedRepeatEvent  *m_delayedRepeatItem;
    QTimer              *m_slideTimer;

};

#endif
