#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qvgroupbox.h>

#include <kcolorbutton.h>
#include <kdialog.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <knuminput.h>

#include "generalwidget.h"

GeneralWidget::GeneralWidget( QWidget *parent, const char *name )
  : BaseWidget( "logo", parent, name )
{
  setTitle( i18n("Settings") );

  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setSpacing( KDialog::spacingHint() );
  layout->setMargin( KDialog::marginHint() + 4 );

  cbFullscreen = new QCheckBox( i18n("Fullscreen mode"), this, "boscreen" );

  cbPreload = new QCheckBox( i18n("Preload next image"), this, "preload");

  QGridLayout *gridLayout = new QGridLayout( 3, 3 );
  QLabel *l0 = new QLabel( i18n("Background color"), this );
  colorButton = new KColorButton( this );

  QLabel *l1 = new QLabel( i18n("Show only files with extension: "), this, "label" );
  editFilter = new KLineEdit( this, "filteredit" );

  QLabel *l2 = new QLabel( i18n("Slideshow delay (1/10 s): "), this );
  delaySpinBox = new KIntNumInput( this, "delay spinbox" );
  delaySpinBox->setRange( 1, 600 * 30, 5 ); // 30 min
  QToolTip::add( delaySpinBox, i18n("default: 30 = 3 seconds") );

  gridLayout->addWidget( l0, 0, 0 );
  gridLayout->addWidget( colorButton, 0, 1 );
  gridLayout->addWidget( l1, 1, 0 );
  gridLayout->addWidget( editFilter, 1, 1 );
  gridLayout->addWidget( l2, 2, 0 );
  gridLayout->addWidget( delaySpinBox, 2, 1 );

  layout->addSpacing( 4 * KDialog::marginHint() );
  layout->addWidget( cbFullscreen );
  layout->addWidget( cbPreload );
  layout->addLayout( gridLayout );
  layout->addSpacing( 10 );

  ////////////////////////////////////////////////////////////////////////

  QVGroupBox *gbox2 = new QVGroupBox( i18n("Quality / Speed"),
				     this, "qualitybox" );
  layout->addWidget( gbox2 );
  layout->addStretch();

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
  maxCacheSpinBox->setLabel( i18n("Maximum cache size (MB): "), AlignVCenter );
  maxCacheSpinBox->setRange( 0, 500, 1 );
  QToolTip::add( maxCacheSpinBox, i18n("0 = don't limit") );

  pixLabel->raise();

  loadSettings( *kdata );
  cbFullscreen->setFocus();
}

GeneralWidget::~GeneralWidget()
{
}

void GeneralWidget::loadSettings( const KuickData& data )
{
    ImData *idata = data.idata;

    colorButton->setColor( data.backgroundColor );
    editFilter->setText( data.fileFilter );
    delaySpinBox->setValue( data.slideDelay / 100 );
    cbFullscreen->setChecked( data.fullScreen );
    cbPreload->setChecked( data.preloadImage );
    cbFastRemap->setChecked( idata->fastRemap );
    cbOwnPalette->setChecked( idata->ownPalette );
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
    data.slideDelay 	  = (delaySpinBox->value() * 100);
    data.fullScreen  	  = cbFullscreen->isChecked();
    data.preloadImage	  = cbPreload->isChecked();

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
