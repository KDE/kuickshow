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

#include <kiconloader.h>

#include <QDateTime>
#include <QPixmap>
#include <QStandardPaths>
#include <QGridLayout>
#include <QLabel>
#include <QGuiApplication>

#include <klocalizedstring.h>


AboutWidget::AboutWidget( QWidget *parent )
    : QFrame( parent )
{
    QGridLayout *gl = new QGridLayout(this);
    gl->setRowMinimumHeight(0, 30);

    QLabel *websiteLabel = new QLabel(i18n("<A HREF=\"%2\">%1 Website</A>", QGuiApplication::applicationDisplayName(), HOMEPAGE_URL), this);
    websiteLabel->setOpenExternalLinks(true);
    websiteLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    gl->addWidget(websiteLabel, 1, 0, Qt::AlignLeft);

    QLabel *websiteLogo = new QLabel(this);
    websiteLogo->setPixmap(KIconLoader::global()->loadIcon("logo", KIconLoader::User));
    gl->addWidget(websiteLogo, 1, 1, Qt::AlignRight);

    gl->setRowMinimumHeight(2, 30);

    // Load & show the logo
    int hour = QTime::currentTime().hour();
    QString file;

    // TODO: can get active hours from KIdleTime?
    // Going by these hours, either you don't have to work very hard
    // or are at an extreme latitude...
    if ( hour >= 10 && hour < 16 )
        file = QStandardPaths::locate(QStandardPaths::AppDataLocation, "pics/kuickshow-day.jpg");
    else
        file = QStandardPaths::locate(QStandardPaths::AppDataLocation, "pics/kuickshow-night.jpg");

    QPixmap image(file);;
    if (!image.isNull()) {
        QLabel *pixLabel = new QLabel(this);
        pixLabel->setPixmap(image);
        gl->addWidget(pixLabel, 3, 0, 1, -1, Qt::AlignHCenter|Qt::AlignTop);
    } else {
        qWarning() << "Image not found or unreadable";
    }

    gl->setRowStretch(3, 1);
}
