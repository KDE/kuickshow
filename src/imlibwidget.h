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

  explicit ImlibWidget(QWidget *parent = nullptr);
  virtual ~ImlibWidget();

  QSize sizeHint() const override;

  QUrl          url()                   const;
  KuickFile *   currentFile()           const;
  bool		loadImage( KuickFile * file);
  bool		loadImage( const QUrl& url );
  bool 		cacheImage( const QUrl& url );
  void 		zoomImage( float );
  void 		setRotation( Rotation );
  void 		setFlipMode( int mode );

  void 		stepBrightness(int b);
  void 		stepContrast(int c);
  void 		stepGamma(int g);

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

  void setUseModifications(bool enable = true);
  void initModifications();


  /**
   * @return true if auto-rotation is not possible, e.g. because no metadata
   * about orientation is available
   */
  virtual bool  autoRotate( KuickImage *kuim );

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
//  KuickImage *	loadImageInternal( KuickFile * file );
  void 			showImage();
//  void          setImageModifier();
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

//  bool		stillResizing;
//  bool          imlibModifierChanged;

  KuickImage 	*m_kuim;
  ImageCache 	*imageCache;

private:
  // TODO: combine this (in a new custom class) with the rotete/flip/scale
  // modifications in KuickImage.  Be able to detect whether it is null
  // (i.e. whether it actually does anything).  Apply all of the requested
  // modifications in one place in KuickImage::toQImage().
  ImlibColorModifier myModifier;

  KuickFile *m_kuickFile;
  QCursor m_oldCursor;

private:
  void init();
  KuickImage *loadImageInternal(KuickFile *file);
  void stepBrightnessInternal(int b);
  void stepContrastInternal(int c);
  void stepGammaInternal(int g);

  void setImageModifier();

  bool 		isAutoRendering;
  bool 		myUseModifications;
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
