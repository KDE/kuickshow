/* This file is part of the KDE project
   Copyright (C) 2002 Carsten Pfeiffer <pfeiffer@kde.org>

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
#include <qfont.h>
#include <qfontmetrics.h>
#include <qgrid.h>
#include <qhbox.h>
#include <qlayout.h>
#include <qimage.h>
#include <kimageeffect.h>
#include <qpaintdevicemetrics.h>
#include <qpainter.h>
#include <qradiobutton.h>
#include <qvbuttongroup.h>
#include <qcolor.h>

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

bool Printing::printImage( ImageWindow& imageWin, QWidget *parent )
{
    QString imageURL = imageWin.url().prettyURL();
    KPrinter printer;
    printer.setDocName( imageURL );
    printer.setCreator( "KuickShow-" KUICKSHOWVERSION );

    KPrinter::addDialogPage( new KuickPrintDialogPage( parent, "kuick page"));

    if ( printer.setup( parent, i18n("Print %1").arg(printer.docName().section('/', -1)) ) )
    {
        KTempFile tmpFile( QString::null, ".png" );
        if ( tmpFile.status() == 0 )
        {
            tmpFile.setAutoDelete( true );
            if ( imageWin.saveImage( tmpFile.name(), true ) )
                return printImageWithQt( tmpFile.name(), printer,
                                         imageURL );
        }

        return false;
    }

    return true; // user aborted
}

bool Printing::printImageWithQt( const QString& filename, KPrinter& printer,
                                 const QString& originalFileName )
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

    // Black & white print?
    if ( printer.option( "app-kuickshow-blackwhite" ) != f) {
        image = image.convertDepth( 1, Qt::MonoOnly | Qt::ThresholdDither | Qt::AvoidDither );
    }

    int filenameOffset = 0;
    bool printFilename = printer.option( "app-kuickshow-printFilename" ) != f;
    if ( printFilename ) {
        filenameOffset = fm.lineSpacing() + 14;
        h -= filenameOffset; // filename goes into one line!
    }

    //
    // shrink image to pagesize, if necessary
    //
    bool shrinkToFit = (printer.option( "app-kuickshow-shrinkToFit" ) != f);
    QSize imagesize = image.size();
    if ( shrinkToFit && (image.width() > w || image.height() > h) ) {
        imagesize.scale( w, h, QSize::ScaleMin );
    }


    //
    // align image
    //
    bool ok = false;
    int alignment = printer.option("app-kuickshow-alignment").toInt( &ok );
    if ( !ok )
        alignment = Qt::AlignCenter; // default

    int x = 0;
    int y = 0;

    // ### need a GUI for this in KuickPrintDialogPage!
    // x - alignment
    if ( alignment & Qt::AlignHCenter )
        x = (w - imagesize.width())/2;
    else if ( alignment & Qt::AlignLeft )
        x = 0;
    else if ( alignment & Qt::AlignRight )
        x = w - imagesize.width();

    // y - alignment
    if ( alignment & Qt::AlignVCenter )
        y = (h - imagesize.height())/2;
    else if ( alignment & Qt::AlignTop )
        y = 0;
    else if ( alignment & Qt::AlignBottom )
        y = h - imagesize.height();

    //
    // perform the actual drawing
    //
    p.drawImage( QRect( x, y, imagesize.width(), imagesize.height()), image );

    if ( printFilename )
    {
        QString fname = minimizeString( originalFileName, fm, w );
        if ( !fname.isEmpty() )
        {
            int fw = fm.width( fname );
            int x = (w - fw)/2;
            int y = metrics.height() - filenameOffset/2;
            p.drawText( x, y, fname );
        }
    }

    p.end();

    return true;
}

QString Printing::minimizeString( QString text, const QFontMetrics&
                                  metrics, int maxWidth )
{
    if ( text.length() <= 5 )
        return QString::null; // no sense to cut that tiny little string

    bool changed = false;
    while ( metrics.width( text ) > maxWidth )
    {
        int mid = text.length() / 2;
        text.remove( mid, 2 ); // remove 2 characters in the middle
        changed = true;
    }

    if ( changed ) // add "..." in the middle
    {
        int mid = text.length() / 2;
        if ( mid <= 5 ) // sanity check
            return QString::null;

        text.replace( mid - 1, 3, "..." );
    }

    return text;
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

    m_blackwhite = new QCheckBox ( i18n("Print image in &black and white"), this);
    m_blackwhite->setChecked( false );
    layout->addWidget (m_blackwhite );

    QVButtonGroup *group = new QVButtonGroup( i18n("Scaling"), this );
    group->setRadioButtonExclusive( true );
    layout->addWidget( group );
    // m_shrinkToFit = new QRadioButton( i18n("Shrink image to &fit, if necessary"), group );
    m_shrinkToFit = new QCheckBox( i18n("Shrink image to &fit, if necessary"), group );
    m_shrinkToFit->setChecked( true );

    QWidget *widget = new QWidget( group );
    QGridLayout *grid = new QGridLayout( widget, 3, 3 );
    grid->addColSpacing( 0, 30 );
    grid->setColStretch( 0, 0 );
    grid->setColStretch( 1, 1 );
    grid->setColStretch( 2, 10 );

    m_scale = new QRadioButton( i18n("Print e&xact size: "), widget );
    m_scale->setEnabled( false ); // ###
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
    m_width->setLabel( i18n("&Width:" ) );
    m_width->setMinValue( 1 );

    m_height = new KIntNumInput( widget, "exact height" );
    grid->addWidget( m_height, 2, 1 );
    m_height->setLabel( i18n("&Height:" ) );
    m_height->setMinValue( 1 );
}

KuickPrintDialogPage::~KuickPrintDialogPage()
{
}

void KuickPrintDialogPage::getOptions( QMap<QString,QString>& opts,
                                       bool /*incldef*/ )
{
    QString t = "true";
    QString f = "false";

//    ### opts["app-kuickshow-alignment"] = ;
    opts["app-kuickshow-printFilename"] = m_addFileName->isChecked() ? t : f;
    opts["app-kuickshow-blackwhite"] = m_blackwhite->isChecked() ? t : f;
    opts["app-kuickshow-shrinkToFit"] = m_shrinkToFit->isChecked() ? t : f;
    opts["app-kuickshow-scale"] = m_scale->isChecked() ? t : f;
    opts["app-kuickshow-scale-unit"] = m_units->currentText();
    opts["app-kuickshow-scale-width-pixels"] = QString::number( scaleWidth() );
    opts["app-kuickshow-scale-height-pixels"] = QString::number( scaleHeight() );
}

void KuickPrintDialogPage::setOptions( const QMap<QString,QString>& opts )
{
    QString t = "true";
    QString f = "false";

    m_addFileName->setChecked( opts["app-kuickshow-printFilename"] != f );
    // This sound strange, but if I copy the code on the line above, the checkbox
    // was always checked. And this isn't the wanted behavior. So, with this works.
    // KPrint magic ;-)
    m_blackwhite->setChecked ( false );
    m_shrinkToFit->setChecked( opts["app-kuickshow-shrinkToFit"] != f );
    m_scale->setChecked( opts["app-kuickshow-scale"] == t );

    m_units->setCurrentItem( opts["app-kuickshow-scale-unit"] );

    bool ok;
    int val = opts["app-kuickshow-scale-width-pixels"].toInt( &ok );
    if ( ok )
        setScaleWidth( val );
    val = opts["app-kuickshow-scale-height-pixels"].toInt( &ok );
    if ( ok )
        setScaleHeight( val );

    if ( m_scale->isChecked() == m_shrinkToFit->isChecked() )
        m_shrinkToFit->setChecked( !m_scale->isChecked() );

    // ### re-enable when implementednn
     toggleScaling( false && m_scale->isChecked() );
}

void KuickPrintDialogPage::toggleScaling( bool enable )
{
    m_width->setEnabled( enable );
    m_height->setEnabled( enable );
    m_units->setEnabled( enable );
}

int KuickPrintDialogPage::scaleWidth() const
{
    return fromUnitToPixels( m_width->value() );
}

int KuickPrintDialogPage::scaleHeight() const
{
    return fromUnitToPixels( m_height->value() );
}

void KuickPrintDialogPage::setScaleWidth( int pixels )
{
    m_width->setValue( (int) pixelsToUnit( pixels ) );
}

void KuickPrintDialogPage::setScaleHeight( int pixels )
{
    m_width->setValue( (int) pixelsToUnit( pixels ) );
}

int KuickPrintDialogPage::fromUnitToPixels( float /*value*/ ) const
{
    return 1; // ###
}

float KuickPrintDialogPage::pixelsToUnit( int /*pixels*/ ) const
{
    return 1.0; // ###
}

#include "printing.moc"
