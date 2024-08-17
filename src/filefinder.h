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

#ifndef FILEFINDER_H
#define FILEFINDER_H

#include <KLineEdit>
#include <KUrlCompletion>

class QFocusEvent;
class QKeyEvent;
class QWidget;


class FileFinder : public KLineEdit
{
    Q_OBJECT

public:
    explicit FileFinder(QWidget* parent = nullptr);
    ~FileFinder();

    KUrlCompletion *completion() {
        return static_cast<KUrlCompletion*>( completionObject() );
    }

    virtual void hide();

Q_SIGNALS:
    void enterDir( const QString& );

protected:
    virtual void focusOutEvent( QFocusEvent * ) override;
    virtual void keyPressEvent( QKeyEvent * ) override;

private Q_SLOTS:
    void slotAccept( const QString& );
};

#endif // FILEFINDER_H
