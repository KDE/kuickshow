/**
 * $Id:
 *
 * Copyright 1998, 1999, 2000 by Carsten Pfeiffer
 */

#ifndef KURLWIDGET_H
#define KURLWIDGET_H

#include <kurllabel.h>

class KURLWidget : public KURLLabel
{
    Q_OBJECT

public:
    KURLWidget( const QString& text, QWidget *, const char *name=0 );

protected slots:
    virtual void run();

};

#endif
