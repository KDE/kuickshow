/****************************************************************************
** $Id$
**
** Definition of something or other
**
** Created : 2000
**
** Copyright (C) 2000 by Carsten Pfeiffer
**
****************************************************************************/

#ifndef KUICKIO_H
#define KUICKIO_H

class QWidget;

#include <qobject.h>

#include <kurl.h>

class KuickIO : public QObject
{
    Q_OBJECT

public:
    static KuickIO * self( QWidget *parent );

private:
    static QWidget * s_parent;
    static KuickIO * s_self;
};

#endif // KUICKIO_H
