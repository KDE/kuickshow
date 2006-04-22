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

#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qvgroupbox.h>

#include <kcombobox.h>
#include <kdialog.h>
#include <klocale.h>
#include <knuminput.h>
#include <kstandarddirs.h>

#include "imlibwidget.h"
#include "defaultswidget.h"

DefaultsWidget::DefaultsWidget( QWidget *parent, const char *name)
  : QWidget( parent, name )
{
  imFiltered = 0L;

  cbEnableMods = new QCheckBox( i18n("Apply default image modifications"), this );
  connect( cbEnableMods, SIGNAL( toggled(bool) ), SLOT( enableWidgets(bool) ));

  // create all the widgets

  gbScale = new QGroupBox( i18n("Scaling"), this );
  gbScale->setColumnLayout( 0, Qt::Horizontal );

  cbDownScale = new QCheckBox( i18n("Shrink image to screen size, if larger"),
			       gbScale, "shrinktoscreen" );

  cbUpScale = new QCheckBox( i18n("Scale image to screen size, if smaller, up to factor:"), gbScale, "upscale checkbox" );

  sbMaxUpScaleFactor = new KIntNumInput( gbScale, "upscale factor" );
  sbMaxUpScaleFactor->setRange( 1, 100, 1, false );

  connect(cbUpScale, SIGNAL( toggled(bool)), sbMaxUpScaleFactor,
            SLOT( setEnabled(bool) ));

  // --

  gbGeometry = new QGroupBox( i18n("Geometry"), this );
  gbGeometry->setColumnLayout( 0, Qt::Horizontal );

  cbFlipVertically = new QCheckBox( i18n("Flip vertically"), gbGeometry );

  cbFlipHorizontally = new QCheckBox( i18n("Flip horizontally"), gbGeometry );

  lbRotate = new QLabel( i18n("Rotate image:"), gbGeometry );

  comboRotate = new KComboBox( gbGeometry, "rotate combobox" );
  comboRotate->insertItem( i18n("0 Degrees") );
  comboRotate->insertItem( i18n("90 Degrees") );
  comboRotate->insertItem( i18n("180 Degrees") );
  comboRotate->insertItem( i18n("270 Degrees") );

  // --

  gbAdjust = new QVGroupBox( i18n("Adjustments"), this );

  sbBrightness = new KIntNumInput( gbAdjust, "brightness spinbox" );
  sbBrightness->setRange( -256, 256, 1, true );
  sbBrightness->setLabel( i18n("Brightness:"), AlignVCenter );

  sbContrast = new KIntNumInput( sbBrightness, 0,gbAdjust, 10,
				 "contrast spinbox");
  sbContrast->setRange( -256, 256, 1, true );
  sbContrast->setLabel( i18n("Contrast:"), AlignVCenter );

  sbGamma = new KIntNumInput( sbContrast, 0, gbAdjust, 10, "gamma spinbox" );
  sbGamma->setRange( -256, 256, 1, true );
  sbGamma->setLabel( i18n("Gamma:"), AlignVCenter );

  // --

  gbPreview = new QGroupBox( i18n("Preview"), this );
  gbPreview->setAlignment( AlignCenter );

  lbImOrig = new QLabel( i18n("Original"), gbPreview );
  imOrig = new ImlibWidget( 0L, gbPreview, "original image" );

  lbImFiltered = new QLabel( i18n("Modified"), gbPreview );
  imFiltered = new ImlibWidget( 0L, imOrig->getImlibData(), gbPreview, "" );
  connect( imFiltered, SIGNAL( destroyed() ), SLOT( slotNoImage() ));

  ////
  ////////////////


  // layout management
  QVBoxLayout *mainLayout = new QVBoxLayout( this, 0,
            KDialog::spacingHint(), "main layout" );

  QVBoxLayout *gbScaleLayout = new QVBoxLayout( gbScale->layout(),
            KDialog::spacingHint());
  QVBoxLayout *gbGeometryLayout = new QVBoxLayout(gbGeometry->layout(),
            KDialog::spacingHint());
  QGridLayout *gbPreviewLayout = new QGridLayout(gbPreview, 2, 3, 0,
            KDialog::spacingHint());

  QHBoxLayout *scaleLayout = new QHBoxLayout();
  QHBoxLayout *rotateLayout = new QHBoxLayout();

  mainLayout->addWidget( cbEnableMods );
  mainLayout->addWidget( gbScale );
  QHBoxLayout *hl = new QHBoxLayout();
  hl->addWidget( gbGeometry );
  hl->addWidget( gbAdjust );
  mainLayout->addLayout( hl );
  mainLayout->addWidget( gbPreview );
  mainLayout->addStretch();

  // --

  gbScaleLayout->addWidget( cbDownScale );
  gbScaleLayout->addLayout( scaleLayout );

  scaleLayout->addWidget( cbUpScale );
  scaleLayout->addWidget( sbMaxUpScaleFactor );

  // --

  gbGeometryLayout->addWidget( cbFlipVertically, 0, AlignLeft );
  gbGeometryLayout->addWidget( cbFlipHorizontally, 0, AlignLeft );
  gbGeometryLayout->addLayout( rotateLayout, 0 );

  rotateLayout->addWidget( lbRotate, 0, AlignLeft );
  rotateLayout->addWidget( comboRotate, 0, AlignLeft );

  // --

  gbPreviewLayout->setMargin( 10 );
  gbPreviewLayout->setSpacing( KDialog::spacingHint() );
  gbPreviewLayout->addWidget( lbImOrig, 0, 0, AlignCenter );
  gbPreviewLayout->addWidget( imOrig,   1, 0, AlignCenter | AlignTop );
  gbPreviewLayout->addWidget( lbImFiltered, 0, 2, AlignCenter );
  gbPreviewLayout->addWidget( imFiltered,   1, 2, AlignCenter | AlignTop );


  ////
  ////////////////

  // connect them all to the update slot
  connect( cbDownScale,        SIGNAL( clicked() ), SLOT( updatePreview() ));
  connect( cbUpScale,          SIGNAL( clicked() ), SLOT( updatePreview() ));
  connect( cbFlipVertically,   SIGNAL( clicked() ), SLOT( updatePreview() ));
  connect( cbFlipHorizontally, SIGNAL( clicked() ), SLOT( updatePreview() ));
  connect( sbMaxUpScaleFactor, SIGNAL( valueChanged(int) ), SLOT( updatePreview() ));
  connect( sbBrightness, SIGNAL( valueChanged(int) ), SLOT( updatePreview() ));
  connect( sbContrast,   SIGNAL( valueChanged(int) ), SLOT( updatePreview() ));
  connect( sbGamma,      SIGNAL( valueChanged(int) ), SLOT( updatePreview() ));

  connect( comboRotate,  SIGNAL( activated(int) ), SLOT( updatePreview() ));


  QString filename = locate( "data", "kuickshow/pics/calibrate.png" );
  if ( !imOrig->loadImage( filename ) )
    imOrig = 0L; // FIXME - display some errormessage!
  if ( !imFiltered->loadImage( filename ) )
    imFiltered = 0L; // FIXME - display some errormessage!

  loadSettings( *kdata );

  if ( imOrig )
    imOrig->setFixedSize( imOrig->size() );
  if ( imFiltered )
    imFiltered->setFixedSize( imFiltered->size() );

  mainLayout->activate();
}


DefaultsWidget::~DefaultsWidget()
{
    // those need to be deleted in the right order, as imFiltered
    // references ImlibData from imOrig
    delete imFiltered;
    delete imOrig;
}

void DefaultsWidget::loadSettings( const KuickData& data )
{
    cbDownScale->setChecked( data.downScale );
    cbUpScale->setChecked( data.upScale );
    sbMaxUpScaleFactor->setValue( data.maxUpScale );

    cbFlipVertically->setChecked( data.flipVertically );
    cbFlipHorizontally->setChecked( data.flipHorizontally );

    comboRotate->setCurrentItem( data.rotation );

    ImData *id = data.idata;

    sbBrightness->setValue( id->brightness );
    sbContrast->setValue( id->contrast );
    sbGamma->setValue( id->gamma );

    cbEnableMods->setChecked( data.isModsEnabled );
    enableWidgets( data.isModsEnabled );

    updatePreview();
}

void DefaultsWidget::applySettings( KuickData& data )
{
    data.isModsEnabled = cbEnableMods->isChecked();

    data.downScale  = cbDownScale->isChecked();
    data.upScale    = cbUpScale->isChecked();
    data.maxUpScale = sbMaxUpScaleFactor->value();

    data.flipVertically   = cbFlipVertically->isChecked();
    data.flipHorizontally = cbFlipHorizontally->isChecked();

    data.rotation = currentRotation();

    ImData *id = data.idata;

    id->brightness = sbBrightness->value();
    id->contrast   = sbContrast->value();
    id->gamma      = sbGamma->value();
}

void DefaultsWidget::updatePreview()
{
    if ( !imFiltered )
	return;

    imFiltered->setAutoRender( false );

    int flipMode = cbFlipHorizontally->isChecked() ? FlipHorizontal : FlipNone;
    flipMode |= cbFlipVertically->isChecked() ? FlipVertical : FlipNone;
    imFiltered->setFlipMode( flipMode );

    Rotation rotation = cbEnableMods->isChecked() ? currentRotation() : ROT_0;
    imFiltered->setRotation( rotation );

    imFiltered->setBrightness( sbBrightness->value() );
    imFiltered->setContrast( sbContrast->value() );
    imFiltered->setGamma( sbGamma->value() );

    imFiltered->updateImage();
    imFiltered->setAutoRender( true );
}


void DefaultsWidget::enableWidgets( bool enable )
{
    gbScale->setEnabled( enable );
    sbMaxUpScaleFactor->setEnabled( enable & cbUpScale->isChecked() );

    gbGeometry->setEnabled( enable );
    gbAdjust->setEnabled( enable );
    gbPreview->setEnabled( enable );
    updatePreview();
}


Rotation DefaultsWidget::currentRotation() const
{
    return (Rotation) comboRotate->currentItem();
}

#include "defaultswidget.moc"
