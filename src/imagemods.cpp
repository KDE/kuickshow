/* This file is part of the KDE project
   Copyright (C) 1998-2009 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include <kdebug.h>
#include "kuickdata.h"
#include "imagemods.h"

QCache<QString,ImageMods> * ImageMods::s_modifications = 0L;

QCache<QString,ImageMods> * ImageMods::getInstance() {
	if ( !s_modifications) {
		s_modifications = new QCache<QString,ImageMods>(kdata->modificationCacheSize);
	}
	return s_modifications;
}


void ImageMods::rememberFor(KuickImage *kuim)
{
	QCache<QString,ImageMods> * instance = getInstance();

	ImageMods *mods = instance->object(kuim->filename());
	if ( !mods )
	{
		mods = new ImageMods();
		instance->insert(kuim->filename(), mods);
	}

	mods->width = kuim->width();
	mods->height = kuim->height();
	mods->rotation = kuim->absRotation();
	mods->flipMode = kuim->flipMode();
}

bool ImageMods::restoreFor(KuickImage *kuim)
{
	ImageMods *mods = getInstance()->object(kuim->filename());
	if ( mods )
	{
		kuim->rotateAbs( mods->rotation );
		kuim->flipAbs( mods->flipMode );
		kuim->resize( mods->width, mods->height );
		return true;
	}

	return false;
}
