#include <qcheckbox.h>
#include <qlayout.h>

#include <kdialog.h>
#include <klocale.h>
#include <knuminput.h>

#include "slideshowwidget.h"


SlideShowWidget::SlideShowWidget( QWidget *parent, const char *name )
    : QWidget( parent, name )
{
//     setTitle( i18n("Slideshow") );

    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->setSpacing( KDialog::spacingHint() );

    m_fullScreen = new QCheckBox( i18n("Switch to &fullscreen"), this );
    
    m_delayTime = new KIntNumInput( this, "delay time" );
    m_delayTime->setLabel( i18n("De&lay between slides:") );
    m_delayTime->setSuffix( i18n(" sec") );
    m_delayTime->setRange( 1, 60 * 60 ); // 1 hour

    m_cycles = new KIntNumInput( m_delayTime, 1, this );
    m_cycles->setLabel( i18n("&Iterations (0 = infinite):") );
    m_cycles->setSpecialValueText( i18n("infinite") );
    m_cycles->setRange( 0, 500 );

    layout->addWidget( m_fullScreen );
    layout->addWidget( m_delayTime );
    layout->addWidget( m_cycles );
    layout->addStretch( 1 );

    loadSettings( *kdata );
}

SlideShowWidget::~SlideShowWidget()
{
}

void SlideShowWidget:: loadSettings( const KuickData& data )
{
    m_delayTime->setValue( data.slideDelay / 1000 );
    m_cycles->setValue( data.slideshowCycles );
    m_fullScreen->setChecked( data.slideshowFullscreen );
}

void SlideShowWidget::applySettings( KuickData& data )
{
    data.slideDelay = m_delayTime->value() * 1000;
    data.slideshowCycles = m_cycles->value();
    data.slideshowFullscreen = m_fullScreen->isChecked();
}

#include "slideshowwidget.moc"
