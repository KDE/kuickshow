#include "imlib.h"


// this variable is set by ImageLibrary::get() during its first call and reset by ImageLibrary::reset()
std::shared_ptr<ImageLibrary> ImageLibrary::singleton;


struct ImageHandleDataWrapper
{
	Q_DISABLE_COPY_MOVE(ImageHandleDataWrapper)

	ImageHandleDataWrapper(ImageLibrary::ImageHandleData* data, const std::shared_ptr<ImageLibrary>& libraryInstance) :
		data(data), libraryInstance(libraryInstance)
	{}
	ImageHandleDataWrapper(ImageLibrary::ImageHandleData* data, std::shared_ptr<ImageLibrary>&& libraryInstance) :
		data(data), libraryInstance(std::move(libraryInstance))
	{}
	~ImageHandleDataWrapper();

	// the library-specific data for this handle
	ImageLibrary::ImageHandleData* const data;

	// the reference to the library instance, so that it stays in memory until its last image handle is deleted
	const std::shared_ptr<ImageLibrary> libraryInstance;
};




ImageLibrary::ImageLibrary()
{
}

ImageLibrary::~ImageLibrary()
{
}


std::shared_ptr<ImageLibrary> ImageLibrary::get()
{
	if(!singleton) {
		auto newImLib = std::shared_ptr<ImageLibrary>(create());
		newImLib->_self = newImLib;
		singleton = newImLib;
	}
	return singleton;
}

void ImageLibrary::reset()
{
	singleton.reset();
}


void ImageLibrary::addAboutData(KAboutData& /* aboutData */)
{
	// this is only relevant if sub-classes use an external library for the image manipulations
}


/*
	the public API
*/

ImageHandle ImageLibrary::loadImage(const QString& filename)
{
	auto imlib = get();
	return imlib->createImageHandle(imlib->loadImageWithLibrary(filename));
}

ImageHandle ImageLibrary::fromQImage(const QImage& image)
{
	auto imlib = get();
	return imlib->createImageHandle(imlib->createImageFromQImage(image));
}

ImageHandle ImageLibrary::copyImage(const ImageHandle& imageHandle)
{
	// this will already create a full copy of the image if the library instance was changed
	ImageHandle copy = ensureCurrentLibraryInstance(imageHandle);
	if (!copy || copy != imageHandle) return copy;

	// otherwise, create a copy within the current library instance
	auto imlib = imageHandle->libraryInstance;
	const ImageHandleData* data = getImageHandleDataFromHandle(imageHandle);
	copy = imlib->createImageHandle(imlib->copyImage(data));

	return copy;
}

bool ImageLibrary::saveImage(const ImageHandle& imageHandle, const QString& filename)
{
	const ImageHandleData* data = getImageHandleDataFromHandle(imageHandle);
	return data != nullptr ? imageHandle->libraryInstance->saveImage(data, filename) : false;
}


QSize ImageLibrary::getImageSize(const ImageHandle& imageHandle)
{
	const ImageHandleData* data = getImageHandleDataFromHandle(imageHandle);
	QSize imageSize;
	if (data != nullptr) imageSize = imageHandle->libraryInstance->getImageSize(data);
	return imageSize;
}


void ImageLibrary::flipImage(ImageHandle& imageHandle, FlipMode flipMode)
{
	imageHandle = ensureCurrentLibraryInstance(imageHandle);
	ImageHandleData* data = getImageHandleDataFromHandle(imageHandle);
	if (data != nullptr) imageHandle->libraryInstance->flipImage(data, flipMode);
}

void ImageLibrary::rotateImage(ImageHandle& imageHandle, Rotation rotation)
{
	imageHandle = ensureCurrentLibraryInstance(imageHandle);
	ImageHandleData* data = getImageHandleDataFromHandle(imageHandle);
	if (data != nullptr) imageHandle->libraryInstance->rotateImage(data, rotation);
}

void ImageLibrary::resizeImageFast(ImageHandle& imageHandle, int newWidth, int newHeight)
{
	// don't touch the image if the new size is weird
	if (newWidth <= 0 || newHeight <= 0) return;

	imageHandle = ensureCurrentLibraryInstance(imageHandle);
	ImageHandleData* data = getImageHandleDataFromHandle(imageHandle);
	if (data == nullptr) return;

	// only perform a resize if the requested image dimensions differ from the current ones
	auto imlib = imageHandle->libraryInstance;
	if (imlib->getImageSize(data) != QSize(newWidth, newHeight)) {
		imlib->resizeImageFast(data, newWidth, newHeight);
	}
}

void ImageLibrary::resizeImageSmooth(ImageHandle& imageHandle, int newWidth, int newHeight)
{
	// don't touch the image if the new size is weird
	if (newWidth <= 0 || newHeight <= 0) return;

	imageHandle = ensureCurrentLibraryInstance(imageHandle);
	ImageHandleData* data = getImageHandleDataFromHandle(imageHandle);
	if (data == nullptr) return;

	// only perform a resize if the requested image dimensions differ from the current ones
	auto imlib = imageHandle->libraryInstance;
	if (imlib->getImageSize(data) == QSize(newWidth, newHeight)) return;

	// try to use the library to perform the resize
	if (imlib->resizeImageSmooth(data, newWidth, newHeight)) return;

	// that failed, so use Qt to do it
	QImage image = imlib->toQImage(data);
	image = image.scaled(newWidth, newHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	ImageHandle newImage = fromQImage(image);
	if (newImage) imageHandle = newImage;
}

bool ImageLibrary::resizeImageSmooth(ImageHandleData* /* data */, int /* newWidth */, int /* newHeight */)
{
	// return false by default, so that libraries that don't support smooth resizing won't have to override the function
	return false;
}


ImageModifiers ImageLibrary::getImageModifiers(const ImageHandle& imageHandle)
{
	const ImageHandleData* data = getImageHandleDataFromHandle(imageHandle);
	return data != nullptr ? imageHandle->libraryInstance->getImageModifiers(data) : ImageModifiers();
}

void ImageLibrary::setImageModifiers(ImageHandle& imageHandle, const ImageModifiers& modifiers)
{
	imageHandle = ensureCurrentLibraryInstance(imageHandle);
	ImageHandleData* data = getImageHandleDataFromHandle(imageHandle);
	if (data != nullptr) imageHandle->libraryInstance->setImageModifiers(data, modifiers);
}


QImage ImageLibrary::toQImage(const ImageHandle& imageHandle)
{
	const ImageHandleData* data = getImageHandleDataFromHandle(imageHandle);
	return data != nullptr ? imageHandle->libraryInstance->toQImage(data) : QImage();
}


bool ImageLibrary::freeImage(ImageHandle& imageHandle)
{
	if (getImageHandleDataFromHandle(imageHandle) == nullptr) return false;

	// the image resources are only freed when all references to it are gone
	imageHandle.reset();
	return true;
}


/*
	utility functions
*/

QImage ImageLibrary::loadImageWithQt(const QString& filename) const
{
	QImage image(filename);
	if (!image.isNull()) {
		if (image.depth() != 32)
			image = image.convertToFormat(QImage::Format_RGB32);
	}
	return image;
}


ImageLibrary::ImageHandle ImageLibrary::createImageHandle(ImageHandleData* data) const
{
	return ImageHandle(data != nullptr ? new ImageHandleDataWrapper(data, self()) : nullptr);
}

ImageHandle ImageLibrary::ensureCurrentLibraryInstance(const ImageHandle& imageHandle)
{
	ImageHandleData* data = getImageHandleDataFromHandle(imageHandle);
	if (data == nullptr) return ImageHandle();

	auto currentImlib = get();
	auto imageImlib = imageHandle->libraryInstance;
	ImageHandle finalImageHandle;
	if (imageImlib == currentImlib || !currentImlib->isImageBoundToLibraryInstance()) {
		// the handle already references the current library instance
		finalImageHandle = imageHandle;
	} else {
		// export the source image's data (image + modifiers)
		const auto imageModifiers = imageImlib->getImageModifiers(data);
		imageImlib->setImageModifiers(data, ImageModifiers());
		const QImage exportedImage = imageImlib->toQImage(data);
		imageImlib->setImageModifiers(data, imageModifiers);

		// create a copy of it with the current library instance
		ImageHandleData* copy = currentImlib->createImageFromQImage(exportedImage);
		if (copy != nullptr) {
			currentImlib->setImageModifiers(copy, imageModifiers);
			finalImageHandle = currentImlib->createImageHandle(copy);
		} else {
			finalImageHandle = imageHandle;
		}
	}

	return finalImageHandle;
}

ImageLibrary::ImageHandleData* ImageLibrary::getImageHandleDataFromHandle(const ImageHandle& imageHandle)
{
	return imageHandle ? imageHandle->data : nullptr;
}




ImageHandleDataWrapper::~ImageHandleDataWrapper()
{
	if (!libraryInstance->freeImage(data)) {
		qWarning("failed to free image in handle %p", reinterpret_cast<void*>(this));
	}
	delete data;
}
