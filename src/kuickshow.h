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

#ifndef KUICKSHOW_H
#define KUICKSHOW_H

#include <qevent.h>
#include <qguardedptr.h>
#include <qstring.h>
#include <qvaluelist.h>

#include <kmainwindow.h>
#include <kurl.h>

#include <Imlib.h>

#include "aboutwidget.h"

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
    void                toggleBrowser();
    void 		slotQuit() { delete this; }
    void 		slotPrint();
    void 		slotConfigApplied();
    void 		slotConfigClosed();
    void 		messageCantLoadImage( const QString& );
    void         	showImage(const KFileItem *, bool newWindow = false,
                                  bool fullscreen = false, bool moveToTopLeft = true );
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
    void                slotReplayAdvance();
    void                slotOpenURL();

private:
    void 		initGUI( const KURL& startDir );
    bool	       	eventFilter( QObject *, QEvent * );
    void 		initImlib();
    void 		saveProperties( KConfig * );
    void 		saveSettings();
    bool 		haveBrowser() const;
    void 		delayedRepeatEvent( ImageWindow *, QKeyEvent * );
    void                deleteAllViewers();

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
    KAction             *m_toggleBrowserAction;
    QGuardedPtr<AboutWidget> aboutWidget;

};

#endif
