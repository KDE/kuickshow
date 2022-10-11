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

#include <KIconLoader>
#include <KLocalizedString>

#include <QDesktopServices>
#include <QPixmap>
#include <QUrl>

#include "kuickdata.h"
#include "imdata.h"
#include "imlibparams.h"


GeneralWidget::GeneralWidget( QWidget *parent )
  : QWidget( parent )
{
  // setup the widget based on its .ui file
  ui = new Ui::GeneralWidget;
  ui->setupUi(this);


  // now the properties that couldn't be set in the .ui file

  // the KuickShow logo
  QPixmap pixmap = KIconLoader::global()->loadIcon("logo", KIconLoader::User);
  ui->logo->setUrl( "http://devel-home.kde.org/~pfeiffer/kuickshow/" );
  ui->logo->setPixmap( pixmap );
  ui->logo->setFixedSize( pixmap.size() );

  // actions
  connect( ui->logo, SIGNAL( leftClickedUrl( const QString & ) ),
            SLOT( slotURLClicked( const QString & ) ) );

  connect( ui->cbOwnPalette, SIGNAL( clicked() ), this, SLOT( useOwnPalette() ) );


  // load and show the saved settings
  loadSettings();
  ui->cbFullscreen->setFocus();
}

GeneralWidget::~GeneralWidget()
{
  delete ui;
}

void GeneralWidget::slotURLClicked( const QString & url )
{
    QDesktopServices::openUrl(QUrl::fromUserInput(url));
}

void GeneralWidget::loadSettings(const KuickData *kdata, const ImData *idata)
{
    if (kdata==nullptr) kdata = ImlibParams::kuickConfig();	// normal, unless resetting to defaults
    if (idata==nullptr) idata = ImlibParams::imlibConfig();

    ui->colorButton->setColor( kdata->backgroundColor );
    ui->editFilter->setText( kdata->fileFilter );
    ui->cbFullscreen->setChecked( kdata->fullScreen );
    ui->cbPreload->setChecked( kdata->preloadImage );
    ui->cbLastdir->setChecked( kdata->startInLastDir );
    ui->cbFastRemap->setChecked( idata->fastRemap );
    ui->cbOwnPalette->setChecked( idata->ownPalette );
    ui->cbSmoothScale->setChecked( idata->smoothScale );
    ui->cbFastRender->setChecked( idata->fastRender );
    ui->cbDither16bit->setChecked( idata->dither16bit );
    ui->cbDither8bit->setChecked( idata->dither8bit );
    ui->maxCacheSpinBox->setValue( idata->maxCache / 1024 );

    useOwnPalette(); // enable/disable remap-checkbox
}

void GeneralWidget::applySettings()
{
    KuickData *kdata = ImlibParams::kuickConfig();
    ImData *idata = ImlibParams::imlibConfig();

    kdata->backgroundColor = ui->colorButton->color();
    kdata->fileFilter      = ui->editFilter->text();
    kdata->fullScreen  	  = ui->cbFullscreen->isChecked();
    kdata->preloadImage	  = ui->cbPreload->isChecked();
    kdata->startInLastDir   = ui->cbLastdir->isChecked();

    idata->smoothScale    = ui->cbSmoothScale->isChecked();
    idata->fastRemap 	  = ui->cbFastRemap->isChecked();
    idata->ownPalette 	  = ui->cbOwnPalette->isChecked();
    idata->fastRender 	  = ui->cbFastRender->isChecked();
    idata->dither16bit 	  = ui->cbDither16bit->isChecked();
    idata->dither8bit 	  = ui->cbDither8bit->isChecked();

    idata->maxCache	  = (uint) ui->maxCacheSpinBox->value() * 1024;
}

void GeneralWidget::useOwnPalette()
{
    ui->cbFastRemap->setEnabled( ui->cbOwnPalette->isChecked() );
}
