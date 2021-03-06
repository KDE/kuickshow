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

#include "filewidget.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KCompletion>
#include <KConfigGroup>
#include <KFileItemActions>
#include <KFileItemListProperties>
#include <KLocalizedString>

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeDatabase>
#include <QModelIndex>
#include <QResizeEvent>
#include <QTimer>

#include "filefinder.h"
#include "kuickdata.h"
#include "kuickshow_debug.h"
#include "kuickshow.h"


FileWidget::FileWidget( const QUrl& url, QWidget *parent )
    : KDirOperator( url, parent ),
      m_validCompletion( false ),
      m_fileFinder( 0L ),
      m_fileItemActions( 0L )
{
    setEnableDirHighlighting( true );

    KConfigGroup group(KSharedConfig::openConfig(), "Filebrowser");
    setViewConfig( group );
    readConfig( group );
    setView( KFile::Default );

    // setOnlyDoubleClickSelectsFiles( true );
    reloadConfiguration();

    completionObject()->setCompletionMode( KCompletion::CompletionAuto );
    dirCompletionObject()->setCompletionMode( KCompletion::CompletionAuto);

    slotViewChanged();
    connect( this, SIGNAL( viewChanged( QAbstractItemView * )),
	     SLOT( slotViewChanged() ));

    connect( dirLister(), SIGNAL( clear() ), SLOT( slotItemsCleared() ));
    connect( dirLister(), SIGNAL( itemsDeleted( const KFileItemList&  ) ),
	     SLOT( slotItemsDeleted( const KFileItemList& ) ));

    connect( this, SIGNAL( fileHighlighted( const KFileItem& )),
	     SLOT( slotHighlighted( const KFileItem& )));

    connect( this, SIGNAL(urlEntered(const QUrl&)),
             SLOT( slotURLEntered( const QUrl& )));

    // should actually be KDirOperator's job!
    connect( this, SIGNAL( finishedLoading() ), SLOT( slotFinishedLoading() ));

    connect( this, SIGNAL( contextMenuAboutToShow( const KFileItem&, QMenu *) ),
             SLOT( slotContextMenu( const KFileItem&, QMenu *)));
}

FileWidget::~FileWidget()
{
    delete m_fileFinder;
}

void FileWidget::initActions()
{
    KActionCollection *coll = actionCollection();
    KActionMenu *menu = static_cast<KActionMenu*>( coll->action("popupMenu") );

    menu->addAction(coll->action("kuick_showInOtherWindow"));
    menu->addAction(coll->action("kuick_showInSameWindow"));
    menu->addAction(coll->action("kuick_showFullscreen"));
    menu->addSeparator();

    // properties dialog is now in kfile, but not at the right position,
    // so we move it to the real bottom
    menu->menu()->removeAction( coll->action( "properties" ) );
/*
    KMenu *pMenu = menu->menu();
    int lastItemId = pMenu->idAt( pMenu->count() - 1 );
    QMenuItem *mItem = pMenu->findItem( lastItemId );
    if ( mItem && !mItem->isSeparator() )
        menu->addSeparator();
*/
    // those at the bottom
    menu->addAction(coll->action("kuick_print") );
    menu->addSeparator();
    menu->addAction(coll->action("properties") );
}

void FileWidget::reloadConfiguration()
{
    if ( kdata->fileFilter != nameFilter() ) {
	// At first, our list must have folders
	QStringList mimes;
	mimes.append("inode/directory");

	// Then, all the images!
	QMimeDatabase mimedb;
	QList<QMimeType> l = mimedb.allMimeTypes();
	for (QList<QMimeType>::const_iterator it = l.begin(); it != l.end(); ++it)
	    if (it->name().startsWith( "image/" ))
		mimes.append( it->name() );

	// Ok, show what we've done
	setMimeFilter (mimes);
	updateDir();
    }
}

bool FileWidget::hasFiles() const
{
    return (numFiles() > 0);
}

void FileWidget::slotContextMenu( const KFileItem& item, QMenu *popupMenu )
{
    bool image = isImage( item );
    actionCollection()->action("kuick_showInSameWindow")->setEnabled( image );
    actionCollection()->action("kuick_showInOtherWindow")->setEnabled( image );
    actionCollection()->action("kuick_showFullscreen")->setEnabled( image );
    actionCollection()->action("kuick_print")->setEnabled( image );

    KActionCollection *coll = actionCollection();
    KActionMenu *menu = static_cast<KActionMenu*>( coll->action("popupMenu") );

    menu->addAction(coll->action("kuick_showInOtherWindow"));
    menu->addAction(coll->action("kuick_showInSameWindow"));
    menu->addAction(coll->action("kuick_showFullscreen"));
    menu->addSeparator();

    if (!item.isNull()) {
	KFileItemList items;
	items.append(item);
	KFileItemListProperties properties( items );
	if ( !m_fileItemActions ) {
	    m_fileItemActions = new KFileItemActions( this );
	    m_fileItemActions->setParentWidget( this );
	}
	m_fileItemActions->setItemListProperties( properties );
	m_fileItemActions->addOpenWithActionsTo( menu->menu(), QString() );
    }

   // properties dialog is now in kfile, but not at the right position,
    // so we move it to the real bottom
    menu->menu()->removeAction( coll->action( "properties" ) );

    // those at the bottom
    menu->addAction(coll->action("kuick_print") );
    menu->addSeparator();
    menu->addAction(coll->action("properties") );
}

void FileWidget::findCompletion( const QString& text )
{
    if ( text.at(0) == '/' || text.at(0) == '~' ||
	 text.indexOf('/') != -1 ) {
	QString t = m_fileFinder->completion()->makeCompletion( text );

    if (m_fileFinder->completionMode() == KCompletion::CompletionPopup ||
            m_fileFinder->completionMode() == KCompletion::CompletionPopupAuto)
	    m_fileFinder->setCompletedItems(
			      m_fileFinder->completion()->allMatches() );
	else
            if ( !t.isNull() )
                m_fileFinder->setCompletedText( t );

	return;
    }

    QString file = makeDirCompletion( text );
    if ( file.isNull() )
	file = makeCompletion( text );
    m_validCompletion = !file.isNull();

    if ( m_validCompletion ) {
        QUrl completeUrl = url();
        completeUrl.setPath(completeUrl.adjusted(QUrl::RemoveFilename).path() + file);
    	KDirOperator::setCurrentItem( completeUrl );
    }
}

bool FileWidget::eventFilter( QObject *o, QEvent *e )
{
    if ( e->type() == QEvent::KeyPress ) {
	QKeyEvent *k = static_cast<QKeyEvent*>( e );

	if ( (k->modifiers() & (Qt::ControlModifier | Qt::AltModifier)) == 0 ) {
	    int key = k->key();
 	    if ( actionCollection()->action("delete")->shortcuts().contains( key ) )
            {
                k->accept();
		KFileItem item = getCurrentItem( false );
		if ( !item.isNull() ) {
                    KFileItemList list;
                    list.append( item );
		    del( list, this, (k->modifiers() & Qt::ShiftModifier) == 0 );
                }
		return true;
	    }

	    const QString& text = k->text();
	    if ( !text.isEmpty() && text.unicode()->isPrint() ) {
                k->accept();

                if ( !m_fileFinder ) {
		    m_fileFinder = new FileFinder( this );
		    m_fileFinder->setObjectName( "file finder" );
		    connect( m_fileFinder, SIGNAL( completion(const QString&)),
			     SLOT( findCompletion( const QString& )));
		    connect( m_fileFinder,
			     SIGNAL( enterDir( const QString& ) ),
			     SLOT( slotReturnPressed( const QString& )));
		    m_fileFinder->move( width()  - m_fileFinder->width(),
					height() - m_fileFinder->height() );
		}

		bool first = m_fileFinder->isHidden();

		m_fileFinder->setText( text );
		m_fileFinder->raise();
		m_fileFinder->show();
		m_fileFinder->setFocus();
		if ( first )
		    findCompletion( text );

		return true;
	    }
        }

        k->ignore();
    }
    return KDirOperator::eventFilter( o, e );
}


bool FileWidget::isImage( const KFileItem& item )
{
//     return item && !item.isDir();
    if ( !item.isNull() )
    {
        return item.isReadable() && item.mimetype().startsWith( "image/");
    }
    return false;
}

KFileItem FileWidget::gotoFirstImage()
{
    QModelIndex modelIndex = view()->model()->index( 0, 0 );
    while ( modelIndex.isValid() ) {
        KFileItem item = fileItemFor(modelIndex);
	if ( isImage( item ) ) {
            setCurrentItem( item );
	    return item;
	}

	modelIndex = modelIndex.sibling( modelIndex.row() + 1, modelIndex.column() );
    }
    return KFileItem();
}

KFileItem FileWidget::gotoLastImage()
{
    QAbstractItemModel *model = view()->model();
    int numRows = model->rowCount();
    QModelIndex index = model->index(numRows - 1, 0); // last item
    while ( index.isValid() ) {
	KFileItem fileItem = fileItemFor( index );
	if (isImage( fileItem )) {
	    setCurrentItem( fileItem );
            return fileItem;
	}

        index = index.parent();
    }

    return KFileItem();
}

KFileItem FileWidget::getNext( bool go )
{
    KFileItem item = getItem( Next, true );
    if ( !item.isNull() ) {
	if ( go )
            // This needs to be done on a single shot timer because,
            // if the setCurrentItem() is done immediately, the current
            // index of the view() seems to be cleared soon afterwards by
            // a KCoreDirLister::completed() signal.  It is not clear what
            // causes the sending of this signal, but the result is that
            // the current index is left cleared and the next call of
            // getItem() will fail.
            QTimer::singleShot(0, this, [item, this]() { setCurrentItem(item); });
	return item;
    }

    return KFileItem();
}

KFileItem FileWidget::getPrevious( bool go )
{
    KFileItem item = getItem( Previous, true );
    if ( !item.isNull() ) {
	if ( go )
            QTimer::singleShot(0, this, [item, this]() { setCurrentItem(item); });
	return item;
    }

    return KFileItem();
}

// returns a null item when there is no previous/next item/image
// this sucks! Use KFileView::currentFileItem() when implemented
KFileItem FileWidget::getItem( WhichItem which, bool onlyImage ) const
{
    QModelIndex currentIndex = view()->currentIndex();
    if ( !currentIndex.isValid() ) {
        qDebug("no current index");
        return KFileItem();
    }

    QModelIndex index = currentIndex;
    KFileItem item;
    const int column = index.column();
    item = fileItemFor( currentIndex );
    if ( item.isNull() )
        qDebug("### current item is null: %s, %s", index.data().typeName(), qUtf8Printable(currentIndex.data().value<QString>()));

    switch( which ) {
    case Previous: {
        index = index.sibling( index.row() - 1, column );
        while ( index.isValid() ) {
            item = fileItemFor(index);
	    if ( !item.isNull() && (!onlyImage || isImage( item )) ) {
                return item;
	    }
            index = index.sibling( index.row() - 1, column );
	}

        return KFileItem(); // no previous item / image
    }

    case Next: {
        index = index.sibling( index.row() + 1, column );
        while ( index.isValid() ) {
            item = fileItemFor(index);
	    if ( !item.isNull() && (!onlyImage || isImage( item )) ) {
                return item;
	    }
            index = index.sibling( index.row() + 1, column );
	}

	    return KFileItem(); // no further item / image
     }

     case Current:
	default:
	    return fileItemFor(currentIndex);
    }
    return KFileItem();
}

void FileWidget::slotViewChanged()
{
    view()->installEventFilter( this );
}

void FileWidget::slotItemsCleared()
{
    m_currentURL.clear();
}

void FileWidget::slotItemsDeleted( const KFileItemList& items )
{
    KFileItem current = getCurrentItem( false );
    if ( !items.contains(current) ) {
	return; // all ok, we already have a new current item
    }

    KFileItem next = getNext();
    if ( next.isNull() )
	next = getPrevious();

    if ( !next.isNull() )
	m_currentURL = next.url().url();
}

void FileWidget::slotHighlighted( const KFileItem& item )
{
    if ( !item.isNull() ) {
        m_currentURL = item.url().url();
    }
    else {
        m_currentURL = QString();
    }
}

void FileWidget::slotReturnPressed( const QString& t )
{
    // we need a / at the end, otherwise replacedPath() will cut off the dir,
    // assuming it is a filename
    QString text = t;
    if ( text.at( text.length()-1 ) != '/' )
	text += '/';

    if ( text.at(0) == '/' || text.at(0) == '~' ) {
	QString dir = m_fileFinder->completion()->replacedPath( text );
        setUrl( QUrl::fromLocalFile(dir), true );
    }

    else if ( text.indexOf('/') != (int) text.length() -1 ) { // relative path
	QString dir = m_fileFinder->completion()->replacedPath( text );
        QUrl u = url().resolved(QUrl(dir));
	setUrl( u, true );
    }

    else if ( m_validCompletion ) {
	KFileItem item = getCurrentItem( true );

	if ( !item.isNull() ) {
	    if ( item.isDir() )
		setUrl( item.url(), true );
	    else
		emit fileSelected( item );
	}
    }
}

void FileWidget::setInitialItem( const QUrl& url )
{
    m_initialName = url;
}

void FileWidget::slotURLEntered( const QUrl& url )
{
    if ( m_fileFinder )
        m_fileFinder->completion()->setDir( url );
}

void FileWidget::slotFinishedLoading()
{
	const KFileItem& current = getCurrentItem( false );
	if ( !m_initialName.isEmpty() )
		setCurrentItem( m_initialName );
	else if ( current.isNull() ) {
		QModelIndex first = view()->model()->index(0, 0);
		if (first.isValid()) {
			KFileItem item = first.data(KDirModel::FileItemRole).value<KFileItem>();
			if (!item.isNull()) {
				setCurrentItem( item );
			}
		}
	}

	m_initialName = QUrl();
	emit finished();
}

QSize FileWidget::sizeHint() const
{
  return QSize( 300, 300 );
}

void FileWidget::resizeEvent( QResizeEvent *e )
{
    KDirOperator::resizeEvent( e );
    if ( m_fileFinder )
	m_fileFinder->move( width()  - m_fileFinder->width(),
			    height() - m_fileFinder->height() );
}
