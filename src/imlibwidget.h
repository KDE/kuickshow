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

#include <QScrollArea>

#include "imlib-wrapper.h"

#include "imdata.h"
#include "kuickdata.h"

class QCloseEvent;
class QColor;
class QUrl;
class KuickFile;
class KuickImage;
class QLabel;
class ImageCache;


class ImlibWidget : public QScrollArea
{
  Q_OBJECT

public:

  ImlibWidget( ImData *_idata=0, QWidget *parent=0 );
  ImlibWidget( ImData *_idata, ImlibData *id, QWidget *parent=0 );
  virtual ~ImlibWidget();

  QUrl          url()                   const;
  KuickFile *   currentFile()           const;
  bool		loadImage( KuickFile * file);
  bool		loadImage( const QUrl& url );
  bool 		cacheImage( const QUrl& url );
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
  KuickImage *	loadImageInternal( KuickFile * file );
  void 			showImage();
  void          setImageModifier();
  void 		    rotate( int );
  void 		    updateWidget( bool geometryUpdate=true );
  virtual void 	updateGeometry( int width, int height );
  virtual void  loaded( KuickImage *, bool wasCached );
  virtual bool  canZoomTo( int newWidth, int newHeight );
  virtual void  rotated( KuickImage *kuim, int rotation );

  void 		closeEvent(QCloseEvent *) override;

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

  // TODO: combine this (in a new custom class) with the rotete/flip/scale
  // modifications in KuickImage.  Be able to detect whether it is null
  // (i.e. whether it actually does anything).  Apply all of the requested
  // modifications in one place in KuickImage::toQImage().
  ImlibColorModifier mod;

  KuickFile *m_kuickFile;
  QCursor m_oldCursor;

  static const int ImlibOffset;


private:
  void 		init();
  bool 		isAutoRendering;
  int 		myMaxImageCache;
  QColor 	myBackgroundColor;
  QLabel *myLabel;

protected slots:
  bool 		cacheImage( KuickFile *file );
  void 		setBusyCursor();
  void 		restoreCursor();


signals:
  void 		sigImageError( const KuickFile * file, const QString& );

};


#endif
