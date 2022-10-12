/****************************************************************************
** $Id$
**
** ImageCache: Cache a certain number of KuickImage's in memory
**
** Copyright (C) 1998-2001 by Carsten Pfeiffer.  All rights reserved.
**
****************************************************************************/

#ifndef IMAGECACHE_H
#define IMAGECACHE_H

#include <qobject.h>
#include <qcache.h>

#include "imlib-wrapper.h"

class KuickFile;
class KuickImage;


class ImageCache : public QObject
{
  Q_OBJECT

public:
  explicit ImageCache(int maxImages = 1);
  // No need for a destructor, ~QCache() deletes all cached objects
  virtual ~ImageCache() = default;

  void 			setMaxImages(int maxImages);
  int 			maxImages() const;

  KuickImage *		getKuimage(KuickFile *file);
  KuickImage *		loadImage(KuickFile *file, const ImlibColorModifier &mod);

private:
  QCache<QUrl,KuickImage> myCache;
  int 			idleCount;

#ifdef DEBUG_CACHE
  void dumpCache() const;
#endif

private slots:
  void 			slotBusy();
  void 			slotIdle();

signals:
  void			sigBusy();
  void 			sigIdle();
};

#endif