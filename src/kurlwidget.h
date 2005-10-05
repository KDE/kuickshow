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

#ifndef KURLWIDGET_H
#define KURLWIDGET_H

#include <kurllabel.h>

class KURLWidget : public KURLLabel
{
    Q_OBJECT

public:
    KURLWidget( const QString& text, QWidget *, const char *name=0 );

protected slots:
    virtual void run();

};

#endif
