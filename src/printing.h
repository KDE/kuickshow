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
    static bool printImageWithQt( const QString& filename, KPrinter& printer );

private:
    static void addConfigPages();

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

    void setScaleWidth( int pixels, int unit );
    void setScaleHeight( int pixels, int unit );

    QRadioButton *m_shrinkToFit;
    QRadioButton *m_scale;
    KIntNumInput *m_width;
    KIntNumInput *m_height;
    KComboBox *m_units;
    QCheckBox *m_addFileName;

};

#endif // PRINTING_H
