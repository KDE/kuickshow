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

#include "defaultswidget.h"
#include <ui_defaultswidget.h>

#include <QStandardPaths>
#include <QUrl>

#include "imlibwidget.h"
#include "imlibparams.h"


DefaultsWidget::DefaultsWidget( QWidget *parent )
  : QWidget( parent )
{
  // setup the widget based on its .ui file
  ui = new Ui::DefaultsWidget;
  ui->setupUi(this);

  // set the properties that couldn't be set in the .ui file
  QGridLayout *gbPreviewLayout = static_cast<QGridLayout *>(ui->gbPreview->layout());

  // The image widgets have to be created here,
  // because the required parameters can only be set on creation.
  // The generated code won't do that.
  imOrig = new ImlibWidget(ui->gbPreview);
  imOrig->setUseModifications(false);
  gbPreviewLayout->addWidget(imOrig, 1, 0, Qt::AlignCenter | Qt::AlignTop);

  imFiltered = new ImlibWidget(ui->gbPreview);
  gbPreviewLayout->addWidget(imFiltered, 1, 1, Qt::AlignCenter | Qt::AlignTop);

  // actions
  connect( ui->cbEnableMods, SIGNAL( toggled(bool) ), SLOT( enableWidgets(bool) ));

  connect(ui->cbUpScale, SIGNAL( toggled(bool)), ui->sbMaxUpScaleFactor,
            SLOT( setEnabled(bool) ));

  // TODO: why is this necessary?  Widget cannot delete itself.
  connect( imFiltered, SIGNAL( destroyed() ), SLOT( slotNoImage() ));

  connect( ui->cbDownScale,        SIGNAL( clicked() ), SLOT( updatePreview() ));
  connect( ui->cbUpScale,          SIGNAL( clicked() ), SLOT( updatePreview() ));
  connect( ui->cbFlipVertically,   SIGNAL( clicked() ), SLOT( updatePreview() ));
  connect( ui->cbFlipHorizontally, SIGNAL( clicked() ), SLOT( updatePreview() ));
  connect( ui->sbMaxUpScaleFactor, SIGNAL( valueChanged(int) ), SLOT( updatePreview() ));
  connect( ui->sbBrightness, SIGNAL( valueChanged(int) ), SLOT( updatePreview() ));
  connect( ui->sbContrast,   SIGNAL( valueChanged(int) ), SLOT( updatePreview() ));
  connect( ui->sbGamma,      SIGNAL( valueChanged(int) ), SLOT( updatePreview() ));

  connect( ui->comboRotate,  SIGNAL( activated(int) ), SLOT( updatePreview() ));

  // load and display the test image
  QString filename = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("pics/calibrate.png"));
  if ( !imOrig->loadImage( QUrl::fromLocalFile(filename) ) )
    imOrig = 0L; // FIXME - display some errormessage!
  if ( !imFiltered->loadImage( QUrl::fromLocalFile(filename) ) )
    imFiltered = 0L; // FIXME - display some errormessage!

  loadSettings();

  if (imOrig!=nullptr) imOrig->setFixedSize(imOrig->sizeHint());
  if (imFiltered!=nullptr) imFiltered->setFixedSize(imFiltered->sizeHint());
}


DefaultsWidget::~DefaultsWidget()
{
    // those need to be deleted in the right order, as imFiltered
    // references ImlibData from imOrig
    delete imFiltered;
    delete imOrig;

    delete ui;
}

void DefaultsWidget::loadSettings(const KuickData *kdata, const ImData *idata)
{
    if (kdata==nullptr) kdata = ImlibParams::kuickConfig();	// normal, unless resetting to defaults
    if (idata==nullptr) idata = ImlibParams::imlibConfig();

    ui->cbDownScale->setChecked( kdata->downScale );
    ui->cbUpScale->setChecked( kdata->upScale );
    ui->sbMaxUpScaleFactor->setValue( kdata->maxUpScale );

    ui->cbFlipVertically->setChecked( kdata->flipVertically );
    ui->cbFlipHorizontally->setChecked( kdata->flipHorizontally );
    ui->comboRotate->setCurrentIndex(static_cast<int>(data.rotation));

    ui->sbBrightness->setValue( idata->brightness );
    ui->sbContrast->setValue( idata->contrast );
    ui->sbGamma->setValue( idata->gamma );

    ui->cbEnableMods->setChecked( kdata->isModsEnabled );
    enableWidgets( kdata->isModsEnabled );

    updatePreview();
}


void DefaultsWidget::applySettings()
{
    KuickData *kdata = ImlibParams::kuickConfig();
    ImData *idata = ImlibParams::imlibConfig();

    kdata->isModsEnabled = ui->cbEnableMods->isChecked();

    kdata->downScale  = ui->cbDownScale->isChecked();
    kdata->upScale    = ui->cbUpScale->isChecked();
    kdata->maxUpScale = ui->sbMaxUpScaleFactor->value();

    kdata->flipVertically   = ui->cbFlipVertically->isChecked();
    kdata->flipHorizontally = ui->cbFlipHorizontally->isChecked();
    kdata->rotation = currentRotation();

    idata->brightness = ui->sbBrightness->value();
    idata->contrast   = ui->sbContrast->value();
    idata->gamma      = ui->sbGamma->value();
}

void DefaultsWidget::updatePreview()
{
    if ( !imFiltered )
	return;

    imFiltered->setAutoRender( false );

    int flipMode = ui->cbFlipHorizontally->isChecked() ? FlipHorizontal : FlipNone;
    flipMode |= ui->cbFlipVertically->isChecked() ? FlipVertical : FlipNone;
    imFiltered->setFlipMode( flipMode );

    Rotation rotation = ui->cbEnableMods->isChecked() ? currentRotation() : ROT_0;
    imFiltered->setRotation( rotation );

    imFiltered->initModifications();			// set absolute values
    imFiltered->stepBrightness(ui->sbBrightness->value());
    imFiltered->stepContrast(ui->sbContrast->value());
    imFiltered->stepGamma(ui->sbGamma->value());

    imFiltered->updateImage();
    imFiltered->setAutoRender( true );
}


void DefaultsWidget::enableWidgets( bool enable )
{
    ui->gbScale->setEnabled( enable );
    ui->sbMaxUpScaleFactor->setEnabled( enable & ui->cbUpScale->isChecked() );

    ui->gbGeometry->setEnabled( enable );
    ui->gbAdjust->setEnabled( enable );
    ui->gbPreview->setEnabled( enable );
    updatePreview();
}


Rotation DefaultsWidget::currentRotation() const
{
    return (static_cast<Rotation>(ui->comboRotate->currentIndex()));
}
