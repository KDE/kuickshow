#ifndef LOGOTABDIALOG_H
#define LOGOTABDIALOG_H

#ifdef KDE_USE_FINAL
#undef index
#endif
#include <kdialogbase.h>

#include <qevent.h>
#include <qglobal.h>
#include <qpushbutton.h>

#include "logo.h"

class LogoTabDialog : public KDialogBase
{
    Q_OBJECT

public:
    LogoTabDialog( int dialogFace, const QString& caption, int buttons,
		   ButtonCode defaultButton,
		   QWidget *parent=0, const char *name=0, bool modal=true );

  void  addWidget(QWidget *);

protected:
//     virtual void resizeEvent(QResizeEvent *);

private:
  Logo 		*logo;
  QWidget       *w;
  bool          hasWidget;

};


#endif
