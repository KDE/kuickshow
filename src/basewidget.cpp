#include <qlabel.h>
#include <qpixmap.h>

#include <kdialog.h>
#include <kiconloader.h>

#include "basewidget.h"


BaseWidget::BaseWidget( const QString& pixfile, QWidget *parent,
			const char *name )
    : QGroupBox( parent, name )
{
    pixLabel = new QLabel( this );

    if ( !pixfile.isEmpty() ) {
	QPixmap pix = UserIcon( pixfile );
	pixLabel->setPixmap( pix );
    }
    pixLabel->adjustSize();
}

BaseWidget::~BaseWidget()
{
}

void BaseWidget::resizeEvent( QResizeEvent *e )
{
    QGroupBox::resizeEvent( e );

    int margin = KDialog::marginHint();
    pixLabel->move( width() - pixLabel->width() - margin, margin + 10 );
}

#include "basewidget.moc"
