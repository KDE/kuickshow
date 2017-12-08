/* This file is part of the KDE project
   Copyright (C) 1998,1999,2000 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include "openfilesanddirsdialog.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QSignalMapper>


OpenFilesAndDirsDialog::OpenFilesAndDirsDialog(QWidget* parent, const QString& caption) : QFileDialog(parent, caption)
{
    // native dialogs usually don't support selecting files AND directories, so we can't use them
    setOption(QFileDialog::DontUseNativeDialog);

    // allow selection of directories
    setFileMode(QFileDialog::Directory);


    // The following code highly depends on the internals of QFileDialog.

    // find the button box within this dialog, so we can connect to the "Open" button
    QDialogButtonBox* bbox = findChild<QDialogButtonBox*>();
    if(bbox && (buttonOpen = bbox->button(QDialogButtonBox::Open))) {
        //buttonOpen->disconnect(SIGNAL(clicked()));
        connect(buttonOpen, SIGNAL(clicked()), SLOT(okClicked()));
    } else {
        qWarning("couldn't find QDialogButtonBox in QFileDialog; selection will not work correctly");
    }

    // find the selection controls
    QSignalMapper* mapper = new QSignalMapper(this);
    for(auto view : findChildren<QAbstractItemView*>(QRegExp("^(?:treeView|listView)$"))) {
        // allow selection of multiple entries
        view->setSelectionMode(QAbstractItemView::ExtendedSelection);

        // we have to manually set the enabled-state of the "Open" button when the selection changes, because
        // the default only enables it when a directory is selected
        auto model = view->selectionModel();
        connect(model, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), mapper, SLOT(map()));
        mapper->setMapping(model, view);
    }
    connect(mapper, SIGNAL(mapped(QWidget*)), SLOT(onSelectionChange(QWidget*)));
}

OpenFilesAndDirsDialog::~OpenFilesAndDirsDialog()
{
}


void OpenFilesAndDirsDialog::onSelectionChange(QWidget* obj)
{
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(obj);
    if(view) {
        auto slist = view->selectionModel()->selectedIndexes();
        buttonOpen->setEnabled(!slist.isEmpty());
    }
}


void OpenFilesAndDirsDialog::okClicked()
{
    if(!selectedFiles().isEmpty()) QDialog::accept();
}
