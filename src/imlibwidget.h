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

#include <qevent.h>
#include <qptrlist.h>
#include <qtimer.h>
#include <qwidget.h>

#include <kurl.h>

// #include those AFTER Qt-includes!
#include <Imlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
// #include <X11/extensions/shape.h>

#include "imdata.h"


// hmm, global declaration for now
typedef 	int FlipMode;
#define 	FlipHorizontal 	(1 << 1)
#define 	FlipVertical 	(1 << 2)
#define 	FlipNone 	~FlipHorizontal & ~FlipVertical
enum Rotation { ROT_0=0, ROT_90=1, ROT_180=2, ROT_270=3 };



class KuickImage : public QObject
{
  Q_OBJECT

public:
  KuickImage( const QString& filename, ImlibImage *im, ImlibData *id );
  ~KuickImage();

  int 		width() 	const { return myWidth;   }
  int 		height()	const { return myHeight;  }
  int 		originalWidth() const { return myOrigWidth; }
  int 		originalHeight() const { return myOrigHeight; }

  void 		resize( int width, int height );
  void 		restoreOriginalSize();
  void 		rotate( Rotation rot );
  void          rotateAbs( Rotation rot );
  void 		flip( FlipMode flipMode );
  ImlibImage *	imlibImage()	const { return myIm;      }
  Pixmap& 	pixmap();
  void 		renderPixmap();
  const QString& filename() 	const { return myFilename;}

  void 		setDirty( bool d )    { myIsDirty = d;    }
  bool 		isDirty() 	const { return myIsDirty; }
  Rotation absRotation()        const { return myRotation; }

private:
  int 		myWidth;
  int 		myHeight;
  QString 	myFilename;
  ImlibImage * 	myIm;
  ImlibData  * 	myId;
  Pixmap 	myPixmap;
  bool 		myIsDirty;

  int 		myOrigWidth;
  int 		myOrigHeight;
  Rotation 	myRotation;

signals:
  void 		startRendering();
  void 		stoppedRendering();
};


// ------------------------------------------


class ImageCache : public QObject
{
  Q_OBJECT

public:
  ImageCache( ImlibData *id, int maxImages=1 );
  ~ImageCache();

  void 			setMaxImages( int maxImages );
  int 			maxImages() 		const { return myMaxImages; }

  KuickImage *		getKuimage( const QString& file, ImlibColorModifier  );
  //  KuickImage *		find( const QString& filename );

private:
  int 			myMaxImages;
  QStringList		fileList;
  QPtrList<KuickImage>	kuickList;
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

  ImlibWidget( ImData *_idata=0, QWidget *parent=0, const char *name=0 );
  ImlibWidget( ImData *_idata, ImlibData *id, QWidget *parent=0,
	       const char *name=0 );
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
  void 		setFlipMode( FlipMode mode );

  int 		brightness() 		 const;
  int 		contrast()		 const;
  int 		gamma() 		 const;
  Rotation 	rotation() 		 const { return m_kuim ? m_kuim->absRotation() : ROT_0; }
  FlipMode	flipMode() 		 const { return myFlipMode; 	  }

  int 		imageWidth() 		 const;
  int 		imageHeight() 		 const;

  void 		setAutoRender( bool enable )   { isAutoRendering = enable;}
  bool 		isAutoRenderEnabled() 	const  { return isAutoRendering;  }
  void 		setMaxImageCache( int );
  int 		maxImageCache() 	const  { return myMaxImageCache;  }
  const QColor& backgroundColor() 	const;
  void 		setBackgroundColor( const QColor& );

  ImlibData*	getImlibData() const 	       { return id; 		  }

public slots:
  void 		rotate90();
  void 		rotate270();
  void 		rotate180();
  void 		flipHoriz();
  void 		flipVert();
  void 		showImageOriginalSize();
  inline void 	updateImage() 		{ updateWidget( true ); }


protected:
  void 		init();
  KuickImage *	loadImageInternal( const QString&  );
  void 		showImage();
  void          setImageModifier();
  void 		rotate( int );
  void 		updateWidget( bool geometryUpdate=true );
  virtual void 	updateGeometry( int width, int height );
  virtual void  loaded( KuickImage * );

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

  QString m_filename;

  static const int ImlibOffset;


private:
  bool 		isAutoRendering;
  FlipMode 	myFlipMode;
  int 		myMaxImageCache;
  QColor 	myBackgroundColor;


protected slots:
  void 		setBusyCursor();
  void 		restoreCursor();


signals:
  void 		sigBadImage( const QString& );

};


#endif
