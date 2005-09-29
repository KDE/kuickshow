/* This file is part of the KDE project
   Copyright (C) 2002 Carsten Pfeiffer <pfeiffer@kde.org>

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

#ifndef SLIDESHOWWIDGET_H
#define SLIDESHOWWIDGET_H

#include "kuickdata.h"

class QCheckBox;
class KIntNumInput;

class SlideShowWidget : public QWidget
{
    Q_OBJECT
public:
    SlideShowWidget( QWidget *parent, const char *name );
    ~SlideShowWidget();

    virtual void loadSettings( const KuickData& data );
    virtual void applySettings( KuickData& data );

private:
    KIntNumInput *m_delayTime;
    KIntNumInput *m_cycles;
    QCheckBox    *m_fullScreen;
    QCheckBox    *m_startWithCurrent;
};

#endif // SLIDESHOWWIDGET_H
