/****************************************************************************
** $Id$
**
** AboutWidget, shows some logos and copyright notice
**
** Created : 98
**
** Copyright (C) 1998, 1999 by Carsten Pfeiffer
**
** All rights reserved.
**
****************************************************************************/

#ifndef ABOUTWIDGET_H
#define ABOUTWIDGET_H

#include <qevent.h>
#include <qvbox.h>

class KURLWidget;

class AboutWidget : public QVBox
{
    Q_OBJECT

public:
    AboutWidget(QWidget *parent = 0, const char *name = 0);

protected:
    ~AboutWidget();
    bool eventFilter( QObject*, QEvent * );
signals:
    void deleteAboutWidget();
private:
    KURLWidget *m_homepage;

};

#endif
