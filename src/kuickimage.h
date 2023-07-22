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

#include <qobject.h>

#include "imlib-wrapper.h"

#include "kuickconfig.h"
#include "kuickfile.h"

class QImage;
class QPixmap;
class QUrl;


class KuickImage : public QObject
{
  Q_OBJECT

public:
  enum ResizeMode { FAST, SMOOTH };

  KuickImage(const KuickFile *file, IMLIBIMAGE &im);
  virtual ~KuickImage();

  int 		width() 	const	{ return myWidth;   }
  int 		height()	const	{ return myHeight;  }
  int 		originalWidth() const	{ return myOrigWidth; }
  int 		originalHeight() const	{ return myOrigHeight; }

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
  IMLIBIMAGE	imlibImage()	const { return myIm;      }
  const KuickFile& file()       const { return *myFile; }
  QUrl url()             const { return myFile->url(); }

  void 		setDirty( bool d )    { myIsDirty = d;    }
  /**
   * Returns true if this image is "dirty", i.e some operation was done,
   * and it needs re-rendering.  TODO: this is not actually used now,
   * but it may be useful for caching the results of toQImage() and
   * toImlibImage().
   */
  bool 		isDirty() 	const { return myIsDirty; }
  Rotation      absRotation()   const { return myRotation; }
  FlipMode      flipMode()      const { return myFlipMode; }

  QImage toQImage() const;

private:
  void      fastResize( int newWidth, int newHeight );
  bool 	    smoothResize( int width, int height );
  /**
   * Note: caller must delete it!
   */
  const KuickFile * myFile;

  int 		myWidth;
  int 		myHeight;
  IMLIBIMAGE 	myOrigIm;
  IMLIBIMAGE 	myIm;

  bool 		myIsDirty;

  int 		myOrigWidth;
  int 		myOrigHeight;
  Rotation 	myRotation;
  FlipMode 	myFlipMode;

signals:
  void 		startRendering() const;
  void 		stoppedRendering() const;
};


#endif // KUICKIMAGE_H
