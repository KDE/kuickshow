/**
 * $Id:
 *
 * Copyright 1998 - 2001 by Carsten Pfeiffer
 */

#ifndef FILEFINDER_H
#define FILEFINDER_H

#include <qevent.h>

#include <klineedit.h>

class FileFinder : public KLineEdit
{
    Q_OBJECT

public:
    FileFinder( QWidget *parent=0, const char *name=0 );
    ~FileFinder();

signals:
    void enterDir( const QString& );

protected:
    virtual void focusOutEvent( QFocusEvent * );
    virtual void keyPressEvent( QKeyEvent * );

private slots:
    void slotAccept( const QString& );
    
};

#endif // FILEFINDER_H
