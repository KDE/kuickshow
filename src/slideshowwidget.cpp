/* This file is part of the KDE project
   Copyright (C) 2002 Carsten Pfeiffer <pfeiffer@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

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

    m_fullScreen = new QCheckBox( i18n("Switch to &full-screen"), this );
    m_startWithCurrent = new QCheckBox( i18n("S&tart with current image"), this);

    m_delayTime = new KIntNumInput( this, "delay time" );
    m_delayTime->setLabel( i18n("De&lay between slides:") );
    m_delayTime->setSuffix( i18n(" sec") );
    m_delayTime->setRange( 0, 60 * 60 ); // 1 hour
    m_delayTime->setSpecialValueText( i18n("Wait for key") );

    m_cycles = new KIntNumInput( m_delayTime, 1, this );
    m_cycles->setLabel( i18n("&Iterations (0 = infinite):") );
    m_cycles->setSpecialValueText( i18n("infinite") );
    m_cycles->setRange( 0, 500 );
    
    layout->addWidget( m_fullScreen );
    layout->addWidget( m_startWithCurrent );
    layout->addWidget( m_delayTime );
    layout->addWidget( m_cycles );
    layout->addStretch( 1 );

    loadSettings( *kdata );
}

SlideShowWidget::~SlideShowWidget()
{
}

void SlideShowWidget::loadSettings( const KuickData& data )
{
    m_delayTime->setValue( data.slideDelay / 1000 );
    m_cycles->setValue( data.slideshowCycles );
    m_fullScreen->setChecked( data.slideshowFullscreen );
    m_startWithCurrent->setChecked( !data.slideshowStartAtFirst );
}

void SlideShowWidget::applySettings( KuickData& data )
{
    data.slideDelay = m_delayTime->value() * 1000;
    data.slideshowCycles = m_cycles->value();
    data.slideshowFullscreen = m_fullScreen->isChecked();
    data.slideshowStartAtFirst = !m_startWithCurrent->isChecked();
}

#include "slideshowwidget.moc"
