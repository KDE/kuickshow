/* This file is part of the KDE project
   Copyright (C) 2000,2001 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include <unistd.h>

#include <qfile.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>

#include "kuickio.h"

KuickIO * KuickIO::s_self = 0L;
QWidget * KuickIO::s_parent = 0L;

KuickIO * KuickIO::self( QWidget *parent )
{
    if ( !s_self )
	s_self = new KuickIO();

    s_self->s_parent = parent;
    return s_self;
}

#include "kuickio.moc"
