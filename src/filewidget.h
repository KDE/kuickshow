/****************************************************************************
** $Id$
**
** Definition of something or other
**
** Created : 2000
**
** Copyright (C) 2000 by Carsten Pfeiffer
**
****************************************************************************/

#ifndef FILEWIDGET_H
#define FILEWIDGET_H

#include <qevent.h>

#include <kdiroperator.h>

class FileFinder;
class KFileItem;

class FileWidget : public KDirOperator
{
    Q_OBJECT

public:
    enum WhichItem { Previous, Next, Current };

    FileWidget( const KURL& url, QWidget *parent = 0L, const char *name = 0L );
    ~FileWidget();

    bool hasFiles() const;
    void reloadConfiguration();

    void setInitialItem( const QString& filename );

    KFileViewItem *getCurrentItem( bool onlyImage ) const {
	return getItem( Current, onlyImage );
    }

    void setCurrentItem( const KFileViewItem * );
    void setCurrentItem( const QString& filename ) {
	KDirOperator::setCurrentItem( filename );
    }

    KFileViewItem * gotoFirstImage();
    KFileViewItem * gotoLastImage();
    KFileViewItem * getNext( bool go=true );
    KFileViewItem * getPrevious( bool go=true );


    KFileViewItem *getItem( WhichItem which, bool onlyImage ) const;

    static bool isImage( const KFileItem * );

    void initActions();

signals:
    void finished();

protected:
    virtual bool eventFilter( QObject *o, QEvent * );
    virtual void resizeEvent( QResizeEvent * );
    virtual void activatedMenu( const KFileViewItem * );

private slots:
    void slotReturnPressed( const QString& text );
    void findCompletion( const QString& );
    void slotViewChanged();

    void slotItemsCleared();
    void slotItemDeleted( KFileItem * );
    void slotHighlighted( const KFileViewItem * );

    void slotFinishedLoading();

private:
    KFileView * fileView() const { return (KFileView*) view(); }

    bool m_validCompletion;
    FileFinder *m_fileFinder;
    QString m_currentURL;
    QString m_initialName;

};


#endif // FILEWIDGET_H
