/**
 * $Id$
 *
 * Copyright 2002 by Carsten Pfeiffer
 */

#ifndef SLIDESHOWWIDGET_H
#define SLIDESHOWWIDGET_H

#include "basewidget.h"

class QCheckBox;
class KIntNumInput;

class SlideShowWidget : public BaseWidget
{
    Q_OBJECT
public:
    SlideShowWidget( QWidget *parent, const char *name );
    ~SlideShowWidget();

    virtual void loadSettings( const KuickData& data );
    virtual void applySettings( KuickData& data );

private:
    KIntNumInput *m_delayTime;
    KIntNumInput *m_cycles;
    QCheckBox    *m_fullScreen;
};

#endif // SLIDESHOWWIDGET_H
