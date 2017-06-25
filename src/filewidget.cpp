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

#include <qnamespace.h>
//Added by qt3to4:
#include <QKeyEvent>
#include <QResizeEvent>
#include <QEvent>
#include <QMenuItem>
#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QModelIndex>

#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kmenu.h>
#include <kpropertiesdialog.h>
#include <kurlcompletion.h>
#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kconfiggroup.h>
#include <kfileitemactions.h>
#include <kfileitemlistproperties.h>
#include "filefinder.h"
#include "filewidget.h"
#include "kuickdata.h"
#include "kuickshow.h"

#ifdef KeyPress
#undef KeyPress
#endif

FileWidget::FileWidget( const KUrl& url, QWidget *parent )
    : KDirOperator( url, parent ),
      m_validCompletion( false ),
      m_fileFinder( 0L ),
      m_fileItemActions( 0L )
{
    setEnableDirHighlighting( true );

    KConfigGroup group(KGlobal::config(), "Filebrowser");
    setViewConfig( group );
    readConfig( group );
    setView( KFile::Default );

    // setOnlyDoubleClickSelectsFiles( true );
    reloadConfiguration();

    completionObject()->setCompletionMode( KGlobalSettings::CompletionAuto );
    dirCompletionObject()->setCompletionMode( KGlobalSettings::CompletionAuto);

    slotViewChanged();
    connect( this, SIGNAL( viewChanged( QAbstractItemView * )),
	     SLOT( slotViewChanged() ));

    connect( dirLister(), SIGNAL( clear() ), SLOT( slotItemsCleared() ));
    connect( dirLister(), SIGNAL( deleteItem( const KFileItem&  ) ),
	     SLOT( slotItemDeleted( const KFileItem& ) ));

    connect( this, SIGNAL( fileHighlighted( const KFileItem& )),
	     SLOT( slotHighlighted( const KFileItem& )));

    connect( this, SIGNAL(urlEntered(const KUrl&)),
             SLOT( slotURLEntered( const KUrl& )));

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
	KMimeType::List l = KMimeType::allMimeTypes();
	for (KMimeType::List::iterator it = l.begin(); it != l.end(); ++it)
	    if ((*it)->name().startsWith( "image/" ))
		mimes.append( (*it)->name() );

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

	if (m_fileFinder->completionMode() == KGlobalSettings::CompletionPopup ||
            m_fileFinder->completionMode() == KGlobalSettings::CompletionPopupAuto)
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
    	KUrl completeUrl = url();
    	completeUrl.setFileName( file );
    	KDirOperator::setCurrentItem( completeUrl.url() );
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


// KIO::NetAccess::stat() does NOT give us the right mimetype, while
// KIO::NetAccess::mimetype() does. So we have this hacklet to tell
// showImage that the KFileItem is really an image.
#define IS_IMAGE 5
#define MY_TYPE 55

bool FileWidget::isImage( const KFileItem& item )
{
//     return item && !item.isDir();
    if ( !item.isNull() )
    {
        return item.isReadable() && (item.mimetype().startsWith( "image/") ||
            item.extraData( (void*) MY_TYPE ) == (void*) IS_IMAGE);
    }
    return false;
}

void FileWidget::setImage( KFileItem& item, bool enable )
{
    if ( enable )
        item.setExtraData( (void*) MY_TYPE, (void*) IS_IMAGE );
    else
        item.removeExtraData( (void*) MY_TYPE );
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
	    setCurrentItem( item );
	return item;
    }

    return KFileItem();
}

KFileItem FileWidget::getPrevious( bool go )
{
    KFileItem item = getItem( Previous, true );
    if ( !item.isNull() ) {
	if ( go )
	    setCurrentItem( item );
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
    	kDebug() << "no current index" << endl;
        return KFileItem();
    }

    QModelIndex index = currentIndex;
    KFileItem item;
    const int column = index.column();
    item = fileItemFor( currentIndex );
    if ( item.isNull() )
        kDebug() << "### current item is null: " << index.data().typeName() << ", " << currentIndex.data().value<QString>() << endl;

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
    m_currentURL = QString::null;
}

void FileWidget::slotItemDeleted( const KFileItem& item )
{
    KFileItem current = getCurrentItem( false );
    if ( item != current ) {
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

	KUrl url;
	url.setPath( dir );
	setUrl( url, true );
    }

    else if ( text.indexOf('/') != (int) text.length() -1 ) { // relative path
	QString dir = m_fileFinder->completion()->replacedPath( text );
	KUrl u( url(), dir );
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

void FileWidget::setInitialItem( const KUrl& url )
{
    m_initialName = url;
}

void FileWidget::slotURLEntered( const KUrl& url )
{
    if ( m_fileFinder )
        m_fileFinder->completion()->setDir( url.path() );
}

void FileWidget::slotFinishedLoading()
{
	const KFileItem& current = getCurrentItem( false );
	if ( !m_initialName.isEmpty() )
		setCurrentItem( m_initialName.url() );
	else if ( current.isNull() ) {
		QModelIndex first = view()->model()->index(0, 0);
		if (first.isValid()) {
			KFileItem item = first.data(Qt::UserRole).value<KFileItem>();
			if (!item.isNull()) {
				setCurrentItem( item );
			}
		}
	}

	m_initialName = KUrl();
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

#include "filewidget.moc"
