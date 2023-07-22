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
  connect(ui->cbEnableMods, &QAbstractButton::toggled, this, &DefaultsWidget::enableWidgets);
  connect(ui->cbUpScale, &QAbstractButton::toggled, ui->sbMaxUpScaleFactor, &QWidget::setEnabled);
  // TODO: why is this necessary?  Widget cannot delete itself.
  connect(imFiltered, &QObject::destroyed, this, [this]() { imFiltered = nullptr; });

  connect(ui->cbDownScale, &QAbstractButton::clicked, this, &DefaultsWidget::updatePreview);
  connect(ui->cbUpScale, &QAbstractButton::clicked, this, &DefaultsWidget::updatePreview);
  connect(ui->cbFlipVertically, &QAbstractButton::clicked, this, &DefaultsWidget::updatePreview);
  connect(ui->cbFlipHorizontally, &QAbstractButton::clicked, this, &DefaultsWidget::updatePreview);
  connect(ui->sbMaxUpScaleFactor, QOverload<int>::of(&QSpinBox::valueChanged), this, &DefaultsWidget::updatePreview);
  connect(ui->sbBrightness, QOverload<int>::of(&QSpinBox::valueChanged), this, &DefaultsWidget::updatePreview);
  connect(ui->sbContrast, QOverload<int>::of(&QSpinBox::valueChanged), this, &DefaultsWidget::updatePreview);
  connect(ui->sbGamma, QOverload<int>::of(&QSpinBox::valueChanged), this, &DefaultsWidget::updatePreview);
  connect(ui->comboRotate, QOverload<int>::of(&QComboBox::activated), this, &DefaultsWidget::updatePreview);

  // load and display the test image
  QString filename = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("pics/calibrate.png"));
  if ( !imOrig->loadImage( QUrl::fromLocalFile(filename) ) )
    imOrig = nullptr; // FIXME - display some errormessage!
  if ( !imFiltered->loadImage( QUrl::fromLocalFile(filename) ) )
    imFiltered = nullptr; // FIXME - display some errormessage!

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

void DefaultsWidget::loadSettings(const KuickConfig* config)
{
    if (config == nullptr) config = &KuickConfig::get();

    ui->cbDownScale->setChecked(config->downScale);
    ui->cbUpScale->setChecked(config->upScale);
    ui->sbMaxUpScaleFactor->setValue(config->maxUpScale);

    ui->cbFlipVertically->setChecked(config->flipVertically);
    ui->cbFlipHorizontally->setChecked(config->flipHorizontally);
    ui->comboRotate->setCurrentIndex(static_cast<int>(config->rotation));

    ui->sbBrightness->setValue(config->brightness);
    ui->sbContrast->setValue(config->contrast);
    ui->sbGamma->setValue(config->gamma);

    ui->cbEnableMods->setChecked(config->isModsEnabled);
    enableWidgets(config->isModsEnabled);

    updatePreview();
}


void DefaultsWidget::applySettings()
{
    KuickConfig& config = KuickConfig::get();

    config.isModsEnabled = ui->cbEnableMods->isChecked();

    config.downScale  = ui->cbDownScale->isChecked();
    config.upScale    = ui->cbUpScale->isChecked();
    config.maxUpScale = ui->sbMaxUpScaleFactor->value();

    config.flipVertically   = ui->cbFlipVertically->isChecked();
    config.flipHorizontally = ui->cbFlipHorizontally->isChecked();
    config.rotation = currentRotation();

    config.brightness = ui->sbBrightness->value();
    config.contrast   = ui->sbContrast->value();
    config.gamma      = ui->sbGamma->value();
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
#ifdef HAVE_QTONLY
  // Disable GUI control groups that are only relevant for Imlib
    enable = false;
#endif // HAVE_QTONLY
    ui->gbAdjust->setEnabled( enable );
    ui->gbPreview->setEnabled( enable );
    updatePreview();
}


Rotation DefaultsWidget::currentRotation() const
{
    return (static_cast<Rotation>(ui->comboRotate->currentIndex()));
}
