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

#include <kurl.h>

// #include those AFTER Qt-includes!
#include <Imlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
// #include <X11/extensions/shape.h>

#include "imdata.h"
#include "kuickdata.h"


// hmm, global declaration for now
enum FlipMode { FlipNone = 0, FlipHorizontal = 1, FlipVertical = 2 };

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
  bool          rotateAbs( Rotation rot );
  void 		flip( FlipMode flipMode );
  bool 		flipAbs( int mode );
  ImlibImage *	imlibImage()	const { return myIm;      }
  Pixmap& 	pixmap();
  void 		renderPixmap();
  const QString& filename() 	const { return myFilename;}

  void 		setDirty( bool d )    { myIsDirty = d;    }
  bool 		isDirty() 	const { return myIsDirty; }
  Rotation      absRotation()   const { return myRotation; }
  FlipMode      flipMode()      const { return myFlipMode; }

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
  void 		setFlipMode( int mode );

  int 		brightness() 		 const;
  int 		contrast()		 const;
  int 		gamma() 		 const;
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

  /**
   * @return true if auto-rotation is not possible, e.g. because no metadata
   * about orientation is available
   */
  virtual bool  autoRotate( KuickImage *kuim );

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
  QCursor m_oldCursor;

  static const int ImlibOffset;


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

};


#endif
