/**
 * $Id$
 *
 * Copyright 1998, 1999 by Carsten Pfeiffer
 */

#include "fileview.h"
#include "kuickdata.h"
#include "mainwidget.h"


MainWidget::MainWidget( QString startDir, QWidget *parent,
			const char *name ) : QWidget ( parent, name )
{	
  box = new FileView( startDir, true, (QDir::Dirs | QDir::Files),
		      this, "fileview" );
}


MainWidget::~MainWidget()
{

}


// for now, no layout managers
void MainWidget::resizeEvent( QResizeEvent * )
{
  box->resize( width(), height() );
}
#include "mainwidget.moc"
