/***************************************************************************
** $Id$
**
** ImageWindow: extends the simple  ImlibWidget to display image in a Qt window
**		can zoom and scroll images inside the QWidget
**
** Created : 99
**
** Copyright (C) 1999 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/


#ifndef IMAGEWINDOW_H
#define IMAGEWINDOW_H

#include <qevent.h>

#include "imlibwidget.h"

class QCursor;
class QPopupMenu;
class QRect;
class QString;
class QTimer;
class QWidget;

class KAccel;

class ImageWindow : public ImlibWidget
{
  Q_OBJECT

public:
  ImageWindow( ImData *_idata, ImlibData *id, QWidget *parent=0,
	       const char *name=0 );
  ImageWindow( ImData *_idata=0, QWidget *parent=0, const char *name=0 );

  bool 		showNextImage( const QString& filename );
  void 		scrollImage( int, int, bool restrict=true );
  void		setFullscreen( bool );
  bool 		isFullscreen() 	const { return myIsFullscreen; }

  void          addBrightness( int );
  void 		addContrast( int );
  void 		addGamma( int );


  void 		setPopupMenu();
  void 		setPopupAccels();

  KAccel * 	accel() const { return m_accel; }

  /**
   * Resizes image to @p w, @p h, but takes into account the workarea, so
   * it won't ever get a bigger size than the workarea.
   */
  void resizeOptimal( int w, int h );

  void updateAccel();

public slots:
  void 		zoomIn();
  void 		zoomOut();
  void          moreBrightness();
  void          lessBrightness();
  void          moreContrast();
  void          lessContrast();
  void          moreGamma();
  void          lessGamma();
  void 		scrollUp();
  void 		scrollDown();
  void 		scrollLeft();
  void 		scrollRight();
  void 		printImage();
  void 		toggleFullscreen();
  void 		maximize();

signals:
  void 		sigFocusWindow( ImageWindow * );


protected:
  ~ImageWindow(); // deletes itself, just call close( true );

  void 		init();
  void 		centerImage();
  virtual void	updateGeometry( int imWidth, int imHeight );
  virtual void  loaded( KuickImage * );

  virtual void	keyPressEvent( QKeyEvent * );
  virtual void 	keyReleaseEvent( QKeyEvent * );
  virtual void 	mousePressEvent( QMouseEvent * );
  virtual void 	mouseDoubleClickEvent( QMouseEvent * );
  virtual void 	mouseReleaseEvent( QMouseEvent * );
  virtual void 	mouseMoveEvent( QMouseEvent * );
  virtual void 	focusInEvent( QFocusEvent * );
  virtual void 	resizeEvent( QResizeEvent * );
  virtual void 	dragEnterEvent( QDragEnterEvent * );
  virtual void 	dropEvent( QDropEvent * );



  // popupmenu entries
  uint 		itemViewerZoomIn, itemViewerZoomOut, itemViewerFlipH;
  uint 		itemRotate90, itemRotate180, itemRotate270;
  uint 		itemViewerFlipV, itemViewerPrint;
  uint          itemViewerSave, itemViewerClose;
  uint 		itemBrightnessPlus, itemBrightnessMinus;
  uint 		itemContrastPlus, itemContrastMinus;
  uint 		itemGammaPlus, itemGammaMinus;


  uint 		xmove, ymove;	// used for scrolling the image with the mouse
  int		xpos, ypos; 	// top left corner of the image
  int 		xzoom, yzoom;  // used for zooming the image with the mouse
  uint 		xposPress, yposPress;

  QRect 	oldGeometry;


  QPopupMenu    *viewerMenu, *gammaMenu, *brightnessMenu, *contrastMenu;
  QWidget       *transWidget;

  ImageCache * 	imageCache;



protected slots:
  void 		saveImage();


private:
  int 		desktopWidth( bool totalScreen = false ) const;
  int 		desktopHeight( bool totalScreen = false ) const;
  QSize		maxImageSize() const;

  bool 		myIsFullscreen;
  bool 		initialFullscreen;
  int 		m_width;
  int 		m_height;
  int           m_numHeads;

  // Qt resizes us even if we just request a move(). This sucks when
  // switching from fullscreen to window mode, as we will be resized twice.
  bool 		ignore_resize_hack;

  KAccel * 	m_accel;
    
  static QCursor * s_handCursor;

};


#endif // IMAGEWINDOW_H
