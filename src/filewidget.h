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

#ifndef FILEWIDGET_H
#define FILEWIDGET_H

#include <KDirModel>
#include <KDirOperator>

#include <QUrl>

class KFileItem;
class KFileItemActions;
class QEvent;
class QResizeEvent;
class FileFinder;
class KuickShow;


class FileWidget : public KDirOperator
{
    Q_OBJECT

public:
    enum WhichItem { Previous, Next, Current };

    FileWidget(const QUrl& url, KuickShow *parent);
    ~FileWidget();

    bool hasFiles() const;
    void reloadConfiguration();

    void setInitialItem( const QUrl& url );

    KFileItem getCurrentItem( bool onlyImage ) const {
	return getItem( Current, onlyImage );
    }

    KFileItem gotoFirstImage();
    KFileItem gotoLastImage();
    KFileItem getNext( bool go=true );
    KFileItem getPrevious( bool go=true );


    KFileItem getItem( WhichItem which, bool onlyImage ) const;

    static bool isImage( const KFileItem& );


Q_SIGNALS:
    void finished();

protected:
    virtual bool eventFilter( QObject *o, QEvent * ) override;
    virtual void resizeEvent( QResizeEvent * ) override;
    virtual QSize sizeHint() const override;

private Q_SLOTS:
    void slotReturnPressed( const QString& text );
    void findCompletion( const QString& );
    void slotViewChanged();

    void slotItemsCleared();
    void slotItemsDeleted( const KFileItemList& );
    void slotHighlighted( const KFileItem& );
    void slotContextMenu( const KFileItem &, QMenu *);

    void slotURLEntered( const QUrl& url );
    void slotFinishedLoading();

private:
	void initializeActionNames();

    KFileItem fileItemFor(const QModelIndex& index) const {
        if (index.isValid()) {
            return index.data(KDirModel::FileItemRole).value<KFileItem>();
	}
	return KFileItem();
    }

    bool m_validCompletion;
    FileFinder *m_fileFinder;
    QString m_currentURL;
    QUrl m_initialName;
    KFileItemActions *m_fileItemActions;

	bool contextMenuInitialized;
	void initializeContextMenu(QMenu* popupMenu);

	int numItemSpecificContextMenuItems;
	void addItemSpecificContextMenuItems(QMenu* popupMenu, const KFileItem& item);
	void removeItemSpecificContextMenuItems(QMenu* popupMenu);

	static bool EnsureLastMenuItemIsSeparator(QMenu* menu, QAction* before);
};


#endif // FILEWIDGET_H
