/* This file is part of the KDE project
   Copyright (C) 2003,2009 Carsten Pfeiffer <pfeiffer@kde.org>

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

#ifndef KUICKIMAGE_H
#define KUICKIMAGE_H

#include <qimage.h>
#include <qobject.h>

#include <kurl.h>

#include "kuickdata.h"
#include "kuickfile.h"

// #include those AFTER Qt-includes!
#include <Imlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
// #include <X11/extensions/shape.h>


class KuickImage : public QObject
{
  Q_OBJECT

public:
  enum ResizeMode { FAST, SMOOTH };

  KuickImage( const KuickFile * file, ImlibImage *im, ImlibData *id );
  ~KuickImage();

  int 		width() 	const { return myWidth;   }
  int 		height()	const { return myHeight;  }
  int 		originalWidth() const { return myOrigWidth; }
  int 		originalHeight() const { return myOrigHeight; }

  /**
   * Returns true if this image is modified against the original loaded
   * version from disk. I.e. resized, rotated, flipped.
   */
  bool		isModified() const;
  void 		resize( int width, int height, KuickImage::ResizeMode mode );
  void 		restoreOriginalSize();
  void		rotate( Rotation rot );
  bool		rotateAbs( Rotation rot );
  void 		flip( FlipMode flipMode );
  bool 		flipAbs( int mode );
  ImlibImage *	imlibImage()	const { return myIm;      }
  Pixmap& 	pixmap();
  void 		renderPixmap();
//  const QString& filename() 	const { return myFilename;}
  const KuickFile& file()       const { return *myFile; }
  const KUrl& url()             const { return myFile->url(); }

  void 		setDirty( bool d )    { myIsDirty = d;    }
  /**
   * Returns true if this image is "dirty", i.e some operation was done,
   * and it needs re-rendering.
   */
  bool 		isDirty() 	const { return myIsDirty; }
  Rotation      absRotation()   const { return myRotation; }
  FlipMode      flipMode()      const { return myFlipMode; }

  static ImlibImage * toImage( ImlibData *id, QImage& image );

private:
  void      fastResize( int newWidth, int newHeight );
  bool 		smoothResize( int width, int height );
  /**
   * Note: caller must delete it!
   */
  QImage * 	newQImage() const;

  const KuickFile * myFile;

  int 		myWidth;
  int 		myHeight;
  ImlibImage * 	myOrigIm;
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


#endif // KUICKIMAGE_H
