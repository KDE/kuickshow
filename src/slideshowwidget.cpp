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

#include "kuickconfig.h"


SlideShowWidget::SlideShowWidget( QWidget *parent )
    : QWidget( parent )
{
    // setup the widget based on its .ui file
    ui = new Ui::SlideShowWidget;
    ui->setupUi(this);

    loadSettings();
}

SlideShowWidget::~SlideShowWidget()
{
    delete ui;
}

void SlideShowWidget::loadSettings(const KuickConfig* config)
{
    if (config == nullptr) config = &KuickConfig::get();

    ui->delayTime->setValue(config->slideDelay / 1000);
    ui->cycles->setValue(config->slideshowCycles);
    ui->cycles->setSpecialValueText(i18nc("Run the slideshow until manually stopped", "Forever"));
    ui->fullScreen->setChecked(config->slideshowFullscreen);
    ui->startWithCurrent->setChecked(!config->slideshowStartAtFirst);
}

void SlideShowWidget::applySettings()
{
    KuickConfig& config = KuickConfig::get();

    config.slideDelay = ui->delayTime->value() * 1000;
    config.slideshowCycles = ui->cycles->value();
    config.slideshowFullscreen = ui->fullScreen->isChecked();
    config.slideshowStartAtFirst = !ui->startWithCurrent->isChecked();
}
