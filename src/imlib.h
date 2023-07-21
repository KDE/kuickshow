#ifndef KUICKSHOW_IMLIB_H
#define KUICKSHOW_IMLIB_H

#include "kuickconfig.h"

#include <KAboutData>

#include <QImage>
#include <QSize>
#include <QString>

#include <memory>


class ImageLibrary
{
	Q_DISABLE_COPY_MOVE(ImageLibrary)

	protected:
		ImageLibrary();
	public:
		virtual ~ImageLibrary();
		static std::shared_ptr<ImageLibrary> get();
		static void reset();

		virtual bool isInitialized() const noexcept = 0;
		virtual QString getLibraryName() const = 0;
		virtual QString getLibraryVersion() const = 0;
		virtual void addAboutData(KAboutData& aboutData);

		// various functions to query the library's capabilities
		virtual bool isImageBoundToLibraryInstance() const noexcept = 0;
		virtual bool supportsColorPalette() const noexcept = 0;
		virtual bool supportsDithering() const noexcept = 0;
		virtual bool supportsFastRendering() const noexcept = 0;
		virtual bool supportsImageColorModifications() const noexcept = 0;


		// subclasses of ImageLibrary must derive this class and add their own properties to it so that
		// they can correlate an ImageHandle with its image's resources
		struct ImageHandleData
		{
			Q_DISABLE_COPY_MOVE(ImageHandleData)
			ImageHandleData() {}
			virtual ~ImageHandleData() {}
		};

		// The image handle is a shared_ptr of a private data structure (defined in imlib.cpp).
		// Next to the library-specific data, it also contains a reference to the ImageLibrary instance
		// that created the handle.
		// The image handle should be treated as an opaque handle: only the boolean test is supported
		typedef std::shared_ptr<struct ImageHandleDataWrapper> ImageHandle;
		friend struct ImageHandleDataWrapper;


		// the image modifier structure to use in the API
		class ImageModifiers
		{
			public:
				ImageModifiers() : brightness(), contrast(0), gamma(0) {}
				ImageModifiers(int brightness, int contrast, int gamma) : brightness(brightness), contrast(contrast), gamma(gamma) {}

				bool isSet() const noexcept { return brightness != 0 || contrast != 0 || gamma != 0; }

				void setBrightness(int newBrightness) noexcept { brightness = qBound(-256, newBrightness, 256); }
				inline int getBrightness() const noexcept { return brightness; }

				void setContrast(int newContrast) noexcept { contrast = qBound(-256, newContrast, 256); }
				inline int getContrast() const noexcept { return contrast; }

				void setGamma(int newGamma) noexcept { gamma = qBound(-256, newGamma, 256); }
				inline int getGamma() const noexcept { return gamma; }

			private:
				int brightness;
				int contrast;
				int gamma;
		};


		// the public API:
		// the functions are all static since ImageHandle stores a reference to the library instance to use

		static ImageHandle loadImage(const QString& filename);
		static ImageHandle fromQImage(const QImage& image);
		static ImageHandle copyImage(const ImageHandle& imageHandle);
		static bool saveImage(const ImageHandle& imageHandle, const QString& filename);

		static QSize getImageSize(const ImageHandle& imageHandle);

		static void flipImage(ImageHandle& imageHandle, FlipMode flipMode);
		static void rotateImage(ImageHandle& imageHandle, Rotation rotation);
		static void resizeImageFast(ImageHandle& imageHandle, int newWidth, int newHeight);
		static void resizeImageSmooth(ImageHandle& imageHandle, int newWidth, int newHeight);

		static ImageModifiers getImageModifiers(const ImageHandle& imageHandle);
		static void setImageModifiers(ImageHandle& imageHandle, const ImageModifiers& modifiers);

		static QImage toQImage(const ImageHandle& imageHandle);

		static bool freeImage(ImageHandle& imageHandle);


		// some Qt-based image manipulation functions
		QImage loadImageWithQt(const QString& filename) const;


		// the functions reimplemented by the sub-class

	protected:
		virtual ImageHandleData* loadImageWithLibrary(const QString& filename) = 0;
		virtual ImageHandleData* createImageFromQImage(const QImage& image) = 0;
		virtual ImageHandleData* copyImage(const ImageHandleData* data) = 0;
		virtual bool saveImage(const ImageHandleData* data, const QString& filename) = 0;

		virtual QSize getImageSize(const ImageHandleData* data) const = 0;

		virtual void flipImage(ImageHandleData* data, FlipMode flipMode) = 0;
		virtual void rotateImage(ImageHandleData* data, Rotation rotation) = 0;
		virtual void resizeImageFast(ImageHandleData* data, int newWidth, int newHeight) = 0;
		virtual bool resizeImageSmooth(ImageHandleData* data, int newWidth, int newHeight);

		virtual ImageModifiers getImageModifiers(const ImageHandleData* data) const = 0;
		virtual void setImageModifiers(ImageHandleData* data, const ImageModifiers& modifiers) = 0;

		virtual QImage toQImage(const ImageHandleData* data) = 0;

		virtual bool freeImage(ImageHandleData* data) = 0;


		// utility functions
		ImageHandle createImageHandle(ImageHandleData* data) const;
		static ImageHandle ensureCurrentLibraryInstance(const ImageHandle& imageHandle);
		static ImageHandleData* getImageHandleDataFromHandle(const ImageHandle& imageHandle);


		// internal management
	private:
		static std::shared_ptr<ImageLibrary> singleton;

		// this function must be defined in each imlib_xxx.cpp file!
		static ImageLibrary* create();

		// this is set by get() for each new instance, so that they have their own shared_ptr
		std::weak_ptr<ImageLibrary> _self;
	protected:
		std::shared_ptr<ImageLibrary> self() const { return _self.lock(); }
};

// for convenience only
typedef ImageLibrary::ImageHandle ImageHandle;
typedef ImageLibrary::ImageModifiers ImageModifiers;

#endif
