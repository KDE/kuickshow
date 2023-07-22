/* This file is part of the KDE project
   Copyright (C) 1998-2002 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include "generalwidget.h"
#include <ui_generalwidget.h>

#include "kuickconfig.h"

#include <KIconLoader>
#include <KLocalizedString>

#include <QDesktopServices>
#include <QPixmap>
#include <QUrl>


GeneralWidget::GeneralWidget( QWidget *parent )
  : QWidget( parent )
{
  // setup the widget based on its .ui file
  ui = new Ui::GeneralWidget;
  ui->setupUi(this);


  // now the properties that couldn't be set in the .ui file

  // the KuickShow logo
  QPixmap pixmap = KIconLoader::global()->loadIcon("logo", KIconLoader::User);
  ui->logo->setUrl(HOMEPAGE_URL);
  ui->logo->setPixmap( pixmap );
  ui->logo->setFixedSize( pixmap.size() );

  // actions
  connect(ui->logo, QOverload<>::of(&KUrlLabel::leftClickedUrl), this, &GeneralWidget::slotURLClicked);
  connect(ui->cbOwnPalette, &QAbstractButton::clicked, this, &GeneralWidget::useOwnPalette);

#ifdef HAVE_QTONLY
  // Disable GUI controls that are only relevant for Imlib
  ui->cbFastRemap->setEnabled(false);
  ui->cbOwnPalette->setEnabled(false);
  ui->cbFastRender->setEnabled(false);
  ui->cbDither16bit->setEnabled( false );
  ui->cbDither8bit->setEnabled( false );
#endif // HAVE_QTONLY

  // load and show the saved settings
  loadSettings();
  ui->cbFullscreen->setFocus();
}

GeneralWidget::~GeneralWidget()
{
  delete ui;
}

void GeneralWidget::slotURLClicked()
{
    QDesktopServices::openUrl(QUrl::fromUserInput(ui->logo->url()));
}

void GeneralWidget::loadSettings(const KuickConfig* config)
{
    if (config == nullptr) config = &KuickConfig::get();

    ui->colorButton->setColor(config->backgroundColor);
    ui->editFilter->setText(config->fileFilter);
    ui->cbFullscreen->setChecked(config->fullScreen);
    ui->cbPreload->setChecked(config->preloadImage);
    ui->cbLastdir->setChecked(config->startInLastDir);
    ui->cbFastRemap->setChecked(config->fastRemap);
    ui->cbOwnPalette->setChecked(config->ownPalette);
    ui->cbSmoothScale->setChecked(config->smoothScale);
    ui->cbFastRender->setChecked(config->fastRender);
    ui->cbDither16bit->setChecked(config->dither16bit);
    ui->cbDither8bit->setChecked(config->dither8bit);
    ui->maxCacheSpinBox->setValue(config->maxCache / 1024);

    useOwnPalette(); // enable/disable remap-checkbox
}

void GeneralWidget::applySettings()
{
    KuickConfig& config = KuickConfig::get();

    config.backgroundColor = ui->colorButton->color();
    config.fileFilter      = ui->editFilter->text();
    config.fullScreen  	  = ui->cbFullscreen->isChecked();
    config.preloadImage	  = ui->cbPreload->isChecked();
    config.startInLastDir   = ui->cbLastdir->isChecked();

    config.smoothScale    = ui->cbSmoothScale->isChecked();
    config.fastRemap 	  = ui->cbFastRemap->isChecked();
    config.ownPalette 	  = ui->cbOwnPalette->isChecked();
    config.fastRender 	  = ui->cbFastRender->isChecked();
    config.dither16bit 	  = ui->cbDither16bit->isChecked();
    config.dither8bit 	  = ui->cbDither8bit->isChecked();

    config.maxCache	  = static_cast<uint>(ui->maxCacheSpinBox->value() * 1024);
}

void GeneralWidget::useOwnPalette()
{
#ifndef HAVE_QTONLY
    // Keep the GUI control disabled if it is not relevant
    ui->cbFastRemap->setEnabled( ui->cbOwnPalette->isChecked() );
#endif // HAVE_QTONLY
}
