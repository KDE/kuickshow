/**
 * $Id$
 *
 * Copyright 1998, 2000 by Carsten Pfeiffer
 */

#include <qstring.h>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocale.h>

#include "kuickshow.h"
#include "version.h"

static KCmdLineOptions options[] =
{
    { "lastdir", I18N_NOOP("Start in the last visited directory, not the "
			   "current working directory"), 0 },
    { "d", 0, 0 }, // short option for --lastdir
    { "+[files]", I18N_NOOP("Optional image filenames/urls to show"), 0 },
    { 0, 0, 0 }
};

int main(int argc, char **argv)
{
    KAboutData about(
	  "kuickshow", I18N_NOOP( "KuickShow" ), KUICKSHOWVERSION,
	  I18N_NOOP("A fast and versatile image viewer" ),
	  KAboutData::License_GPL, "(c) 1998-2001, Carsten Pfeiffer",
	  0 /*text*/, "http://devel-home.kde.org/~pfeiffer/",
	  "pfeiffer@kde.org" );

    about.addAuthor( "Carsten Pfeiffer", 0, "pfeiffer@kde.org",
		     "http://devel-home.kde.org/~pfeiffer/" );
    about.addCredit( "Rober Hamberger", 0, "Robert.Hamberger@AUDI.DE" );
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
