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
    cbFullscreen->setChecked( data.fullScreen );
    cbPreload->setChecked( data.preloadImage );
    cbLastdir->setChecked( data.startInLastDir );
     
    quality->setValue(idata->renderQuality);    
    colorButton->setColor( idata->backgroundColor );
}

void GeneralWidget::applySettings( KuickData& data)
{
    ImData *idata = data.idata;

    data.fileFilter      = editFilter->text();
    data.fullScreen  	  = cbFullscreen->isChecked();
    data.preloadImage	  = cbPreload->isChecked();
    data.startInLastDir   = cbLastdir->isChecked();
    
    idata->renderQuality  = quality->value();    
    idata->backgroundColor = colorButton->color();
}

#include "generalwidget.moc"
