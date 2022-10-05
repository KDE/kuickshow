/****************************************************************************
** $Id$
**
** ImageCache: Cache a certain number of KuickImage's in memory
**
** Copyright (C) 1998-2001 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/

#ifndef IMLIBCACHE_H
#define IMLIBCACHE_H

#include <qobject.h>

#include "imlib-wrapper.h"

class QUrl;
class KuickFile;
class KuickImage;

class ImageCache : public QObject
{
  Q_OBJECT

public:
  explicit ImageCache( ImlibData *id, int maxImages=1 );
  virtual ~ImageCache();

  void 			setMaxImages( int maxImages );
  int 			maxImages() 		const { return myMaxImages; }

  KuickImage *		getKuimage( KuickFile * file );
  KuickImage *		loadImage( KuickFile *file, ImlibColorModifier );

private:
  ImlibImage *		loadImageWithQt( const QString& filename ) const;

  int 			myMaxImages;
  QList<KuickFile*>	fileList;
  QList<KuickImage*>	kuickList;
  ImlibData * 		myId;
  int 			idleCount;

private slots:
  void 			slotBusy();
  void 			slotIdle();

signals:
  void			sigBusy();
  void 			sigIdle();
};

#endif
