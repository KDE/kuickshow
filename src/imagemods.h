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

#ifndef IMAGE_MODS_H
#define IMAGE_MODS_H

#include "imlibwidget.h"
#include "kuickimage.h"

#include <QCache>

class ImData;

class ImageMods
{
public:
    static void rememberFor(KuickImage *kuim);
    static bool restoreFor(KuickImage *kuim, ImData *idata);

private:
    static QCache<KUrl,ImageMods> *s_modifications;
    static QCache<KUrl,ImageMods> * getInstance();

    int           width;
    int           height;

    Rotation      rotation;
    FlipMode      flipMode;
};



#endif // IMAGE_MODS_H
