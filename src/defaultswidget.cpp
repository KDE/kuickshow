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
#include "kuickdata.h"

DefaultsWidget::DefaultsWidget( QWidget *parent, const char *name)
  : BaseWidget( QString::null, parent, name )
{
  imFiltered = 0L;

  cbEnableMods = new QCheckBox( i18n("Apply default image modifications"), this );
  connect( cbEnableMods, SIGNAL( toggled(bool) ), SLOT( enableWidgets(bool) ));

  // create all the widgets

  gbScale = new QGroupBox( i18n("Scaling"), this );
  cbDownScale = new QCheckBox( i18n("Shrink image to screensize, if larger"),
			       gbScale, "shrinktoscreen" );

  cbUpScale = new QCheckBox( i18n("Scale image to screensize, if smaller, up to factor:"), gbScale, "upscale checkbox" );
  connect(cbUpScale, SIGNAL( toggled(bool)), SLOT( slotEnableMaxScale(bool) ));

  sbMaxUpScaleFactor = new KIntNumInput( gbScale, "upscale factor" );
  sbMaxUpScaleFactor->setRange( 1, 1000, 1, false );

  // --

  gbGeometry = new QGroupBox( i18n("Geometry"), this );
  cbFlipVertically = new QCheckBox( i18n("Flip vertically"), gbGeometry );

  cbFlipHorizontally = new QCheckBox( i18n("Flip horizontally"), gbGeometry );

  lbRotate = new QLabel( i18n("Rotate image about"), gbGeometry );

  comboRotate = new KComboBox( gbGeometry, "rotate combobox" );
  comboRotate->insertItem( i18n("0 degrees") );
  comboRotate->insertItem( i18n("90 degrees") );
  comboRotate->insertItem( i18n("180 degrees") );
  comboRotate->insertItem( i18n("270 degrees") );

  // --

  gbAdjust = new QVGroupBox( i18n("Adjustments"), this );

  sbBrightness = new KIntNumInput( gbAdjust, "brightness spinbox" );
  sbBrightness->setRange( -256, 256, 1, true );
  sbBrightness->setLabel( i18n("Brightness"), AlignVCenter );

  sbContrast = new KIntNumInput( sbBrightness, 0,gbAdjust, 10,
				 "contrast spinbox");
  sbContrast->setRange( -256, 256, 1, true );
  sbContrast->setLabel( i18n("Contrast"), AlignVCenter );

  sbGamma = new KIntNumInput( sbContrast, 0, gbAdjust, 10, "gamma spinbox" );
  sbGamma->setRange( -256, 256, 1, true );
  sbGamma->setLabel( i18n("Gamma"), AlignVCenter );

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
  QVBoxLayout *mainLayout = new QVBoxLayout( this, 10, -1, "main layout" );

  QVBoxLayout *gbScaleLayout = new QVBoxLayout( gbScale, 10, 5, "scale lay");
  QVBoxLayout *gbGeometryLayout = new QVBoxLayout(gbGeometry, 10, 5,"geo lay");
  QGridLayout *gbPreviewLayout = new QGridLayout(gbPreview, 2, 3, 0, -1, "vi");

  QHBoxLayout *scaleLayout = new QHBoxLayout( -1, "scale sublayout" );
  QHBoxLayout *rotateLayout = new QHBoxLayout( -1, "rotate sublayout" );


  mainLayout->addWidget( cbEnableMods );
  mainLayout->addWidget( gbScale );
  QHBoxLayout *hl = new QHBoxLayout();
  hl->addWidget( gbGeometry );
  hl->addWidget( gbAdjust );
  mainLayout->addLayout( hl );
  mainLayout->addWidget( gbPreview );

  // --

  gbScaleLayout->addSpacing( 6 );
  gbScaleLayout->addWidget( cbDownScale, 0, AlignLeft );
  gbScaleLayout->addLayout( scaleLayout, 0 );

  scaleLayout->addWidget( cbUpScale );
  scaleLayout->addWidget( sbMaxUpScaleFactor );

  // --

  gbGeometryLayout->addSpacing( 6 );
  gbGeometryLayout->addWidget( cbFlipVertically, 0, AlignLeft );
  gbGeometryLayout->addWidget( cbFlipHorizontally, 0, AlignLeft );
  gbGeometryLayout->addLayout( rotateLayout, 0 );

  rotateLayout->addWidget( lbRotate, 0, AlignLeft );
  rotateLayout->addWidget( comboRotate, 0, AlignLeft );

  // --

  if ( imFiltered ) { // assume, if one is ok, all are ok
      gbPreviewLayout->setMargin( 10 );
      gbPreviewLayout->setSpacing( KDialog::spacingHint() );
      gbPreviewLayout->addWidget( lbImOrig, 0, 0, AlignCenter );
      gbPreviewLayout->addWidget( imOrig,   1, 0, AlignCenter | AlignTop );
      gbPreviewLayout->addWidget( lbImFiltered, 0, 2, AlignCenter );
      gbPreviewLayout->addWidget( imFiltered,   1, 2, AlignCenter | AlignTop );
  }


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

  loadSettings();

  if ( imOrig )
    imOrig->setFixedSize( imOrig->size() );
  if ( imFiltered )
    imFiltered->setFixedSize( imFiltered->size() );

  mainLayout->activate();
}


DefaultsWidget::~DefaultsWidget()
{
}

void DefaultsWidget::loadSettings()
{
    init( *kdata );
}

void DefaultsWidget::resetDefaults()
{
    KuickData data;
    init( data );
}

void DefaultsWidget::init( const KuickData& data )
{
    cbDownScale->setChecked( data.downScale );
    cbUpScale->setChecked( data.upScale );
    sbMaxUpScaleFactor->setValue( data.maxUpScale );

    cbFlipVertically->setChecked( data.flipVertically );
    cbFlipHorizontally->setChecked( data.flipHorizontally );

    comboRotate->setCurrentItem( data.rotation / 90 );

    ImData *id = data.idata;

    sbBrightness->setValue( id->brightness );
    sbContrast->setValue( id->contrast );
    sbGamma->setValue( id->gamma );

    cbEnableMods->setChecked( data.isModsEnabled );
    enableWidgets( data.isModsEnabled );

    updatePreview();
}

void DefaultsWidget::applySettings()
{
    kdata->isModsEnabled = cbEnableMods->isChecked();

    kdata->downScale  = cbDownScale->isChecked();
    kdata->upScale    = cbUpScale->isChecked();
    kdata->maxUpScale = sbMaxUpScaleFactor->value();

    kdata->flipVertically   = cbFlipVertically->isChecked();
    kdata->flipHorizontally = cbFlipHorizontally->isChecked();

    kdata->rotation = currentRotation();

    ImData *id = kdata->idata;

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

    switch ( currentRotation() ) {
    case 0: {
	imFiltered->setRotation( ROT_0 );
	break;
    }
    case 90: {
	imFiltered->setRotation( ROT_90 );
	break;
    }
    case 180: {
	imFiltered->setRotation( ROT_180 );
	break;
    }
    case 270: {
	imFiltered->setRotation( ROT_270 );
	break;
    }
    default:
	qDebug("kuickshow: oups, what rotation did you select???");
    }

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
}


void DefaultsWidget::slotEnableMaxScale( bool enable )
{
    sbMaxUpScaleFactor->setEnabled( enable );
}


const int DefaultsWidget::currentRotation()
{
    return comboRotate->currentItem() * 90;
}

#include "defaultswidget.moc"
