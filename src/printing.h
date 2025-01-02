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

#ifndef PRINTING_H
#define PRINTING_H

#include <QWidget>

class QFontMetrics;
class QPrinter;
class QString;
class ImageWindow;
class KuickPrintDialogPage;
namespace Ui { class KuickPrintDialogPage; }


class Printing
{
public:
    static bool printImage( ImageWindow& imageWin, QWidget *parent = nullptr);
    static bool printImageWithQt( const QString& filename, QPrinter& printer, KuickPrintDialogPage& dialogPage,
                                  const QString& originalFileName );

private:
    static QString minimizeString( QString text, const QFontMetrics& metrics,
                                   int maxWidth );

};

class KuickPrintDialogPage : public QWidget
{
    Q_OBJECT

public:
    KuickPrintDialogPage( QWidget *parent = nullptr );
    ~KuickPrintDialogPage();

    bool printFilename();
    void setPrintFilename( bool addFilename );

    bool printBlackWhite();
    void setPrintBlackWhite( bool blackWhite );

    bool printShrinkToFit();
    void setPrintShrinkToFit( bool shrinkToFit );

    bool printScale();
    void setPrintScale( bool scale );

    QString printScaleUnit();
    void setPrintScaleUnit( QString scaleUnit );

    int printScaleWidthPixels();
    void setPrintScaleWidthPixels( int scaleWidth );

    int printScaleHeightPixels();
    void setPrintScaleHeightPixels( int scaleHeight );

private Q_SLOTS:
    void toggleScaling( bool enable );

private:
    Ui::KuickPrintDialogPage* ui;

    // return values in pixels!
    int scaleWidth() const;
    int scaleHeight() const;

    void setScaleWidth( int pixels );
    void setScaleHeight( int pixels );

    int fromUnitToPixels( float val ) const;
    float pixelsToUnit( int pixels ) const;

};

#endif // PRINTING_H
