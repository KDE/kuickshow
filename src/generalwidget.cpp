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
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qvgroupbox.h>

#include <kapplication.h>
#include <kcolorbutton.h>
#include <kdialog.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <knuminput.h>
#include <kurllabel.h>

#include "generalwidget.h"

GeneralWidget::GeneralWidget( QWidget *parent, const char *name )
  : QWidget( parent, name )
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setSpacing( KDialog::spacingHint() );

  QPixmap pixmap = UserIcon( "logo" );
  KURLLabel *logo = new KURLLabel( this );
  logo->setURL( "http://devel-home.kde.org/~pfeiffer/kuickshow/" );
  logo->setPixmap( pixmap );
  logo->setFixedSize( pixmap.size() );
  logo->setTipText( i18n( "Open KuickShow Website" ) );
  logo->setUseTips( true );

  connect( logo, SIGNAL( leftClickedURL( const QString & ) ),
            SLOT( slotURLClicked( const QString & ) ) );

  layout->addWidget( logo, 0, AlignRight );

  cbFullscreen = new QCheckBox( i18n("Fullscreen mode"), this, "boscreen" );

  cbPreload = new QCheckBox( i18n("Preload next image"), this, "preload");
  cbLastdir = new QCheckBox( i18n("Remember last folder"), this, "restart_lastdir");

  QGridLayout *gridLayout = new QGridLayout( 2, 2 );
  gridLayout->setSpacing( KDialog::spacingHint() );
  QLabel *l0 = new QLabel( i18n("Background color:"), this );
  colorButton = new KColorButton( this );

  QLabel *l1 = new QLabel( i18n("Show only files with extension: "), this, "label" );
  editFilter = new KLineEdit( this, "filteredit" );

  gridLayout->addWidget( l0, 0, 0 );
  gridLayout->addWidget( colorButton, 0, 1 );
  gridLayout->addWidget( l1, 1, 0 );
  gridLayout->addWidget( editFilter, 1, 1 );

  layout->addWidget( cbFullscreen );
  layout->addWidget( cbPreload );
  layout->addWidget( cbLastdir );
  layout->addLayout( gridLayout );

  ////////////////////////////////////////////////////////////////////////

  QVGroupBox *gbox2 = new QVGroupBox( i18n("Quality/Speed"),
				     this, "qualitybox" );
  layout->addWidget( gbox2 );
  layout->addStretch();

  cbSmoothScale = new QCheckBox( i18n("Smooth scaling"), gbox2, "smoothscale" );
  cbFastRender = new QCheckBox( i18n("Fast rendering"), gbox2, "fastrender" );
  cbDither16bit = new QCheckBox( i18n("Dither in HiColor (15/16bit) modes"),
				 gbox2, "dither16bit" );

  cbDither8bit = new QCheckBox( i18n("Dither in LowColor (<=8bit) modes"),
				gbox2, "dither8bit" );

  cbOwnPalette = new QCheckBox( i18n("Use own color palette"),
                                gbox2, "pal");
  connect( cbOwnPalette, SIGNAL( clicked() ), this, SLOT( useOwnPalette() ) );

  cbFastRemap = new QCheckBox( i18n("Fast palette remapping"), gbox2, "remap");

  maxCacheSpinBox = new KIntNumInput( gbox2, "editmaxcache" );
  maxCacheSpinBox->setLabel( i18n("Maximum cache size: "), AlignVCenter );
  maxCacheSpinBox->setSuffix( i18n( " MB" ) );
  maxCacheSpinBox->setSpecialValueText( i18n( "Unlimited" ) );
  maxCacheSpinBox->setRange( 0, 400, 1 );

  loadSettings( *kdata );
  cbFullscreen->setFocus();
}

GeneralWidget::~GeneralWidget()
{
}

void GeneralWidget::slotURLClicked( const QString & url )
{
  kapp->invokeBrowser( url );
}

void GeneralWidget::loadSettings( const KuickData& data )
{
    ImData *idata = data.idata;

    colorButton->setColor( data.backgroundColor );
    editFilter->setText( data.fileFilter );
    cbFullscreen->setChecked( data.fullScreen );
    cbPreload->setChecked( data.preloadImage );
    cbLastdir->setChecked( data.startInLastDir );
    cbFastRemap->setChecked( idata->fastRemap );
    cbOwnPalette->setChecked( idata->ownPalette );
    cbSmoothScale->setChecked( idata->smoothScale );
    cbFastRender->setChecked( idata->fastRender );
    cbDither16bit->setChecked( idata->dither16bit );
    cbDither8bit->setChecked( idata->dither8bit );
    maxCacheSpinBox->setValue( idata->maxCache / 1024 );

    useOwnPalette(); // enable/disable remap-checkbox
}

void GeneralWidget::applySettings( KuickData& data)
{
    ImData *idata = data.idata;

    data.backgroundColor = colorButton->color();
    data.fileFilter      = editFilter->text();
    data.fullScreen  	  = cbFullscreen->isChecked();
    data.preloadImage	  = cbPreload->isChecked();
    data.startInLastDir   = cbLastdir->isChecked();

    idata->smoothScale    = cbSmoothScale->isChecked();
    idata->fastRemap 	  = cbFastRemap->isChecked();
    idata->ownPalette 	  = cbOwnPalette->isChecked();
    idata->fastRender 	  = cbFastRender->isChecked();
    idata->dither16bit 	  = cbDither16bit->isChecked();
    idata->dither8bit 	  = cbDither8bit->isChecked();

    idata->maxCache	  = (uint) maxCacheSpinBox->value() * 1024;
}

void GeneralWidget::useOwnPalette()
{
    cbFastRemap->setEnabled( cbOwnPalette->isChecked() );
}

#include "generalwidget.moc"
