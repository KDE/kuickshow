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

#include "slideshowwidget.h"
#include <ui_slideshowwidget.h>

#include "kuickdata.h"


SlideShowWidget::SlideShowWidget( QWidget *parent )
    : QWidget( parent )
{
    // setup the widget based on its .ui file
    ui = new Ui::SlideShowWidget;
    ui->setupUi(this);

    loadSettings( *kdata );
}

SlideShowWidget::~SlideShowWidget()
{
    delete ui;
}

void SlideShowWidget::loadSettings( const KuickData& data )
{
    ui->delayTime->setValue( data.slideDelay / 1000 );
    ui->cycles->setValue( data.slideshowCycles );
    ui->fullScreen->setChecked( data.slideshowFullscreen );
    ui->startWithCurrent->setChecked( !data.slideshowStartAtFirst );
}

void SlideShowWidget::applySettings( KuickData& data )
{
    data.slideDelay = ui->delayTime->value() * 1000;
    data.slideshowCycles = ui->cycles->value();
    data.slideshowFullscreen = ui->fullScreen->isChecked();
    data.slideshowStartAtFirst = !ui->startWithCurrent->isChecked();
}
