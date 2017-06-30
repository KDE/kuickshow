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
#include <KApplication>
#include <KCmdLineArgs>
#include <KDebug>
#include <KLocale>

#include "kuickshow.h"
#include "version.h"


extern "C" KDE_EXPORT int kdemain(int argc, char **argv)
{
    KAboutData about(
	  "kuickshow", 0, ki18n( "KuickShow" ), KUICKSHOWVERSION,
	  ki18n("A fast and versatile image viewer" ),
	  KAboutData::License_GPL, ki18n("(c) 1998-2009, Carsten Pfeiffer"),
	  ki18n(0 /*text*/), "http://devel-home.kde.org/~pfeiffer/" );

    about.addAuthor( ki18n("Carsten Pfeiffer"), KLocalizedString(), "pfeiffer@kde.org",
		     "http://devel-home.kde.org/~pfeiffer/" );
    about.addCredit( ki18n("Rober Hamberger"), KLocalizedString(), "rh474@bingo-ev.de" );
    about.addCredit( ki18n("Thorsten Scheuermann"), KLocalizedString(), "uddn@rz.uni-karlsruhe.de" );

    KCmdLineArgs::init( argc, argv, &about );

    KCmdLineOptions options;
    options.add("lastfolder", ki18n("Start in the last visited folder, not the "
			      "current working folder."));
    options.add("d");
    // short option for --lastdir
    options.add("+[files]", ki18n("Optional image filenames/urls to show"));
    KCmdLineArgs::addCmdLineOptions( options );

    KApplication app;

    if ( app.isSessionRestored() )
	RESTORE( KuickShow )
    else {
	(void) new KuickShow( "kuickshow" );
    }

    return app.exec();
}
