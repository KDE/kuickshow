#include "logotabdialog.h"


LogoTabDialog::LogoTabDialog( int dialogFace, const QString& caption,
			      int buttons, ButtonCode defaultButton,
			      QWidget *parent, const char *name, bool modal )
    : KDialogBase( dialogFace, caption, buttons, defaultButton, parent, name,
		   modal )
{
    // logo = new Logo("logo.png", this);
}

void  LogoTabDialog::addWidget(QWidget *widget)
{
    w = widget;
    w->setGeometry(0, 0, width(), height()-logo->height()-10);

    hasWidget = true;
}


// void  LogoTabDialog::resizeEvent(QResizeEvent *e)
// {
//     uint  margin    = 5;
//     uint  y         = 0; // The y position of the space at the bottom
//     int   barHeight = 20; //height() - tabBar()->y() - tabBar()->height();
//     int   tmp, leftmost;

//     KDialogBase::resizeEvent(e);

//     y = height()-2*margin-logo->height(); // Wild guess...

//     tmp = (barHeight-logo->height())/2;
//     logo->move( 2*margin, height() - logo->height() - 4 );

//     leftmost = logo->x()+logo->width()+2*margin;
// }

#include "logotabdialog.moc"
