/****************************************************************************
** $Id$
**
** The main widget, that contains the fileview, dirview and preview of images
**
** Created : 98
**
** Copyright (C) 1998, 1999 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <qevent.h>
#include <qstring.h>
#include <qwidget.h>

class FileView;

class MainWidget : public QWidget
{
  Q_OBJECT

public:
  MainWidget( QString, QWidget *parent, const char *name=0L);
  ~MainWidget();

  FileView* 	getFileBox() { return box; }

protected:
  virtual void 	resizeEvent( QResizeEvent * );
  
private:
  FileView 	*box;

};


#endif
