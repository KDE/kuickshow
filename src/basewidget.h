#ifndef BASEWIDGET_H
#define BASEWIDGET_H

#include <qevent.h>
#include <qgroupbox.h>

class BaseWidget : public QGroupBox
{
    Q_OBJECT

public:
    BaseWidget( const QString& pixfile=QString::null, QWidget *parent=0,
		const char *name=0 );
    virtual ~BaseWidget();

    virtual void 	loadSettings()  = 0;
    virtual void	applySettings() = 0;
    virtual void	resetDefaults() = 0;

protected:
    QLabel 		*pixLabel;

    virtual void 	resizeEvent( QResizeEvent * );

};

#endif
