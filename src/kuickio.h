/* This file is part of the KDE project
   Copyright (C) 2000,2001 Carsten Pfeiffer <pfeiffer@kde.org>

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

#ifndef KUICKIO_H
#define KUICKIO_H

class QWidget;

#include <qobject.h>

#include <kurl.h>

class KuickIO : public QObject
{
    Q_OBJECT

public:
    static KuickIO * self( QWidget *parent );

private:
    static QWidget * s_parent;
    static KuickIO * s_self;
};

#endif // KUICKIO_H
