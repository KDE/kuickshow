/**
 * $Id$
 *
 * Copyright 1998, 1999 by Carsten Pfeiffer
 */


#ifndef DEFAULTSWIDGET_H
#define DEFAULTSWIDGET_H

#include "basewidget.h"

class ImlibWidget;
class ImData;

class QCheckBox;
class QLabel;

class KComboBox;
class KIntNumInput;

class DefaultsWidget : public BaseWidget
{
  Q_OBJECT

public:
  DefaultsWidget( QWidget *parent, const char *name );
  ~DefaultsWidget();

  void 		loadSettings( const KuickData& data );
  void 		applySettings( KuickData& data );

private:
  const int 	currentRotation();

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
  void 		slotEnableMaxScale( bool );
  void 		slotNoImage()		{ imFiltered = 0L; }
  void 		enableWidgets( bool );

};

#endif
