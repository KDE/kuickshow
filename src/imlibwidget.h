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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
 */

#ifndef IMLIBWIDGET_H
#define IMLIBWIDGET_H

#include <qvariant.h>

#include <qcursor.h>
#include <qevent.h>
#include <qptrlist.h>
#include <qtimer.h>
#include <qwidget.h>
#include <qwmatrix.h>

#include <kurl.h>

#include <qimage.h>
// #include those AFTER Qt-includes!
#include <X11/Xlib.h>
#include <X11/Xutil.h>
// #include <X11/extensions/shape.h>

#include "imdata.h"
#include "kuickdata.h"

class ImageWindow;

// hmm, global declaration for now
enum FlipMode { FlipNone = 0, FlipHorizontal = 1, FlipVertical = 2 };

class KuickImage : public QObject
{
  Q_OBJECT

public:
  KuickImage(const QString& filename, QImage *im, ImData *idata);
  ~KuickImage();

  int 		width() 	const { return myWidth;   }
  int 		height()	const { return myHeight;  }
  int 		originalWidth() const { return myOrigWidth; }
  int 		originalHeight() const { return myOrigHeight; }

  void 		resize( int width, int height );
  void 		restoreOriginalSize();
  void 		rotate( Rotation rot );
  bool          rotateAbs( Rotation rot );
  void 		flip( FlipMode flipMode );
  bool 		flipAbs( int mode );
  QImage *	getQImage()	const { return myIm;      }
  QPixmap& 	getQPixmap();
  void 		renderPixmap();
  const QString& filename() 	const { return myFilename;}

  void 		setDirty( bool d )    { myIsDirty = d;    }
  bool 		isDirty() 	const { return myIsDirty; }
  Rotation      absRotation()   const { return myRotation; }
  FlipMode      flipMode()      const { return myFlipMode; }

  void		setViewportSize(const QSize &size);
  void		setViewportPosition(const QPoint &point);
  void		setViewport(const QRect &vp);
  const QRect & getViewport() const { return myViewport; }
  
  QImage	smoothTransform();
  QImage	fastTransform();

  void		setColourTransform(int bright, int contrast, int gamma);
  
  void	fastXform(QImage **im, FlipMode flip, Rotation rot, int bright, int contrast, int gamma);
  
private:  
  inline QRgb 	getValidPixel(QImage * im, int x, int y, int dest_x, int dest_y);

  int 		myWidth;
  int 		myHeight;
  QString 	myFilename;
  QImage 	*myIm, *mySourceIm;
  QPixmap	myPixmap;
  QRect		myViewport;
  QWMatrix	myTransform;
  bool 		myIsDirty;

  ImData	*idata;
  
  int 		myOrigWidth;
  int 		myOrigHeight;
  Rotation 	myRotation;
  FlipMode 	myFlipMode;

signals:
  void 		startRendering();
  void 		stoppedRendering();
};


// ------------------------------------------


class ImageCache : public QObject
{
  Q_OBJECT

public:
  ImageCache( int maxImages=1 );
  ~ImageCache();

  void 			setMaxImages( int maxImages );
  int 			maxImages() 		const { return myMaxImages; }

  KuickImage *		getKuimage( const QString& file , ImData *idata );
  //  KuickImage *		find( const QString& filename );

private:
  int 			myMaxImages;
  QStringList		fileList;
  QPtrList<KuickImage>	kuickList;

  int 			idleCount;

private slots:
  void 			slotBusy();
  void 			slotIdle();

signals:
  void			sigBusy();
  void 			sigIdle();

};


// ------------------------------------------


class QColor;

class ImlibWidget : public QWidget
{
  Q_OBJECT

  friend class ImageWindow;  
  
public:

  ImlibWidget( ImData *_idata=0, QWidget *parent=0, const char *name=0 );
  virtual ~ImlibWidget();

  const QString& filename() 		const { return m_filename; }
  KURL          url()                   const;
  bool		loadImage( const QString& filename );
  bool 		cacheImage( const QString& filename );
  void 		zoomImage( float );
  void 		setBrightness( int );
  void 		setContrast( int );
  void 		setGamma( int );
  void 		setRotation( Rotation );
  void 		setFlipMode( int mode );

  int 		brightness() 		 const {return mod_brightness;}
  int 		contrast()		 const {return mod_contrast;}
  int 		gamma() 		 const {return mod_gamma;}
  Rotation 	rotation() 		 const { return m_kuim ? m_kuim->absRotation() : ROT_0; }
  FlipMode	flipMode() 		 const { return m_kuim ? m_kuim->flipMode() : FlipNone; }

  int 		imageWidth() 		 const;
  int 		imageHeight() 		 const;

  void 		setAutoRender( bool enable )   { isAutoRendering = enable;}
  bool 		isAutoRenderEnabled() 	const  { return isAutoRendering;  }
  void 		setMaxImageCache( int );
  int 		maxImageCache() 	const  { return myMaxImageCache;  }
  const QColor& backgroundColor() 	const;
  void 		setBackgroundColor( const QColor& );

  void		setViewportSize(const QSize &size) { m_kuim->setViewportSize(size); }
  void		setViewportPosition(const QPoint &point) {m_kuim->setViewportPosition(point); }
  void		setViewport(const QRect &vp) { m_kuim->setViewport(vp); }
  const QRect & getViewport() { return m_kuim->getViewport(); }
  const QSize	originalImageSize() { if (!imageLoaded()) return QSize(-1,-1); return QSize(m_kuim->originalWidth(), m_kuim->originalHeight()); }
  const QString& imageFilename() { return m_kuim->filename(); }
  
  bool		imageLoaded() { return m_kuim != 0L ; }
  
  void		autoScaleImage(const QSize &maxImageSize);
  
  virtual QSize sizeHint() { if (!imageLoaded()) return QSize(-1, -1); return QSize(imageWidth(), imageHeight()); }
  
  /**
   * @return true if auto-rotation is not possible, e.g. because no metadata
   * about orientation is available
   */
  virtual bool  autoRotate( KuickImage *kuim );

public slots:
  void 		rotate90();
  void 		rotate270();
  void 		rotate180();
  void 		flipHoriz();
  void 		flipVert();
  void 		showImageOriginalSize();
  inline void 	updateImage() 		{ updateWidget( true ); }


protected:
  KuickImage *	loadImageInternal( const QString&  );
  void 		showImage();
  void          setImageModifier();
  void 		rotate( int );
  void 		updateWidget( bool geometryUpdate=true );
  virtual void 	updateGeometry( int width, int height );
  
  virtual void resize(int w, int h);
  
  virtual void	paintEvent( QPaintEvent *e);
  
  void 		closeEvent( QCloseEvent * );

  inline void	autoUpdate( bool geometryUpdate=false ) {
    if ( isAutoRendering )
      updateWidget( geometryUpdate );
  }

  bool		stillResizing, deleteImData;
  bool          imlibModifierChanged;
  bool		needNewCacheImage;
  QString	nextImage;
  
  //int xpos, ypos;	//positioning of the image within the widget
  
  KuickImage 	*m_kuim;
  ImageCache 	*imageCache;
  ImData    	*idata;
  int mod_gamma, mod_brightness, mod_contrast;

  QString m_filename;
  QCursor m_oldCursor;

private:
  void 		init();
  bool 		isAutoRendering;
  int 		myMaxImageCache;
  QColor 	myBackgroundColor;


protected slots:
  void 		setBusyCursor();
  void 		restoreCursor();


signals:
  void 		sigBadImage( const QString& );
  void		loaded( KuickImage * );

};


#endif
