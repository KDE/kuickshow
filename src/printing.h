/****************************************************************************
** $Id$
**
** Created : 2001
**
** Copyright (C) 2001 Carsten Pfeiffer <pfeiffer@kde.org>
**
****************************************************************************/

#ifndef PRINTING_H
#define PRINTING_H

#include <qfontmetrics.h>
#include <qstring.h>

#include <kdeprint/kprintdialogpage.h>

class QCheckBox;
class QRadioButton;
class KComboBox;
class KPrinter;
class KIntNumInput;

class ImageWindow;


class Printing
{
public:
    static bool printImage( const ImageWindow& imageWin, QWidget *parent = 0L);
    static bool printImageWithQt( const QString& filename, KPrinter& printer,
                                  const QString& originalFileName );

private:
    static void addConfigPages();
    static QString minimizeString( QString text, const QFontMetrics& metrics,
                                   int maxWidth );

};

class KuickPrintDialogPage : public KPrintDialogPage
{
    Q_OBJECT

public:
    KuickPrintDialogPage( QWidget *parent = 0L, const char *name = 0 );
    ~KuickPrintDialogPage();

    virtual void getOptions(QMap<QString,QString>& opts, bool incldef = false);
    virtual void setOptions(const QMap<QString,QString>& opts);

private slots:
    void toggleScaling( bool enable );

private:
    // return values in pixels!
    int scaleWidth() const;
    int scaleHeight() const;

    void setScaleWidth( int pixels );
    void setScaleHeight( int pixels );

    int fromUnitToPixels( float val ) const;
    float pixelsToUnit( int pixels ) const;

    QCheckBox *m_shrinkToFit;
    QRadioButton *m_scale;
    KIntNumInput *m_width;
    KIntNumInput *m_height;
    KComboBox *m_units;
    QCheckBox *m_addFileName;

};

#endif // PRINTING_H
