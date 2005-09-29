/* This file is part of the KDE project
   Copyright (C) 1998-2003 Carsten Pfeiffer <pfeiffer@kde.org>

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

#ifndef DEFAULTSWIDGET_H
#define DEFAULTSWIDGET_H

#include "kuickdata.h"

class ImlibWidget;
class ImData;

class QCheckBox;
class QLabel;

class KComboBox;
class KIntNumInput;

class DefaultsWidget : public QWidget
{
  Q_OBJECT

public:
  DefaultsWidget( QWidget *parent, const char *name );
  ~DefaultsWidget();

  void 		loadSettings( const KuickData& data );
  void 		applySettings( KuickData& data );

private:
  Rotation      currentRotation() const;

  QCheckBox 	*cbEnableMods;

  QGroupBox 	*gbScale;
  QCheckBox 	*cbUpScale, *cbDownScale;
  KIntNumInput 	*sbMaxUpScaleFactor;

  QGroupBox 	*gbAdjust;
  KIntNumInput 	*sbBrightness, *sbContrast, *sbGamma;

  QGroupBox 	*gbGeometry;
  QLabel 	*lbRotate;
  KComboBox 	*comboRotate;
  QCheckBox 	*cbFlipVertically, *cbFlipHorizontally;

  QGroupBox 	*gbPreview;
  QLabel 	*lbImOrig, *lbImFiltered;
  ImlibWidget 	*imOrig, *imFiltered;


private slots:
  void 		updatePreview();
  void 		slotNoImage()		{ imFiltered = 0L; }
  void 		enableWidgets( bool );

};

#endif
