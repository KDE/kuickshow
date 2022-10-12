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

#include "printing.h"
#include <ui_printing_page.h>

#include <KLocalizedString>

#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QScopedPointer>
#include <QTemporaryFile>
#include <QUrl>

#include "filecache.h"
#include "imagewindow.h"
#include "kuickshow_debug.h"
#include "version.h"


bool Printing::printImage( ImageWindow& imageWin, QWidget *parent )
{
    QString imageURL = imageWin.url().toDisplayString();
    QPrinter printer;
    printer.setDocName( imageURL );
    printer.setCreator( "KuickShow-" KUICKSHOWVERSION );

    KuickPrintDialogPage* dialogPage = new KuickPrintDialogPage( parent );
    dialogPage->setObjectName(QString::fromLatin1("kuick page"));
    QPrintDialog *printDialog = new QPrintDialog(&printer, parent);
    printDialog->setOptionTabs(QList<QWidget*>() << dialogPage);
    printDialog->setWindowTitle(i18n("Print %1", printer.docName().section('/', -1)));

    if (printDialog->exec())
    {
        QScopedPointer<QTemporaryFile> tmpFilePtr(FileCache::self()->createTempFile(QStringLiteral(".png")));
        if ( tmpFilePtr->open() )
        {
            if ( imageWin.saveImage( QUrl::fromLocalFile(tmpFilePtr->fileName()), true ) )
            {

                bool success = printImageWithQt( tmpFilePtr->fileName(), printer, *dialogPage,
                                         imageURL);
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
        qWarning("Can't load image: %s for printing.", qUtf8Printable(filename));
        return false;
    }

    QPainter p;
    p.begin( &printer );

    p.setFont( QFontDatabase::systemFont(QFontDatabase::GeneralFont) );
    QFontMetrics fm = p.fontMetrics();

    int w = printer.width();
    int h = printer.height();

    // Black & white print?
    if ( dialogPage.printBlackWhite() ) {
        image = image.convertToFormat( QImage::Format_Mono, Qt::MonoOnly | Qt::ThresholdDither | Qt::AvoidDither );
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
        imagesize.scale( w, h, Qt::KeepAspectRatio );
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
            int fw = fm.horizontalAdvance(fname);
            int tx = (w - fw)/2;
            int ty = printer.height() - filenameOffset/2;
            p.drawText( tx, ty, fname );
        }
    }

    p.end();

    return true;
}

QString Printing::minimizeString( QString text, const QFontMetrics&
                                  metrics, int maxWidth )
{
    if ( text.length() <= 5 )
        return QString(); // no sense to cut that tiny little string

    bool changed = false;
    while (metrics.horizontalAdvance(text) > maxWidth)
    {
        int mid = text.length() / 2;
        text.remove( mid, 2 ); // remove 2 characters in the middle
        changed = true;
    }

    if ( changed ) // add "..." in the middle
    {
        int mid = text.length() / 2;
        if ( mid <= 5 ) // sanity check
            return QString();

        text.replace( mid - 1, 3, "..." );
    }

    return text;
}


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


KuickPrintDialogPage::KuickPrintDialogPage( QWidget *parent )
    : QWidget( parent )
{
    ui = new Ui::KuickPrintDialogPage;
    ui->setupUi(this);

    connect( ui->scale, SIGNAL( toggled( bool )), SLOT( toggleScaling( bool )));
}

KuickPrintDialogPage::~KuickPrintDialogPage()
{
    delete ui;
}

//    ### opts["app-kuickshow-alignment"] = ;

bool KuickPrintDialogPage::printFilename()
{
    return ui->addFileName->isChecked();
}

void KuickPrintDialogPage::setPrintFilename( bool addFilename )
{
    ui->addFileName->setChecked( addFilename );
}

bool KuickPrintDialogPage::printBlackWhite()
{
    return ui->blackwhite->isChecked();
}

void KuickPrintDialogPage::setPrintBlackWhite( bool blackWhite )
{
    ui->blackwhite->setChecked( blackWhite );
}

bool KuickPrintDialogPage::printShrinkToFit()
{
    return ui->shrinkToFit->isChecked();
}

void KuickPrintDialogPage::setPrintShrinkToFit( bool shrinkToFit )
{
    toggleScaling( !shrinkToFit );
}

bool KuickPrintDialogPage::printScale()
{
    return ui->scale->isChecked();
}

void KuickPrintDialogPage::setPrintScale( bool scale )
{
    toggleScaling( scale );
}

QString KuickPrintDialogPage::printScaleUnit()
{
    return ui->units->currentText();
}

void KuickPrintDialogPage::setPrintScaleUnit( QString scaleUnit )
{
    for(int i = 0, count = ui->units->count(); i < count; i++) {
        if(ui->units->itemText(i) == scaleUnit) {
            ui->units->setCurrentIndex(i);
            return;
        }
    }

    ui->units->setCurrentIndex(-1);
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
    ui->shrinkToFit->setChecked( !enable );
    ui->scale->setChecked( enable );
    ui->width->setEnabled( enable );
    ui->height->setEnabled( enable );
    ui->units->setEnabled( enable );
}

int KuickPrintDialogPage::scaleWidth() const
{
    return fromUnitToPixels( ui->width->value() );
}

int KuickPrintDialogPage::scaleHeight() const
{
    return fromUnitToPixels( ui->height->value() );
}

void KuickPrintDialogPage::setScaleWidth( int pixels )
{
    ui->width->setValue(static_cast<int>(pixelsToUnit(pixels)));
}

void KuickPrintDialogPage::setScaleHeight( int pixels )
{
    ui->width->setValue(static_cast<int>(pixelsToUnit(pixels)));
}

int KuickPrintDialogPage::fromUnitToPixels( float /*value*/ ) const
{
    return 1; // ###
}

float KuickPrintDialogPage::pixelsToUnit( int /*pixels*/ ) const
{
    return 1.0; // ###
}
