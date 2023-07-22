/****************************************************************************
** $Id$
**
** ImlibParams: Access to a global copy of the Imlib parameters.
**
** Copyright (C) 1998-2001 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/

#ifndef IMLIBPARAMS_H
#define IMLIBPARAMS_H

#include "imlib-wrapper.h"


class ImlibParams
{
public:
    static ImlibParams *self();

    // This is defined both for Imlib1 and Imlib2, although it
    // is not meaningful for the latter.  The compatibility macros
    // in 'imlib-wrapper.h' throw away the 'id' parameter.
    static ImlibData *imlibData()		{ return (self()->mId); }

    bool init();

private:
    explicit ImlibParams();
    ~ImlibParams();

    ImlibData *mId;
};

#endif
