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

#include <KAboutData>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineParser>

#include "kuickshow.h"
#include "version.h"


extern "C" Q_DECL_EXPORT int kdemain(int argc, char **argv)
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("kuickshow");

    KAboutData about(
        QStringLiteral("kuickshow"),
        i18n("KuickShow"),
        KUICKSHOWVERSION,
        i18n("A fast and versatile image viewer"),
        KAboutLicense::GPL,
        i18n("(c) 1998-2009, Carsten Pfeiffer"),
        QString(),
        QStringLiteral("http://devel-home.kde.org/~pfeiffer/")
    );

    about.addAuthor(i18n("Carsten Pfeiffer"), QString(), QStringLiteral("pfeiffer@kde.org"),
        QStringLiteral("http://devel-home.kde.org/~pfeiffer/"));
    about.addCredit(i18n("Rober Hamberger"), QString(), QStringLiteral("rh474@bingo-ev.de"));
    about.addCredit(i18n("Thorsten Scheuermann"), QString(), QStringLiteral("uddn@rz.uni-karlsruhe.de"));

    KAboutData::setApplicationData(about);
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("kuickshow")));


    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    about.setupCommandLine(&parser);

    parser.addOption({ {"d", "lastfolder"}, i18n("Start in the last visited folder, not the current working folder.") });
    parser.addPositionalArgument("files", i18n("Optional image filenames/urls to show"), QStringLiteral("[files...]"));

    parser.process(app);
    about.processCommandLine(&parser);

    // the parser is needed in KuickShow::KuickShow()
    app.setProperty("cmdlineParser", QVariant::fromValue<void *>(&parser));


    if ( app.isSessionRestored() )
	RESTORE( KuickShow )
    else {
	(void) new KuickShow( "kuickshow" );
    }

    return app.exec();
}
