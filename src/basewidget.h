#ifndef BASEWIDGET_H
#define BASEWIDGET_H

#include <qevent.h>
#include <qgroupbox.h>

#include "kuickdata.h"

class BaseWidget : public QGroupBox
{
    Q_OBJECT

public:
    BaseWidget( const QString& pixfile=QString::null, QWidget *parent=0,
		const char *name=0 );
    virtual ~BaseWidget();

    virtual void 	loadSettings( const KuickData& data )  = 0;
    virtual void	applySettings( KuickData& data ) = 0;

protected:
    QLabel 		*pixLabel;

    virtual void 	resizeEvent( QResizeEvent * );

};

#endif
