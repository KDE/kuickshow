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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qvgroupbox.h>
#include <qslider.h>

#include <kapplication.h>
#include <kcolorbutton.h>
#include <kdialog.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <knuminput.h>
#include <kurllabel.h>
#include <krun.h>

#include "generalwidget.h"

GeneralWidget::GeneralWidget( QWidget *parent, const char *name )
  : QWidget( parent, name )
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setSpacing( KDialog::spacingHint() );

  //removed the label because imo it looks a bit naff
  /* 
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
*/
  cbFullscreen = new QCheckBox( i18n("Display images fullscreen"), this, "boscreen" );

  cbPreload = new QCheckBox( i18n("Preload next image"), this, "preload");

  QGridLayout *gridLayout = new QGridLayout( 3, 2 );
  gridLayout->setSpacing( KDialog::spacingHint() );
  QLabel *l0 = new QLabel( i18n("Background color:"), this );
  colorButton = new KColorButton( this );

  QLabel *l1 = new QLabel( i18n("Show only files with extension: "), this, "label" );
  editFilter = new KLineEdit( this, "filteredit" );

  QLabel *l2 = new QLabel( i18n("Slideshow delay (1/10 s): "), this );
  delaySpinBox = new KIntNumInput( this, "delay spinbox" );
  delaySpinBox->setRange( 1, 600 * 10, 5 ); // max 10 min

  gridLayout->addWidget( l0, 0, 0 );
  gridLayout->addWidget( colorButton, 0, 1 );
  gridLayout->addWidget( l1, 1, 0 );
  gridLayout->addWidget( editFilter, 1, 1 );
  gridLayout->addWidget( l2, 2, 0 );
  gridLayout->addWidget( delaySpinBox, 2, 1 );

  layout->addWidget( cbFullscreen );
  layout->addWidget( cbPreload );
  layout->addLayout( gridLayout );

  ////////////////////////////////////////////////////////////////////////

  QGroupBox *gbox2 = new QGroupBox( 1, Qt::Vertical, i18n("Display Quality vs Speed"),
				     this, "qualitybox" );
  layout->addWidget( gbox2 );
  layout->addStretch();

  QWidget *panel=new QWidget(gbox2, "sliderpanel");
  
  QGridLayout *grid2 = new QGridLayout(panel, 2, 3);
  
  QLabel *l3=new QLabel(i18n("Low Quality\nHigh Speed"), panel);
  l3->setAlignment(Qt::AlignCenter | Qt::AlignVCenter | Qt::ExpandTabs);
  QLabel *l4=new QLabel(i18n("High Quality\nMedium Speed"), panel);
  l4->setAlignment(Qt::AlignCenter | Qt::AlignVCenter | Qt::ExpandTabs);
  QLabel *l5=new QLabel(i18n("Very High Quality\nSlow Speed"), panel);
  l5->setAlignment(Qt::AlignCenter | Qt::AlignVCenter | Qt::ExpandTabs);
  
  quality=new QSlider(0, 2, 1, 1, QSlider::Horizontal, panel, "qualityslider");
  
  quality->setTickmarks(QSlider::Above);
  
  grid2->addWidget(l3, 0, 0, Qt::AlignLeft);
  grid2->addWidget(l4, 0, 1, Qt::AlignCenter);
  grid2->addWidget(l5, 0, 2, Qt::AlignRight);
  
  grid2->addMultiCellWidget(quality, 1, 1, 0, 2);
  
  //cbFastRender = new QCheckBox( i18n("Fast rendering"), gbox2, "fastrender" );
/*  cbDither16bit = new QCheckBox( i18n("Dither in HiColor (15/16bit) modes"),
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
*/
  
  loadSettings( *kdata );
  cbFullscreen->setFocus();
}

GeneralWidget::~GeneralWidget()
{
}

void GeneralWidget::slotURLClicked( const QString & url )
{
  /* There is a bug in my version of KDE such that KApplication::invokeBrowser() runs
     Konqueror instead of the default handler for text/html (Firebird on my machine).
     It would be nice if that were fixed. Failing that, this should be changed to call KRun::runURL */
  kapp->invokeBrowser( url );		
}

void GeneralWidget::loadSettings( const KuickData& data )
{
    ImData *idata = data.idata;

    editFilter->setText( data.fileFilter );
    delaySpinBox->setValue( data.slideDelay / 100 );
    cbFullscreen->setChecked( data.fullScreen );
    cbPreload->setChecked( data.preloadImage );
    quality->setValue(idata->renderQuality);    
    colorButton->setColor( idata->backgroundColor );
    
    /*cbFastRender->setChecked( idata->fastRender );
    cbFastRemap->setChecked( idata->fastRemap );
    cbOwnPalette->setChecked( idata->ownPalette );
    cbDither16bit->setChecked( idata->dither16bit );
    cbDither8bit->setChecked( idata->dither8bit );
    maxCacheSpinBox->setValue( idata->maxCache / 1024 );
*/
    useOwnPalette(); // enable/disable remap-checkbox
}

void GeneralWidget::applySettings( KuickData& data)
{
    ImData *idata = data.idata;

    data.fileFilter      = editFilter->text();
    data.slideDelay 	  = (delaySpinBox->value() * 100);
    data.fullScreen  	  = cbFullscreen->isChecked();
    data.preloadImage	  = cbPreload->isChecked();

    idata->renderQuality  = quality->value();    
    idata->backgroundColor = colorButton->color();
    
    /*
    idata->fastRender 	  = cbFastRender->isChecked();
    idata->fastRemap 	  = cbFastRemap->isChecked();
    idata->ownPalette 	  = cbOwnPalette->isChecked();
    idata->dither16bit 	  = cbDither16bit->isChecked();
    idata->dither8bit 	  = cbDither8bit->isChecked();
    idata->maxCache	  = (uint) maxCacheSpinBox->value() * 1024;
    */
}

void GeneralWidget::useOwnPalette()
{
    //cbFastRemap->setEnabled( cbOwnPalette->isChecked() );
}

#include "generalwidget.moc"
