#include "imlib.h"

#include <QTransform>


// the image handle data
struct ImageHandleDataQt : public ImageLibrary::ImageHandleData
{
	ImageHandleDataQt(const QImage& image, const ImageLibrary::ImageModifiers& modifiers) : image(image), modifiers(modifiers) {}
	~ImageHandleDataQt();

	QImage image;
	ImageLibrary::ImageModifiers modifiers;
};


// the subclass of ImageLibrary
class ImageLibraryQt : public ImageLibrary
{
	public:
		ImageLibraryQt();
		~ImageLibraryQt();

		bool isInitialized() const noexcept override { return true; }
		QString getLibraryName() const override { return "Qt"; }
		QString getLibraryVersion() const override { return QT_VERSION_STR; }

		bool isImageBoundToLibraryInstance() const noexcept override { return false; }
		bool supportsColorPalette() const noexcept override { return false; }
		bool supportsDithering() const noexcept override { return false; }
		bool supportsFastRendering() const noexcept override { return false; }
		bool supportsImageColorModifications() const noexcept override { return false; }


	protected:
		// the library-specific image maniplation functions
		ImageHandleData* loadImageWithLibrary(const QString& filename) override;
		ImageHandleData* createImageFromQImage(const QImage& image) override;
		ImageHandleData* copyImage(const ImageHandleData* data) override;
		bool saveImage(const ImageHandleData* data, const QString& filename) override;

		QSize getImageSize(const ImageHandleData* data) const override;

		void flipImage(ImageHandleData* data, FlipMode flipMode) override;
		void rotateImage(ImageHandleData* data, Rotation rotation) override;
		void resizeImageFast(ImageHandleData* data, int newWidth, int newHeight) override;
		bool resizeImageSmooth(ImageHandleData* data, int newWidth, int newHeight) override;

		ImageModifiers getImageModifiers(const ImageHandleData* data) const override;
		void setImageModifiers(ImageHandleData* data, const ImageModifiers& modifiers) override;

		QImage toQImage(const ImageHandleData* data) override;

		bool freeImage(ImageHandleData* data) override;


	private:
		void resizeImage(ImageHandleData* data, int newWidth, int newHeight, Qt::TransformationMode mode);

		// utility functions
		ImageHandleData* createImageHandleData(const QImage& image, const ImageModifiers& modifiers = ImageModifiers()) const;
		inline const ImageHandleDataQt* castData(const ImageHandleData* data) const {
			return dynamic_cast<const ImageHandleDataQt*>(data);
		}
		inline ImageHandleDataQt* castData(ImageHandleData* data) const {
			return dynamic_cast<ImageHandleDataQt*>(data);
		}
};




/*
	implementation of ImageLibraryQt
*/

ImageLibraryQt::ImageLibraryQt()
{
}

ImageLibraryQt::~ImageLibraryQt()
{
}


// implementation of the base class' factory function
ImageLibrary* ImageLibrary::create()
{
	return new ImageLibraryQt();
}


/*
	the library-specific API implementation
*/

ImageLibrary::ImageHandleData* ImageLibraryQt::loadImageWithLibrary(const QString& filename)
{
	// the Qt implementation can just use the generic load function directly
	const QImage image = loadImageWithQt(filename);
	return createImageHandleData(image);
}

ImageLibrary::ImageHandleData* ImageLibraryQt::createImageFromQImage(const QImage& image)
{
	return createImageHandleData(image);
}

ImageLibrary::ImageHandleData* ImageLibraryQt::copyImage(const ImageHandleData* data)
{
	const ImageHandleDataQt* d = castData(data);
	return createImageHandleData(d->image, d->modifiers);
}

bool ImageLibraryQt::saveImage(const ImageHandleData* data, const QString& filename)
{
	const ImageHandleDataQt* d = castData(data);
	return d->image.save(filename);
}


QSize ImageLibraryQt::getImageSize(const ImageHandleData* data) const
{
	return castData(data)->image.size();
}


void ImageLibraryQt::flipImage(ImageHandleData* data, FlipMode flipMode)
{
	ImageHandleDataQt* d = castData(data);
	if (flipMode != FlipNone) {
		d->image = d->image.mirrored(flipMode & FlipHorizontal ? true : false, flipMode & FlipVertical ? true : false);
	}
}

void ImageLibraryQt::rotateImage(ImageHandleData* data, Rotation rotation)
{
	ImageHandleDataQt* d = castData(data);
	QImage& image = d->image;
	QTransform tf;

	switch (rotation) {
		case ROT_0:
			// no-op
			break;

		case ROT_90:
			tf.rotate(90);
			break;

		case ROT_180:
			// use mirrored() for faster transformation
			image = image.mirrored(true, true);
			break;

		case ROT_270:
			tf.rotate(270);
	}

	if (!tf.isIdentity()) image = image.transformed(tf);
}

void ImageLibraryQt::resizeImageFast(ImageHandleData* data, int newWidth, int newHeight)
{
	resizeImage(data, newWidth, newHeight, Qt::FastTransformation);
}

bool ImageLibraryQt::resizeImageSmooth(ImageHandleData* data, int newWidth, int newHeight)
{
	resizeImage(data, newWidth, newHeight, Qt::SmoothTransformation);
	return true;
}

void ImageLibraryQt::resizeImage(ImageHandleData* data, int newWidth, int newHeight, Qt::TransformationMode mode)
{
	ImageHandleDataQt* d = castData(data);
	d->image = d->image.scaled(newWidth, newHeight, Qt::IgnoreAspectRatio, mode);
}


ImageModifiers ImageLibraryQt::getImageModifiers(const ImageHandleData* data) const
{
	return castData(data)->modifiers;
}

void ImageLibraryQt::setImageModifiers(ImageHandleData* data, const ImageModifiers& modifiers)
{
	castData(data)->modifiers = modifiers;
}


QImage ImageLibraryQt::toQImage(const ImageHandleData* data)
{
	const ImageHandleDataQt* d = castData(data);
	QImage image = d->image;

	if (d->modifiers.isSet()) {
		// TODO: apply image modifiers!
	}
	return image;
}


bool ImageLibraryQt::freeImage(ImageHandleData* data)
{
	// no-op, since ImageHandleQt.image doesn't require any manual clean up
	Q_UNUSED(data)
	return true;
}


// even though the destructor is empty, it is defined here to avoid inlining
ImageHandleDataQt::~ImageHandleDataQt()
{
}




/*
	utility functions
*/

ImageLibrary::ImageHandleData* ImageLibraryQt::createImageHandleData(const QImage& image, const ImageModifiers& modifiers) const
{
	return image.isNull() ? nullptr : new ImageHandleDataQt(image, modifiers);
}
