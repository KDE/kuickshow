#ifndef LOGO_H
#define LOGO_H

#include <qevent.h>
#include <qlabel.h>

class Logo : public QLabel
{
public:
    Logo(const QString& filename = "logo.png", QWidget *parent = 0,
	 const char *name = 0);
    ~Logo() {}

protected:
    void  mousePressEvent( QMouseEvent * );
    void  openHomepage();

};

#endif
