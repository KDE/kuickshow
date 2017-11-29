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

#include <KComboBox>
#include <KDialog>
#include <KLocale>
#include <KNumInput>

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

#include "imlibwidget.h"


DefaultsWidget::DefaultsWidget( QWidget *parent )
  : QWidget( parent )
{
  // setup the widget based on its .ui file
  ui = new Ui::DefaultsWidget;
  ui->setupUi(this);


  // set the properties that couldn't be set in the .ui file
  QGridLayout* gbPreviewLayout = dynamic_cast<QGridLayout*>(ui->gbPreview->layout());
  gbPreviewLayout->setSpacing( KDialog::spacingHint() );

  // The image widgets have to be created here, because the required parameters can only be set on creation.
  // The generated code won't do that.
  imOrig = new ImlibWidget(0L, ui->gbPreview);
  gbPreviewLayout->addWidget(imOrig, 1, 0, Qt::AlignCenter | Qt::AlignTop);

  imFiltered = new ImlibWidget(0L, imOrig->getImlibData(), ui->gbPreview);
  gbPreviewLayout->addWidget(imFiltered, 1, 1, Qt::AlignCenter | Qt::AlignTop);


  // actions
  connect( ui->cbEnableMods, SIGNAL( toggled(bool) ), SLOT( enableWidgets(bool) ));

  connect(ui->cbUpScale, SIGNAL( toggled(bool)), ui->sbMaxUpScaleFactor,
            SLOT( setEnabled(bool) ));

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
  QString filename = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kuickshow/pics/calibrate.png"));
  if ( !imOrig->loadImage( QUrl::fromLocalFile(filename) ) )
    imOrig = 0L; // FIXME - display some errormessage!
  if ( !imFiltered->loadImage( QUrl::fromLocalFile(filename) ) )
    imFiltered = 0L; // FIXME - display some errormessage!

  loadSettings( *kdata );

  if ( imOrig )
    imOrig->setFixedSize( imOrig->size() );
  if ( imFiltered )
    imFiltered->setFixedSize( imFiltered->size() );

  //layout()->activate();
}


DefaultsWidget::~DefaultsWidget()
{
    // those need to be deleted in the right order, as imFiltered
    // references ImlibData from imOrig
    delete imFiltered;
    delete imOrig;

    delete ui;
}

void DefaultsWidget::loadSettings( const KuickData& data )
{
    ui->cbDownScale->setChecked( data.downScale );
    ui->cbUpScale->setChecked( data.upScale );
    ui->sbMaxUpScaleFactor->setValue( data.maxUpScale );

    ui->cbFlipVertically->setChecked( data.flipVertically );
    ui->cbFlipHorizontally->setChecked( data.flipHorizontally );

    ui->comboRotate->setCurrentIndex( ( int )data.rotation );

    ImData *id = data.idata;

    ui->sbBrightness->setValue( id->brightness );
    ui->sbContrast->setValue( id->contrast );
    ui->sbGamma->setValue( id->gamma );

    ui->cbEnableMods->setChecked( data.isModsEnabled );
    enableWidgets( data.isModsEnabled );

    updatePreview();
}

void DefaultsWidget::applySettings( KuickData& data )
{
    data.isModsEnabled = ui->cbEnableMods->isChecked();

    data.downScale  = ui->cbDownScale->isChecked();
    data.upScale    = ui->cbUpScale->isChecked();
    data.maxUpScale = ui->sbMaxUpScaleFactor->value();

    data.flipVertically   = ui->cbFlipVertically->isChecked();
    data.flipHorizontally = ui->cbFlipHorizontally->isChecked();

    data.rotation = currentRotation();

    ImData *id = data.idata;

    id->brightness = ui->sbBrightness->value();
    id->contrast   = ui->sbContrast->value();
    id->gamma      = ui->sbGamma->value();
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

    imFiltered->setBrightness( ui->sbBrightness->value() );
    imFiltered->setContrast( ui->sbContrast->value() );
    imFiltered->setGamma( ui->sbGamma->value() );

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
    return (Rotation) ui->comboRotate->currentIndex();
}
