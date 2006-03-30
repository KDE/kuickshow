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

#ifndef IMAGEWINDOW_H
#define IMAGEWINDOW_H

#include <qevent.h>

#include <kaction.h>
#include <kurl.h>

#include "imlibwidget.h"

class QCursor;
class QPopupMenu;
class QRect;
class QString;
class QTimer;
class QWidget;

class KuickFile;

class ImageWindow : public ImlibWidget
{
  Q_OBJECT

public:
  ImageWindow( ImData *_idata, ImlibData *id, QWidget *parent=0,
	       const char *name=0 );
  ImageWindow( ImData *_idata=0, QWidget *parent=0, const char *name=0 );

  bool 		showNextImage( KuickFile * file );
  bool 		showNextImage( const KURL& url );
  void 		scrollImage( int, int, bool restrict=true );
  void		setFullscreen( bool );
  bool 		isFullscreen() 	const { return myIsFullscreen; }

  void          addBrightness( int );
  void 		addContrast( int );
  void 		addGamma( int );

  void 		updateActions();

  KActionCollection * actionCollection() const { return m_actions; }

  /**
   * Resizes image to @p w, @p h, but takes into account the workarea, so
   * it won't ever get a bigger size than the workarea.
   */
  void resizeOptimal( int w, int h );
  void autoScale( KuickImage *kuim );
  virtual bool autoRotate( KuickImage *kuim );

  bool          saveImage( const KURL& dest, bool keepOriginalSize );

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
  void 	        printImage();
  void 		toggleFullscreen();
  void 		maximize();
  void 		imageDelete();
  void 		imageTrash();

signals:
  void 		sigFocusWindow( ImageWindow * );
  // go advance images back/forth
  void          requestImage( ImageWindow *, int advance );
  void          deleteImage(ImageWindow *viewer);
  void          trashImage(ImageWindow *viewer);

protected:
  ~ImageWindow(); // deletes itself, just call close( true );

  void 		init();
  void 		centerImage();
  void          addAlternativeShortcut( KAction *action, int key );
  virtual void	updateGeometry( int imWidth, int imHeight );
  virtual void  loaded( KuickImage * );
  virtual bool  canZoomTo( int newWidth, int newHeight );
  virtual void  rotated( KuickImage *kuim, int rotation );

  virtual void  wheelEvent( QWheelEvent * );
  virtual void	keyPressEvent( QKeyEvent * );
  virtual void 	keyReleaseEvent( QKeyEvent * );
  virtual void 	mousePressEvent( QMouseEvent * );
  virtual void 	mouseReleaseEvent( QMouseEvent * );
  virtual void 	mouseMoveEvent( QMouseEvent * );
  virtual void 	focusInEvent( QFocusEvent * );
  virtual void 	resizeEvent( QResizeEvent * );
  virtual void 	dragEnterEvent( QDragEnterEvent * );
  virtual void 	dropEvent( QDropEvent * );
  virtual void  contextMenuEvent( QContextMenuEvent * );

  void 			showWindow();
  enum KuickCursor { DefaultCursor = 0, ZoomCursor, MoveCursor };
  void 	updateCursor( KuickCursor cursor = DefaultCursor );

  // popupmenu entries
  uint 		itemViewerZoomMax, itemViewerZoomOrig, itemViewerZoomIn;
  uint          itemViewerZoomOut, itemViewerFlipH, itemViewerProps;
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


  QPopupMenu    *viewerMenu, *gammaMenu, *brightnessMenu, *contrastMenu;
  QWidget       *transWidget;


protected slots:
  void 		saveImage();
  void          slotRequestNext()           { emit requestImage( this, +1 ); }
  void          slotRequestPrevious()       { emit requestImage( this, -1 ); }
  void          reload();
  void          slotProperties();
  void          pauseSlideShow();
  virtual void  setBusyCursor();
  virtual void  restoreCursor();

signals:
  void          pauseSlideShowSignal();

private:
  int 		desktopWidth( bool totalScreen = false ) const;
  int 		desktopHeight( bool totalScreen = false ) const;
  QSize		maxImageSize() const;
  void          setupActions();
  void 		setPopupMenu();
  bool          isCursorHidden() const;

  bool 		myIsFullscreen;
  int           m_numHeads;
  QString   m_saveDirectory;

  KActionCollection *m_actions;

  static QCursor * s_handCursor;
};


#endif // IMAGEWINDOW_H
