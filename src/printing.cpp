#include <qcheckbox.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qgrid.h>
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
#include <kglobalsettings.h>
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

    KPrinter::addDialogPage( new KuickPrintDialogPage( parent, "kuick page"));

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
    p.setFont( KGlobalSettings::generalFont() );
    QFontMetrics fm = p.fontMetrics();

    int w = metrics.width();
    int h = metrics.height();

    QString t = "true";
    QString f = "false";

    bool printFilename = printer.option( "kuickshow-printFilename" ) != f;
    if ( printFilename ) {
        h -= fm.lineSpacing(); // ### assuming the filename fits into one line
    }

    // shrink image to pagesize, if necessary
    bool shrinkToFit = (printer.option( "kuickshow-shrinkToFit" ) != f);
    if ( shrinkToFit && image.width() > w || image.height() > h ) {
        image = image.smoothScale( w, h, QImage::ScaleMin );
    }

    // center image
    int x = (w - image.width())/2;
    int y = (h - image.height())/2;
    p.drawImage( x, y, image );
    
    if ( printFilename ) {
        int fw = fm.width( filename );
        int x = (w - fw)/2;
        int y = h - fm.lineSpacing(); // ### assumption as above
        p.drawText( x, y, filename );
    }
    
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
    layout->setMargin( KDialog::marginHint() );
    layout->setSpacing( KDialog::spacingHint() );
    
    m_addFileName = new QCheckBox( i18n("Print fi&lename below image"), this);
    m_addFileName->setChecked( true );
    layout->addWidget( m_addFileName );

    QVButtonGroup *group = new QVButtonGroup( i18n("Scaling"), this );
    group->setRadioButtonExclusive( true );
    layout->addWidget( group );
    m_shrinkToFit = new QRadioButton( i18n("Shrink image to &fit, if necessary"), group );
    m_shrinkToFit->setChecked( true );

    QWidget *widget = new QWidget( group );
    QGridLayout *grid = new QGridLayout( widget, 3, 3 );
    grid->addColSpacing( 0, 30 );
    grid->setColStretch( 0, 0 );
    grid->setColStretch( 1, 1 );
    grid->setColStretch( 2, 10 );

    m_scale = new QRadioButton( i18n("Print e&xact size: "), widget );
    grid->addMultiCellWidget( m_scale, 0, 0, 0, 1 );
    group->insert( m_scale );
    connect( m_scale, SIGNAL( toggled( bool )), SLOT( toggleScaling( bool )));

    m_units = new KComboBox( false, widget, "unit combobox" );
    grid->addWidget( m_units, 0, 2, AlignLeft );
    m_units->insertItem( i18n("Millimeters") );
    m_units->insertItem( i18n("Centimeters") );
    m_units->insertItem( i18n("Inches") );

    m_width = new KIntNumInput( widget, "exact width" );
    grid->addWidget( m_width, 1, 1 );
    m_width->setLabel( i18n("&Width" ) );
    m_width->setMinValue( 1 );

    m_height = new KIntNumInput( widget, "exact height" );
    grid->addWidget( m_height, 2, 1 );
    m_height->setLabel( i18n("&Height" ) );
    m_height->setMinValue( 1 );
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
    opts["kuickshow-printFilename"] = m_addFileName->isChecked() ? t : f;
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

    m_addFileName->setChecked( opts["kuickshow-printFilename"] != f );
    m_shrinkToFit->setChecked( opts["kuickshow-shrinkToFit"] != f );
    m_scale->setChecked( opts["kuickshow-scale"] == t );

    m_units->setCurrentItem( opts["kuickshow-scale-unit"] );

    bool ok;
    int val = opts["kuickshow-scale-width-pixels"].toInt( &ok );
    if ( ok )
        setScaleWidth( val, 1 ); // ###
    val = opts["kuickshow-scale-height-pixels"].toInt( &ok );
    if ( ok )
        setScaleHeight( val, 1 ); // ###

    if ( m_scale->isChecked() == m_shrinkToFit->isChecked() )
        m_shrinkToFit->setChecked( !m_scale->isChecked() );

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
