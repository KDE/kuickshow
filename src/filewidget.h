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

    KFileItem *getCurrentItem( bool onlyImage ) const {
	return getItem( Current, onlyImage );
    }

    void setCurrentItem( const KFileItem * );
    void setCurrentItem( const QString& filename ) {
	KDirOperator::setCurrentItem( filename );
    }

    KFileItem * gotoFirstImage();
    KFileItem * gotoLastImage();
    KFileItem * getNext( bool go=true );
    KFileItem * getPrevious( bool go=true );


    KFileItem *getItem( WhichItem which, bool onlyImage ) const;

    static bool isImage( const KFileItem * );
    static void setImage( KFileItem& item, bool enable );

    void initActions();

signals:
    void finished();

protected:
    virtual bool eventFilter( QObject *o, QEvent * );
    virtual void resizeEvent( QResizeEvent * );
    virtual void activatedMenu( const KFileItem *, const QPoint& );

private slots:
    void slotReturnPressed( const QString& text );
    void findCompletion( const QString& );
    void slotViewChanged();

    void slotItemsCleared();
    void slotItemDeleted( KFileItem * );
    void slotHighlighted( const KFileItem * );

    void slotURLEntered( const KURL& url );
    void slotFinishedLoading();

private:
    KFileView * fileView() const { return (KFileView*) view(); }

    bool m_validCompletion;
    FileFinder *m_fileFinder;
    QString m_currentURL;
    QString m_initialName;

};


#endif // FILEWIDGET_H
