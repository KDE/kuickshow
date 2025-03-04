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

#include <qurl.h>

#include "imlibwidget.h"

class KActionCollection;
class QContextMenuEvent;
class QCursor;
class QDragEnterEvent;
class QDropEvent;
class QFocusEvent;
class QKeyEvent;
class QMenu;
class QMouseEvent;
class QResizeEvent;
class QSize;
class QWheelEvent;
class QWidget;
class KuickFile;


class ImageWindow : public ImlibWidget
{
  Q_OBJECT

public:
  ImageWindow(QWidget *parent = nullptr);
  virtual ~ImageWindow() = default;

  bool 		showNextImage( KuickFile * file );
  bool 		showNextImage( const QUrl& url );
  void 		scrollImage( int x, int y );
  void		setFullscreen( bool );
  bool 		isFullscreen() 	const { return myIsFullscreen; }

  void 		updateActions();

  KActionCollection * actionCollection() const { return m_actions; }

  /**
   * Resizes image to @p w, @p h, but takes into account the workarea, so
   * it won't ever get a bigger size than the workarea.
   */
  void resizeOptimal( int w, int h );
  void autoScale( KuickImage *kuim );
  virtual bool autoRotate(KuickImage *kuim) override;
  void safeMoveWindow(const QPoint& pos);
  void safeMoveWindow(int x, int y) { safeMoveWindow(QPoint(x, y)); }
  void safeResizeWindow(const QSize& size);
  void safeResizeWindow(int width, int height) { safeResizeWindow(QSize(width, height)); }

  bool          saveImage( const QUrl& dest, bool keepOriginalSize );

public Q_SLOTS:
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

Q_SIGNALS:
  void 		sigFocusWindow( ImageWindow * );
  // go advance images back/forth
  void          requestImage( ImageWindow *, int advance );
  void          deleteImage(ImageWindow *viewer);
  void          trashImage(ImageWindow *viewer);
  void          duplicateWindow(const QUrl &url);
  void		nextSlideRequested();
  void		prevSlideRequested();
  void          showFileBrowser(const QUrl &url);

protected:

  void 		init();
  void 		centerImage();
  virtual void	updateGeometry( int imWidth, int imHeight ) override;
  virtual void  loaded( KuickImage *, bool wasCached ) override;
  virtual bool  canZoomTo( int newWidth, int newHeight ) override;
  virtual void  rotated( KuickImage *kuim, int rotation ) override;

  virtual void  wheelEvent( QWheelEvent * ) override;
  virtual void	keyPressEvent( QKeyEvent * ) override;
  virtual void 	keyReleaseEvent( QKeyEvent * ) override;
  virtual void 	mousePressEvent( QMouseEvent * ) override;
  virtual void 	mouseReleaseEvent( QMouseEvent * ) override;
  virtual void 	mouseMoveEvent( QMouseEvent * ) override;
  virtual void 	focusInEvent( QFocusEvent * ) override;
  virtual void 	resizeEvent( QResizeEvent * ) override;
  virtual void 	dragEnterEvent( QDragEnterEvent * ) override;
  virtual void 	dropEvent( QDropEvent * ) override;
  virtual void  contextMenuEvent( QContextMenuEvent * ) override;

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


  QMenu    *viewerMenu, *gammaMenu, *brightnessMenu, *contrastMenu;
  QWidget       *transWidget;

protected Q_SLOTS:
  void 		slotSaveImage();
  void          reload();
  void          slotProperties();
  void          pauseSlideShow();
  virtual void  setBusyCursor();
  virtual void  restoreCursor();

Q_SIGNALS:
  void          pauseSlideShowSignal();

private:
  int 		desktopWidth( bool totalScreen = false ) const;
  int 		desktopHeight( bool totalScreen = false ) const;
  QSize		maxImageSize() const;
  void          setupActions();
  void 		setPopupMenu();
  bool          isCursorHidden() const;

  bool 		myIsFullscreen;
  QUrl          m_saveDirectory;

  KActionCollection *m_actions;

  static QCursor * s_handCursor;
};


#endif // IMAGEWINDOW_H
