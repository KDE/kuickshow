#include <qhbox.h>
#include <qlayout.h>
#include <qimage.h>
#include <qpaintdevicemetrics.h>
#include <qpainter.h>
#include <qradiobutton.h>
#include <qvbuttongroup.h>

#include <kcombobox.h>
#include <kdialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <knuminput.h>
#include <kprinter.h>
#include <ktempfile.h>

#include "imagewindow.h"
#include "printing.h"
#include "version.h"

bool Printing::printImage( const ImageWindow& imageWin, QWidget *parent )
{
    KPrinter printer;
    printer.setFullPage( true );
    printer.setDocName( imageWin.filename() );
    printer.setCreator( "KuickShow-" KUICKSHOWVERSION );

    KPrinter::addDialogPage( new KuickPrintDialogPage( parent, "kuick page" ));

    if ( printer.setup( parent ) )
    {
        KTempFile tmpFile( "kuickshow", ".png" );
        if ( tmpFile.status() == 0 )
        {
            tmpFile.setAutoDelete( true );
            if ( imageWin.saveImage( tmpFile.name() ) )
                return printImageWithQt( tmpFile.name(), printer );
        }

        return false;
    }

    return true; // user aborted
}

bool Printing::printImageWithQt( const QString& filename, KPrinter& printer)
{
    QImage image( filename );
    if ( image.isNull() ) {
        kdWarning() << "Can't load image: " << filename << " for printing.\n";
        return false;
    }

    QPainter p;
    p.begin( &printer );

    QPaintDeviceMetrics metrics( &printer );
    int w = metrics.width();
    int h = metrics.height();

    // shrink image to pagesize, if necessary
    if ( image.width() > w || image.height() > h ) {
        image = image.smoothScale( w, h, QImage::ScaleMin );
    }

    // center image
    int x = (w - image.width())/2;
    int y = (h - image.height())/2;
    p.drawImage( x, y, image );
    p.end();

    return true;
}


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


KuickPrintDialogPage::KuickPrintDialogPage( QWidget *parent, const char *name )
    : KPrintDialogPage( parent, name )
{
    setTitle( i18n("Image Settings") );

    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->setAutoAdd( true );
    layout->setMargin( KDialog::marginHint() );
    layout->setSpacing( KDialog::spacingHint() );

    QVButtonGroup *group = new QVButtonGroup( i18n("Scaling"), this );
    group->setRadioButtonExclusive( true );
    m_shrinkToFit = new QRadioButton( i18n("Shrink image to &fit, if necessary"), group );
    m_shrinkToFit->setChecked( true );

    QHBox *box = new QHBox( group );
    m_scale = new QRadioButton( i18n("Print in e&xact size"), box );
    group->insert( m_scale );
    connect( m_scale, SIGNAL( toggled( bool ) ), SLOT( toggleScaling( bool )));

    m_units = new KComboBox( false, box, "unit combobox" );
    m_units->insertItem( i18n("Millimeters") );
    m_units->insertItem( i18n("Centimeters") );
    m_units->insertItem( i18n("Inches") );

    QGridLayout *grid = new QGridLayout( group, 2, 2, KDialog::marginHint() );
//     grid->addItem( new QSpacerItem( 20, 1 ) );

    m_width = new KIntNumInput( group, "exact width" );
    m_width->setLabel( i18n("&Width" ));
    m_width->setMinValue( 1 );

    grid->addWidget( m_width, 0, 1 );
//     grid->addItem( new QSpacerItem( 20, 1) );

    m_height = new KIntNumInput( m_width, 1, group );
    m_height->setLabel( i18n("&Height" ));
    m_height->setMinValue( 1 );
    grid->addWidget( m_height, 1, 1 );


}

KuickPrintDialogPage::~KuickPrintDialogPage()
{
}

void KuickPrintDialogPage::getOptions( QMap<QString,QString>& opts,
                                       bool incldef )
{
    QString t = "true";
    QString f = "false";

//    ### opts["kuickshow-alignment"] = ;
    opts["kuickshow-shrinkToFit"] = m_shrinkToFit->isChecked() ? t : f;
    opts["kuickshow-scale"] = m_scale->isChecked() ? t : f;
    opts["kuickshow-scale-unit"] = m_units->currentText();
    opts["kuickshow-scale-width-pixels"] = QString::number( scaleWidth() );
    opts["kuickshow-scale-height-pixels"] = QString::number( scaleHeight() );
}

void KuickPrintDialogPage::setOptions( const QMap<QString,QString>& opts )
{
    QString t = "true";
    QString f = "false";

    m_shrinkToFit->setChecked( opts["kuickshow-shrinkToFit"] == t );
    m_scale->setChecked( opts["kuickshow-scale"] == t );

    m_units->setCurrentItem( opts["kuickshow-scale-unit"] );

    bool ok;
    int val = opts["kuickshow-scale-width-pixels"].toInt( &ok );
    if ( ok )
        setScaleWidth( val, 1 ); // ###
    val = opts["kuickshow-scale-height-pixels"].toInt( &ok );
    if ( ok )
        setScaleHeight( val, 1 ); // ###
    
    if ( !m_scale->isChecked() && !m_shrinkToFit->isChecked() )
        m_shrinkToFit->setChecked( true );
    
    toggleScaling( m_scale->isChecked() );
}

void KuickPrintDialogPage::toggleScaling( bool enable )
{
    m_width->setEnabled( enable );
    m_height->setEnabled( enable );
    m_units->setEnabled( enable );
}

int KuickPrintDialogPage::scaleWidth() const
{
    return m_width->value(); // ###
}

int KuickPrintDialogPage::scaleHeight() const
{
    return m_height->value(); // ###
}

void KuickPrintDialogPage::setScaleWidth( int pixels, int unit )
{
    m_width->setValue( pixels ); // ###
}

void KuickPrintDialogPage::setScaleHeight( int pixels, int unit )
{
    m_width->setValue( pixels ); // ###
}

#include "printing.moc"
