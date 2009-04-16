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
#include <q3grid.h>
#include <qlayout.h>
#include <qimage.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QGridLayout>
#include <Q3VButtonGroup>
#include <qpainter.h>
#include <qradiobutton.h>
//
#include <qcolor.h>
#include <QtGui/QPrinter>
#include <QtGui/QPrintDialog>

#include <kcombobox.h>
#include <kdialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <knuminput.h>
#include <ktemporaryfile.h>
#include <kdeprintdialog.h>

#include "imagewindow.h"
#include "printing.h"
#include "version.h"

bool Printing::printImage( const ImageWindow& imageWin, QWidget *parent )
{
    QPrinter printer;
    printer.setDocName( imageWin.filename() );
    printer.setCreator( "KuickShow-" KUICKSHOWVERSION );

    KuickPrintDialogPage* dialogPage = new KuickPrintDialogPage( parent );
    dialogPage->setObjectName(QString::fromLatin1("kuick page"));
    QPrintDialog *printDialog = KdePrint::createPrintDialog(&printer, QList<QWidget*>() << dialogPage, parent);
    printDialog->setWindowTitle(i18n("Print %1", printer.docName().section('/', -1)));

    if (printDialog->exec())
    {
        KTemporaryFile tmpFile;
        tmpFile.setSuffix(".png");
        if ( tmpFile.open() )
        {
            if ( imageWin.saveImage( tmpFile.fileName(), true ) )
            {

                bool success = printImageWithQt( tmpFile.fileName(), printer, *dialogPage,
                                         imageWin.filename() );
                delete printDialog;
                return success;
            }
        }
        delete printDialog;
        return false;
    }
    delete printDialog;
    return true; // user aborted
}

bool Printing::printImageWithQt( const QString& filename, QPrinter& printer, KuickPrintDialogPage& dialogPage,
                                 const QString& originalFileName )
{
    QImage image( filename );
    if ( image.isNull() ) {
        kWarning() << "Can't load image: " << filename << " for printing.\n";
        return false;
    }

    QPainter p;
    p.begin( &printer );

    p.setFont( KGlobalSettings::generalFont() );
    QFontMetrics fm = p.fontMetrics();

    int w = printer.width();
    int h = printer.height();

    // Black & white print?
    if ( dialogPage.printBlackWhite() ) {
        image = image.convertDepth( 1, Qt::MonoOnly | Qt::ThresholdDither | Qt::AvoidDither );
    }

    int filenameOffset = 0;
    bool printFilename = dialogPage.printFilename();
    if ( printFilename ) {
        filenameOffset = fm.lineSpacing() + 14;
        h -= filenameOffset; // filename goes into one line!
    }

    //
    // shrink image to pagesize, if necessary
    //
    bool shrinkToFit = dialogPage.printShrinkToFit();
    QSize imagesize = image.size();
    if ( shrinkToFit && (image.width() > w || image.height() > h) ) {
        imagesize.scale( w, h, Qt::ScaleMin );
    }


    //
    // align image
    //
    // not currently implemented in print options
    //bool ok = false;
    //int alignment = printer.option("app-kuickshow-alignment").toInt( &ok );
    //if ( !ok )
        int alignment = Qt::AlignCenter; // default

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
            int y = printer.height() - filenameOffset/2;
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


KuickPrintDialogPage::KuickPrintDialogPage( QWidget *parent )
    : QWidget( parent )
{
    setWindowTitle( i18n("Image Settings") );

    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->setMargin( KDialog::marginHint() );
    layout->setSpacing( KDialog::spacingHint() );

    m_addFileName = new QCheckBox( i18n("Print fi&lename below image"), this);
    m_addFileName->setChecked( true );
    layout->addWidget( m_addFileName );

    m_blackwhite = new QCheckBox ( i18n("Print image in &black and white"), this);
    m_blackwhite->setChecked( false );
    layout->addWidget (m_blackwhite );

    Q3VButtonGroup *group = new Q3VButtonGroup( i18n("Scaling"), this );
    group->setRadioButtonExclusive( true );
    layout->addWidget( group );
    // m_shrinkToFit = new QRadioButton( i18n("Shrink image to &fit, if necessary"), group );
    m_shrinkToFit = new QCheckBox( i18n("Shrink image to &fit, if necessary"), group );
    m_shrinkToFit->setChecked( true );

    QWidget *widget = new QWidget( group );
    QGridLayout *grid = new QGridLayout( widget );
    grid->addItem( new QSpacerItem( 30, 0 ), 0, 0 );
    grid->setColumnStretch( 0, 0 );
    grid->setColumnStretch( 1, 1 );
    grid->setColumnStretch( 2, 10 );

    m_scale = new QRadioButton( i18n("Print e&xact size: "), widget );
    m_scale->setEnabled( false ); // ###
    grid->addMultiCellWidget( m_scale, 0, 0, 0, 1 );
    group->insert( m_scale );
    connect( m_scale, SIGNAL( toggled( bool )), SLOT( toggleScaling( bool )));

    m_units = new KComboBox( false, widget );
    m_units->setObjectName( "unit combobox" );
    grid->addWidget( m_units, 0, 2, Qt::AlignLeft );
    m_units->addItem( i18n("Millimeters") );
    m_units->addItem( i18n("Centimeters") );
    m_units->addItem( i18n("Inches") );

    m_width = new KIntNumInput( widget/*, "exact width"*/ );
    grid->addWidget( m_width, 1, 1 );
    m_width->setLabel( i18n("&Width:" ) );
    m_width->setMinimum( 1 );

    m_height = new KIntNumInput( widget/*, "exact height"*/ );
    grid->addWidget( m_height, 2, 1 );
    m_height->setLabel( i18n("&Height:" ) );
    m_height->setMinimum( 1 );
}

KuickPrintDialogPage::~KuickPrintDialogPage()
{
}

//    ### opts["app-kuickshow-alignment"] = ;

bool KuickPrintDialogPage::printFilename()
{
    return m_addFileName->isChecked();
}

void KuickPrintDialogPage::setPrintFilename( bool addFilename )
{
    m_addFileName->setChecked( addFilename );
}

bool KuickPrintDialogPage::printBlackWhite()
{
    return m_blackwhite->isChecked();
}

void KuickPrintDialogPage::setPrintBlackWhite( bool blackWhite )
{
    m_blackwhite->setChecked( blackWhite );
}

bool KuickPrintDialogPage::printShrinkToFit()
{
    return m_shrinkToFit->isChecked();
}

void KuickPrintDialogPage::setPrintShrinkToFit( bool shrinkToFit )
{
    toggleScaling( !shrinkToFit );
}

bool KuickPrintDialogPage::printScale()
{
    return m_scale->isChecked();
}

void KuickPrintDialogPage::setPrintScale( bool scale )
{
    toggleScaling( scale );
}

QString KuickPrintDialogPage::printScaleUnit()
{
    return m_units->currentText();
}

void KuickPrintDialogPage::setPrintScaleUnit( QString scaleUnit )
{
    m_units->setCurrentItem( scaleUnit );
}

int KuickPrintDialogPage::printScaleWidthPixels()
{
    return scaleWidth();
}

void KuickPrintDialogPage::setPrintScaleWidthPixels( int scaleWidth )
{
    setScaleWidth(scaleWidth);
}

int KuickPrintDialogPage::printScaleHeightPixels()
{
    return scaleHeight();
}

void KuickPrintDialogPage::setPrintScaleHeightPixels( int scaleHeight )
{
    setScaleHeight(scaleHeight);
}

void KuickPrintDialogPage::toggleScaling( bool enable )
{
    m_shrinkToFit->setChecked( !enable );
    m_scale->setChecked( enable );
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
