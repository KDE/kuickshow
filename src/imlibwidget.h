/****************************************************************************
** $Id$
**
** ImlibWidget: maps an Xlib window with Imlib's contents on a QWidget
**
** Created : 98
**
** Copyright (C) 1998-2001 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/

#ifndef IMLIBWIDGET_H
#define IMLIBWIDGET_H

#include <qvariant.h>

#include <qcursor.h>
#include <qevent.h>
#include <qtimer.h>
#include <qwidget.h>
#include <QX11Info>

#include <kurl.h>

// #include those AFTER Qt-includes!
#include "imlib-wrapper.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
// #include <X11/extensions/shape.h>

#include "imdata.h"
#include "kuickdata.h"

class KuickFile;
class KuickImage;

class ImageCache : public QObject
{
  Q_OBJECT

public:
  ImageCache( ImlibData *id, int maxImages=1 );
  ~ImageCache();

  void 			setMaxImages( int maxImages );
  int 			maxImages() 		const { return myMaxImages; }

  KuickImage *		getKuimage( KuickFile * file );
  KuickImage *		loadImage( KuickFile *file, ImlibColorModifier );
  //  KuickImage *		find( const QString& filename );

private:
  ImlibImage *		loadImageWithQt( const QString& filename ) const;

  int 			myMaxImages;
  QList<KuickFile*>	fileList;
  QList<KuickImage*>	kuickList;
  //  QPtrList<ImlibImage>	imList;
  ImlibData * 		myId;
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

public:

  ImlibWidget( ImData *_idata=0, QWidget *parent=0 );
  ImlibWidget( ImData *_idata, ImlibData *id, QWidget *parent=0 );
  virtual ~ImlibWidget();

  KUrl          url()                   const;
  KuickFile *   currentFile()           const;
  bool		loadImage( KuickFile * file);
  bool		loadImage( const KUrl& url );
  bool 		cacheImage( const KUrl& url );
  void 		zoomImage( float );
  void 		setBrightness( int );
  void 		setContrast( int );
  void 		setGamma( int );
  void 		setRotation( Rotation );
  void 		setFlipMode( int mode );

  int 		brightness()     const;
  int 		contrast()		 const;
  int 		gamma() 		 const;
  Rotation 	rotation() 		 const;
  FlipMode	flipMode() 		 const;

  int 		imageWidth() 		 const;
  int 		imageHeight() 		 const;

  void 		setAutoRender( bool enable )   { isAutoRendering = enable;}
  bool 		isAutoRenderEnabled() 	const  { return isAutoRendering;  }
  void 		setMaxImageCache( int );
  int 		maxImageCache() 	const  { return myMaxImageCache;  }
  const QColor& backgroundColor() 	const;
  void 		setBackgroundColor( const QColor& );

  /**
   * @return true if auto-rotation is not possible, e.g. because no metadata
   * about orientation is available
   */
  virtual bool  autoRotate( KuickImage *kuim );

  ImlibData*	getImlibData() const 	       { return id; 		  }

//  virtual void  reparent( QWidget* parent, Qt::WFlags f, const QPoint& p, bool showIt = false );

public slots:
  void 		rotate90();
  void 		rotate270();
  void 		rotate180();
  void 		flipHoriz();
  void 		flipVert();
  void 		showImageOriginalSize();
  inline void 	updateImage() 		{ updateWidget( true ); }


protected:
  Display *	    getX11Display() const { return x11Info().display(); }
  KuickImage *	loadImageInternal( KuickFile * file );
  void 			showImage();
  void          setImageModifier();
  void 		    rotate( int );
  void 		    updateWidget( bool geometryUpdate=true );
  virtual void 	updateGeometry( int width, int height );
  virtual void  loaded( KuickImage *, bool wasCached );
  virtual bool  canZoomTo( int newWidth, int newHeight );
  virtual void  rotated( KuickImage *kuim, int rotation );

  void 		closeEvent( QCloseEvent * );

  inline void	autoUpdate( bool geometryUpdate=false ) {
    if ( isAutoRendering )
      updateWidget( geometryUpdate );
  }

  bool		stillResizing, deleteImData, deleteImlibData;
  bool          imlibModifierChanged;

  KuickImage 	*m_kuim;
  ImageCache 	*imageCache;
  ImlibData     *id;
  ImData    	*idata;
  Window        win;
  ImlibColorModifier mod;

  KuickFile *m_kuickFile;
  QCursor m_oldCursor;

  static const int ImlibOffset;


private:
  void 		init();
  bool 		isAutoRendering;
  int 		myMaxImageCache;
  QColor 	myBackgroundColor;


protected slots:
  bool 		cacheImage( KuickFile *file );
  void 		setBusyCursor();
  void 		restoreCursor();


signals:
  void 		sigImageError( const KuickFile * file, const QString& );

};


#endif
