/****************************************************************************
** $Id$
**
** Created : 98
**
** Copyright (C) 1998-2001 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/

#include <qkeycode.h>

#include <kapplication.h> // KDE_VERSION
#include <kglobal.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kpropertiesdialog.h>
#include <kurlcompletion.h>

#include "filefinder.h"
#include "filewidget.h"
#include "kuickdata.h"
#include "kuickshow.h"

#ifdef KeyPress
#undef KeyPress
#endif

FileWidget::FileWidget( const KURL& url, QWidget *parent, const char *name )
    : KDirOperator( url, parent, name ),
      m_validCompletion( false ),
      m_fileFinder( 0L )
{
    setEnableDirHighlighting( true );

    readConfig( KGlobal::config(), "Filebrowser" );
    setView( KFile::Default );

    // setOnlyDoubleClickSelectsFiles( true );
    reloadConfiguration();

    completionObject()->setCompletionMode( KGlobalSettings::CompletionAuto );
    dirCompletionObject()->setCompletionMode( KGlobalSettings::CompletionAuto);

    slotViewChanged();
    connect( this, SIGNAL( viewChanged( KFileView * )),
	     SLOT( slotViewChanged() ));

    connect( dirLister(), SIGNAL( clear() ), SLOT( slotItemsCleared() ));
    connect( dirLister(), SIGNAL( deleteItem( KFileItem * ) ),
	     SLOT( slotItemDeleted( KFileItem *) ));

    connect( this, SIGNAL( fileHighlighted( const KFileItem * )),
	     SLOT( slotHighlighted( const KFileItem * )));

    // should actually be KDirOperator's job!
    connect( this, SIGNAL( finishedLoading() ), SLOT( slotFinishedLoading() ));
}

FileWidget::~FileWidget()
{
    delete m_fileFinder;
}

void FileWidget::initActions()
{
    int index = 0;
    KActionCollection *coll = actionCollection();
    KActionSeparator *sep = new KActionSeparator( coll );
    KActionMenu *menu = static_cast<KActionMenu*>( coll->action("popupMenu") );

    menu->insert( coll->action("kuick_showInOtherWindow"), index++ );
    menu->insert( coll->action("kuick_showInSameWindow"), index++ );
    menu->insert( sep, index++ );

    // support for older kdelibs, remove somewhen...
    if ( coll->action("kuick_delete") )
        menu->insert( coll->action("kuick_delete"), 9 );

    // properties dialog is now in kfile, but not at the right position,
    // so we move it to the real bottom
    menu->remove( coll->action( "properties" ) );

    QPopupMenu *pMenu = menu->popupMenu();
    int lastItemId = pMenu->idAt( pMenu->count() - 1 );
    QMenuItem *mItem = pMenu->findItem( lastItemId );
    if ( mItem && !mItem->isSeparator() )
        menu->insert( sep );

    // those at the bottom
    menu->insert( coll->action("kuick_print") );
    menu->insert( sep );
    menu->insert( coll->action("properties") );
}

void FileWidget::reloadConfiguration()
{
    if ( kdata->fileFilter != nameFilter() ) {
	setNameFilter( kdata->fileFilter );
	updateDir();
    }
}

bool FileWidget::hasFiles() const 
{
    return (numFiles() > 0);
}

void FileWidget::activatedMenu( const KFileItem *item, const QPoint& pos )
{
    bool image = isImage( item );
    actionCollection()->action("kuick_showInSameWindow")->setEnabled( image );
    actionCollection()->action("kuick_showInOtherWindow")->setEnabled( image );
    actionCollection()->action("kuick_print")->setEnabled( image );
    actionCollection()->action("properties")->setEnabled( item );

    bool hasSelection = (item != 0L);
    if ( actionCollection()->action("kuick_delete") )
        actionCollection()->action("kuick_delete")->setEnabled( hasSelection );

    KDirOperator::activatedMenu( item, pos );
}

void FileWidget::findCompletion( const QString& text )
{
    if ( text.at(0) == '/' || text.at(0) == '~' ||
	 text.find('/') != -1 ) {
	QString t = m_fileFinder->completionObject()->makeCompletion( text );

	if (m_fileFinder->completionMode() == KGlobalSettings::CompletionPopup ||
            m_fileFinder->completionMode() == KGlobalSettings::CompletionPopupAuto)
	    m_fileFinder->setCompletedItems(
			      m_fileFinder->completionObject()->allMatches() );
	else
            if ( !t.isNull() )
                m_fileFinder->setCompletedText( t );

	return;
    }
	
    QString file = makeDirCompletion( text );
    if ( file.isNull() )
	file = makeCompletion( text );

    m_validCompletion = !file.isNull();

    if ( m_validCompletion )
	KDirOperator::setCurrentItem( file );
}

bool FileWidget::eventFilter( QObject *o, QEvent *e )
{
    if ( e->type() == QEvent::KeyPress ) {
	QKeyEvent *k = static_cast<QKeyEvent*>( e );
	
	if ( (k->state() & (ControlButton | AltButton)) == 0 ) {
	    int key = k->key();
	    if ( key == Key_Space && !KuickShow::s_viewers.isEmpty() ) {
		topLevelWidget()->hide();
                k->accept();
		return true;
	    }
	    else if ( key == Key_Delete ) {
                k->accept();
		KFileItem *item = getCurrentItem( false );
		if ( item ) {
                    KFileItemList list;
                    list.append( item );
		    del( list, (k->state() & ShiftButton) == 0 );
                }
		return true;
	    }
	
	    const QString& text = k->text();
	    if ( !text.isEmpty() && text.unicode()->isPrint() ) {
                k->accept();
		
                if ( !m_fileFinder ) {
		    m_fileFinder = new FileFinder( this, "file finder" );
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

bool FileWidget::isImage( const KFileItem *item )
{
    return item && !item->isDir();

//     return ( item && item->isReadable() &&
//  	     item->mimetype().startsWith( "image/") );
}
	

KFileItem * FileWidget::gotoFirstImage()
{
    KFileItemListIterator it( *(fileView()->items()) );

    while ( it.current() ) {
	if ( isImage( it.current() ) ) {
	    setCurrentItem( it.current() );
	    return it.current();
	}
	++it;
    }

    return 0L;
}

KFileItem * FileWidget::gotoLastImage()
{
    KFileItemListIterator it( *(fileView()->items()) );
    it.toLast();

    while ( it.current() ) {
	if ( isImage( it.current() ) ) {
	    setCurrentItem( it.current() );
	    return it.current();
	}
	--it;
    }

    return 0L;
}

KFileItem * FileWidget::getNext( bool go )
{
    KFileItem *item = getItem( Next, true );
    if ( item ) {
	if ( go )
	    setCurrentItem( item );
	return item;
    }

    return 0L;
}

KFileItem * FileWidget::getPrevious( bool go )
{
    KFileItem *item = getItem( Previous, true );
    if ( item ) {
	if ( go )
	    setCurrentItem( item );
	return item;
    }

    return 0L;
}

// returns 0L when there is no previous/next item/image
// this sucks! Use KFileView::currentFileItem() when implemented
KFileItem * FileWidget::getItem( WhichItem which, bool onlyImage ) const
{
    KFileItemListIterator it( *(fileView()->items()) );

    while ( it.current() ) { // find the iterator to the current item
	if ( it.current()->url() == m_currentURL )
	    break;

	++it;
    }

    if ( it.current() ) {
	switch ( which ) {
	case Previous: {
	    --it;
	    while ( it.current() ) {
		if ( isImage( it.current() ) || !onlyImage )
		    return it.current();
		--it;
	    }
	    return 0L; // no previous item / image
	}

	case Next: {
	    ++it;
	    while ( it.current() ) {
		if ( isImage( it.current() ) || !onlyImage )
		    return it.current();
		++it;
	    }
	    return 0L; // no further item / image
	}
	
	case Current:
	default:
	    return it.current();
	}
    }

    return 0L;
}

void FileWidget::slotViewChanged()
{
    fileView()->widget()->installEventFilter( this );
}

void FileWidget::slotItemsCleared()
{
    m_currentURL = QString::null;
}

void FileWidget::slotItemDeleted( KFileItem *item )
{
    KFileItem *current = getCurrentItem( false );
    if ( item != current ) {
	return; // all ok, we already have a new current item
    }

    KFileItem *next = getNext();
    if ( !next )
	next = getPrevious();

    if ( next )
	m_currentURL = next->url().url();
}

void FileWidget::slotHighlighted( const KFileItem *item )
{
    m_currentURL = item->url().url();
}

void FileWidget::slotReturnPressed( const QString& t )
{
    // we need a / at the end, otherwise replacedPath() will cut off the dir,
    // assuming it is a filename
    QString text = t;
    if ( text.at( text.length()-1 ) != '/' )
	text += '/';

    if ( text.at(0) == '/' || text.at(0) == '~' ) {
	QString dir = (static_cast<KURLCompletion*>( m_fileFinder->completionObject() ))->replacedPath( text );
	
	KURL url;
	url.setPath( dir );
	setURL( url, true );
    }

    else if ( text.find('/') != (int) text.length() -1 ) { // relative path
	QString dir = (static_cast<KURLCompletion*>( m_fileFinder->completionObject() ))->replacedPath( text );
	KURL u = url();
	u.addPath( dir );
	setURL( u, true );
    }

    else if ( m_validCompletion ) {
	KFileItem *item = getCurrentItem( true );

	if ( item ) {
	    if ( item->isDir() )
		setURL( item->url(), true );
	    else
		emit fileSelected( item );
	}
    }
}

void FileWidget::setCurrentItem( const KFileItem *item )
{
    if ( item ) {
        fileView()->setCurrentItem( item );
	fileView()->ensureItemVisible( item );
    }
}

void FileWidget::setInitialItem( const QString& filename )
{
    m_initialName = filename;
}

void FileWidget::slotFinishedLoading()
{
    KFileItem *current = getCurrentItem( false );
    if ( !m_initialName.isEmpty() )
	setCurrentItem( m_initialName );
    else if ( !current )
	setCurrentItem( view()->items()->getFirst() );

    m_initialName = QString::null;
    emit finished();
}

void FileWidget::resizeEvent( QResizeEvent *e )
{
    KDirOperator::resizeEvent( e );
    if ( m_fileFinder )
	m_fileFinder->move( width()  - m_fileFinder->width(),
			    height() - m_fileFinder->height() );
}

#include "filewidget.moc"
