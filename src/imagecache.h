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
#include <qcache.h>

#include "imlib-wrapper.h"

class KuickFile;
class KuickImage;


class ImageCache : public QObject
{
  Q_OBJECT

public:
  explicit ImageCache(ImlibData *id, int maxImages = 1);
  // No need for a destructor, ~QCache() deletes all cached objects
  virtual ~ImageCache() = default;

  void 			setMaxImages(int maxImages);
  int 			maxImages() const;

  KuickImage *		getKuimage(KuickFile *file);
  KuickImage *		loadImage(KuickFile *file, const ImlibColorModifier &mod);

private:
  ImlibImage *		loadImageWithQt(const QString &filename) const;

  QCache<QUrl,KuickImage> myCache;
  ImlibData * 		myId;
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
