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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qstring.h>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kdebug.h>

#include "kuickshow.h"
#include "version.h"

static KCmdLineOptions options[] =
{
    { "lastdir", I18N_NOOP("Start in the last visited directory, not the "
			   "current working directory"), 0 },
    { "d", 0, 0 }, // short option for --lastdir
    { "+[files]", I18N_NOOP("Optional image filenames/urls to show"), 0 },
    KCmdLineLastOption
};

extern "C" int kdemain(int argc, char **argv)
{
    KAboutData about(
	  "kuickshow", I18N_NOOP( "KuickShow" ), KUICKSHOWVERSION,
	  I18N_NOOP("A fast and versatile image viewer" ),
	  KAboutData::License_GPL, "(c) 1998-2002, Carsten Pfeiffer",
	  0 /*text*/, "http://devel-home.kde.org/~pfeiffer/" );

    about.addAuthor( "Carsten Pfeiffer", 0, "pfeiffer@kde.org",
		     "http://devel-home.kde.org/~pfeiffer/" );
    about.addCredit( "Rober Hamberger", 0, "rh474@bingo-ev.de" );
    about.addCredit( "Thorsten Scheuermann", 0, "uddn@rz.uni-karlsruhe.de" );

    KCmdLineArgs::init( argc, argv, &about );
    KCmdLineArgs::addCmdLineOptions( options );

    KApplication app;

    if ( app.isRestored() )
	RESTORE( KuickShow )
    else {
	KuickShow *k = new KuickShow( "kuickshow" );
	app.setMainWidget( k );
    }

    return app.exec();
}
