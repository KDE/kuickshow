/* This file is part of the KDE project
   Copyright (C) 1998-2002 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include "aboutwidget.h"
#include <ui_aboutwidget.h>

#include <QDateTime>
#include <QMouseEvent>
#include <QPixmap>
#include <QStandardPaths>

#include "version.h"


AboutWidget::AboutWidget( QWidget *parent )
    : QFrame( parent )
{
    // setup the widget based on its .ui file
    ui = new Ui::AboutWidget;
    ui->setupUi(this);


    // now the properties that couldn't be set in the .ui file

    // KDE specific settings for "window" display (it's just a frame, not a real window)
    setWindowFlag(Qt::FramelessWindowHint, true);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);

    // these settings are difficult to set in designer
    QPalette whitePalette((QColor(Qt::white)));
    setPalette(whitePalette);
    ui->groupBox->setPalette(whitePalette);
    ui->groupBox->setBackgroundRole(QPalette::Window);

    // fill the labels
    ui->lblAuthors->setText("Kuickshow " KUICKSHOWVERSION " was brought to you by");
    ui->urlHomepage->setText("Carsten Pfeiffer");
    ui->urlHomepage->setUrl(HOMEPAGE_URL);
    ui->lblCopyright->setText("(C) 1998-2009");

    // load & show the logo
    int hour = QTime::currentTime().hour();
    QString file;

    if ( hour >= 10 && hour < 16 )
        file = QStandardPaths::locate(QStandardPaths::AppDataLocation, "pics/kuickshow-day.jpg");
    else
        file = QStandardPaths::locate(QStandardPaths::AppDataLocation, "pics/kuickshow-night.jpg");

    QPixmap image;
    if (image.load(file)) {
        ui->picLogo->setPixmap(image);
    } else {
        qWarning("KuickShow: about-image not found/unreadable.");
    }
}

AboutWidget::~AboutWidget()
{
    delete ui;
}


void AboutWidget::mouseReleaseEvent(QMouseEvent* event)
{
    // Clicking anywhere on the frame except for the URL widget removes it.
    // Note: This only works as intended if the frame is displayed as a window. If it is used in another window's
    //       layout, it'll just remove itself from that window (and probably mess up the layout in the process).
    if (!ui->urlHomepage->geometry().contains(event->pos()))
        deleteLater();
}
