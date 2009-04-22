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
#include <q3groupbox.h>
#include <qlabel.h>
#include <qlayout.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

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

  gbScale = new Q3GroupBox( i18n("Scaling"), this );
  gbScale->setColumnLayout( 0, Qt::Horizontal );

  cbDownScale = new QCheckBox( i18n("Shrink image to screen size, if larger"), gbScale );
  cbDownScale->setObjectName( QString::fromLatin1( "shrinktoscreen" ) );

  cbUpScale = new QCheckBox( i18n("Scale image to screen size, if smaller, up to factor:"), gbScale );
  cbUpScale->setObjectName( QString::fromLatin1( "upscale checkbox" ) );

  sbMaxUpScaleFactor = new KIntNumInput( gbScale/*, "upscale factor"*/ );
  sbMaxUpScaleFactor->setRange( 1, 100, 1, false );

  connect(cbUpScale, SIGNAL( toggled(bool)), sbMaxUpScaleFactor,
            SLOT( setEnabled(bool) ));

  // --

  gbGeometry = new Q3GroupBox( i18n("Geometry"), this );
  gbGeometry->setColumnLayout( 0, Qt::Horizontal );

  cbFlipVertically = new QCheckBox( i18n("Flip vertically"), gbGeometry );

  cbFlipHorizontally = new QCheckBox( i18n("Flip horizontally"), gbGeometry );

  lbRotate = new QLabel( i18n("Rotate image:"), gbGeometry );

  comboRotate = new KComboBox( gbGeometry );
  comboRotate->setObjectName( "rotate combobox" );
  comboRotate->addItem( i18n("0 Degrees") );
  comboRotate->addItem( i18n("90 Degrees") );
  comboRotate->addItem( i18n("180 Degrees") );
  comboRotate->addItem( i18n("270 Degrees") );

  // --

  gbAdjust = new Q3GroupBox (1, Qt::Horizontal, i18n("Adjustments"), this );

  sbBrightness = new KIntNumInput( gbAdjust/*, "brightness spinbox"*/ );
  sbBrightness->setRange( -256, 256, 1, true );
  sbBrightness->setLabel( i18n("Brightness:"), Qt::AlignVCenter );

  sbContrast = new KIntNumInput( sbBrightness, 0,gbAdjust, 10/*,
				 "contrast spinbox"*/);
  sbContrast->setRange( -256, 256, 1, true );
  sbContrast->setLabel( i18n("Contrast:"), Qt::AlignVCenter );

  sbGamma = new KIntNumInput( sbContrast, 0, gbAdjust, 10/*, "gamma spinbox"*/ );
  sbGamma->setRange( -256, 256, 1, true );
  sbGamma->setLabel( i18n("Gamma:"), Qt::AlignVCenter );

  // --

  gbPreview = new Q3GroupBox( i18n("Preview"), this );
  gbPreview->setAlignment( Qt::AlignCenter );

  lbImOrig = new QLabel( i18n("Original"), gbPreview );
  imOrig = new ImlibWidget( 0L, gbPreview );
  imOrig->setObjectName( QString::fromLatin1("original image") );

  lbImFiltered = new QLabel( i18n("Modified"), gbPreview );
  imFiltered = new ImlibWidget( 0L, imOrig->getImlibData(), gbPreview );
  imFiltered->setObjectName( QString::fromLatin1("modified image") );
  connect( imFiltered, SIGNAL( destroyed() ), SLOT( slotNoImage() ));

  ////
  ////////////////


  // layout management
  QVBoxLayout *mainLayout = new QVBoxLayout( this );
  mainLayout->setMargin( 0 );
  mainLayout->setObjectName( QString::fromLatin1( "main layout" ) );

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

  gbGeometryLayout->addWidget( cbFlipVertically, 0, Qt::AlignLeft );
  gbGeometryLayout->addWidget( cbFlipHorizontally, 0, Qt::AlignLeft );
  gbGeometryLayout->addLayout( rotateLayout, 0 );

  rotateLayout->addWidget( lbRotate, 0, Qt::AlignLeft );
  rotateLayout->addWidget( comboRotate, 0, Qt::AlignLeft );

  // --

  gbPreviewLayout->setMargin( 10 );
  gbPreviewLayout->setSpacing( KDialog::spacingHint() );
  gbPreviewLayout->addWidget( lbImOrig, 0, 0, Qt::AlignCenter );
  gbPreviewLayout->addWidget( imOrig,   1, 0, Qt::AlignCenter | Qt::AlignTop );
  gbPreviewLayout->addWidget( lbImFiltered, 0, 2, Qt::AlignCenter );
  gbPreviewLayout->addWidget( imFiltered,   1, 2, Qt::AlignCenter | Qt::AlignTop );


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


  QString filename = KStandardDirs::locate( "data", "kuickshow/pics/calibrate.png" );
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

    comboRotate->setCurrentIndex( ( int )data.rotation );

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
    return (Rotation) comboRotate->currentIndex();
}

#include "defaultswidget.moc"
