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
class KFileViewItem;
class KuickConfigDialog;

class KAccel;
class KConfig;
class KToggleAction;

class DelayedRepeatEvent
{
public:
    ImageWindow *viewer;
    QKeyEvent *event; // leaking the QKeyEvent here :}
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

private:
    void 		initGUI( const KURL& startDir );
    bool	       	eventFilter( QObject *, QEvent * );
    void 		initImlib();
    void 		saveProperties( KConfig * );
    void 		saveSettings();
    bool 		haveBrowser() const;
    void 		delayedRepeatEvent( ImageWindow *, QKeyEvent * );

    uint 		viewItem, renameItem, deleteItem, printItem;

    FileWidget   	*fileWidget;
    KuickConfigDialog 	*dialog;
    ImlibData           *id;
    ImageWindow 	*viewer;
    KToggleAction 	*newWindowAction;
    KAccel 		*m_accel;
    bool		m_lockEvents;
    DelayedRepeatEvent  *m_delayedRepeatItem;

private slots:
    void 		slotQuit() { delete this; }
    void 		slotPrint();
    void 		slotShowProperties();
    void 		slotConfigApplied();
    void 		slotConfigClosed();
    void 		messageCantLoadImage( const QString& );
    void         	showImage(const KFileViewItem *, bool newWin = false);
    void 		showFileItem( ImageWindow *, const KFileViewItem * );
    void		slotHighlighted( const KFileViewItem * );
    void 		slotSelected( const KFileViewItem * );
    void		dirSelected( const KURL& );
    void		configuration();
    void	      	about();
    void 		startSlideShow();
    void 		nextSlide();
    void		viewerDeleted();
    void 		dropEvent( QDropEvent * );
    void 		slotSetActiveViewer( ImageWindow *i ) { viewer = i; }

    void 		slotShowInSameWindow();
    void 		slotShowInOtherWindow();

    void		slotReplayEvent();

    void slotDelete(); // ### fallback, remove somewhen

};

#endif
