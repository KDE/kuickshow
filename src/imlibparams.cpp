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

#include "imlibparams.h"

#include <QX11Info>
#include <qdebug.h>

#include "kuickdata.h"
#include "imdata.h"

#include <stdlib.h>


/* private */ ImlibParams::ImlibParams()
{
    qDebug();

    mKuickConfig = new KuickData;			// default global application configuration
    mImlibConfig = new ImData;				// default global Imlib configuration

    init();						// initialise the Imlib library
}


bool ImlibParams::init()
{
    mKuickConfig->load();				// read settings from config
    mImlibConfig->load();
    const uint maxcache = mImlibConfig->maxCache*1024;	// cache size from settings

    Display *disp = QX11Info::display();
    Visual *vis = DefaultVisual(disp, DefaultScreen(disp));
    Colormap cm = DefaultColormap(disp, DefaultScreen(disp));

#ifdef HAVE_IMLIB1
    par->paletteoverride = mImlibConfig->ownPalette  ? 1 : 0;
    par->remap           = mImlibConfig->fastRemap   ? 1 : 0;
    par->fastrender      = mImlibConfig->fastRender  ? 1 : 0;
    par->hiquality       = mImlibConfig->dither16bit ? 1 : 0;
    par->dither          = mImlibConfig->dither8bit  ? 1 : 0;
    par->sharedmem       = 1;
    par->sharedpixmaps   = 1;
    par->visualid	 = vis->visualid;

    // PARAMS_PALETTEOVERRIDE taken out because of segfault in imlib :o(
    par->flags = ( PARAMS_REMAP | PARAMS_VISUALID | PARAMS_SHAREDMEM | PARAMS_SHAREDPIXMAPS |
                   PARAMS_FASTRENDER | PARAMS_HIQUALITY | PARAMS_DITHER |
                   PARAMS_IMAGECACHESIZE | PARAMS_PIXMAPCACHESIZE );

    par->imagecachesize  = maxcache;
    par->pixmapcachesize = maxcache;

    mId = Imlib_init_with_params(disp, &par);		// initialise Imlib
    if (mId==nullptr)					// failed to initialise
    {
        qWarning() << "Imlib initialisation failed, trying with own palette file");
        QString paletteFile = QStandardPaths::locate(QStandardPaths::AppDataLocation, "im_palette.pal");
        // FIXME - does the qstrdup() cure the segfault in imlib eventually?
        char *file = qstrdup(paletteFile.toLocal8Bit());
        par.palettefile = file;
        par.flags |= PARAMS_PALETTEFILE;

        qWarning() << "Using palette file" << paletteFile;
        mId = Imlib_init_with_params(disp, &par);		// retry initialise Imlib
        if (mId==nullptr) return (false);
    }
#endif // HAVE_IMLIB1
#ifdef HAVE_IMLIB2
    imlib_context_set_display(disp);
    imlib_context_set_visual(vis);
    imlib_context_set_colormap(cm);
    imlib_set_cache_size(maxcache);
    // Set the maximum number of colours to allocate for 8bpp or less
    imlib_set_color_usage(128);
    // Dither for depths<24bpp
    imlib_context_set_dither((mImlibConfig->dither8bit || mImlibConfig->dither16bit)  ? 1 : 0);

    mId = nullptr;					// nothing to deallocate
#endif // HAVE_IMLIB2
    return (true);
}


/* static */ ImlibParams *ImlibParams::self()
{
    static ImlibParams *p = new ImlibParams;
    return (p);
}


/* private */ ImlibParams::~ImlibParams()
{
#ifdef HAVE_IMLIB1
    if (mId!=nullptr) free(mId);
#endif // HAVE_IMLIB1
}
