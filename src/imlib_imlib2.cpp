#include "imlib.h"

#include "kuickconfig.h"

#include <KLocalizedString>

#include <QByteArray>
#include <QFile>
#include <QX11Info>

#include <Imlib2.h>


// the image handle data
struct ImageHandleDataImlib2 : public ImageLibrary::ImageHandleData
{
	ImageHandleDataImlib2(Imlib_Image image, const ImageLibrary::ImageModifiers& modifiers) : image(image), modifiers(modifiers) {}
	~ImageHandleDataImlib2();

	Imlib_Image image;
	ImageLibrary::ImageModifiers modifiers;
};


// the subclass of ImageLibrary
class ImageLibraryImlib2 : public ImageLibrary
{
	public:
		ImageLibraryImlib2();
		~ImageLibraryImlib2();

		bool isInitialized() const noexcept override { return initialized; }
		QString getLibraryName() const override { return "Imlib2"; }
		QString getLibraryVersion() const override { return IMAGE_LIBRARY_VERSION; }
		void addAboutData(KAboutData& aboutData) override;

		bool isImageBoundToLibraryInstance() const noexcept override { return false; }
		bool supportsColorPalette() const noexcept override { return false; }
		bool supportsDithering() const noexcept override { return true; }
		bool supportsFastRendering() const noexcept override { return false; }
		bool supportsImageColorModifications() const noexcept override { return true; }


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

		ImageModifiers getImageModifiers(const ImageHandleData* data) const override;
		void setImageModifiers(ImageHandleData* data, const ImageModifiers& modifiers) override;

		QImage toQImage(const ImageHandleData* data) override;

		bool freeImage(ImageHandleData* data) override;


	private:
		// initialization of the library
		bool initialized;
		bool initialize();

		// various imlib-specific data
		Display* display;
		Visual* visual;

		// utility functions
		ImageHandleData* createImageHandleData(Imlib_Image image, const ImageModifiers& modifiers = ImageModifiers()) const;
		inline const ImageHandleDataImlib2* castData(const ImageHandleData* data) const {
			return dynamic_cast<const ImageHandleDataImlib2*>(data);
		}
		inline ImageHandleDataImlib2* castData(ImageHandleData* data) const {
			return dynamic_cast<ImageHandleDataImlib2*>(data);
		}

		Imlib_Image createImlibImageFromQImage(const QImage& image);
		bool applyImageModifiers(const ImageModifiers& modifiers);
};




/*
	implementation of ImageLibraryImlib2
*/

ImageLibraryImlib2::ImageLibraryImlib2() :
	initialized(false),
	display(nullptr), visual(nullptr)
{
	display = QX11Info::display();
	visual = DefaultVisual(display, DefaultScreen(display));

	initialized = initialize();
}

ImageLibraryImlib2::~ImageLibraryImlib2()
{
}


// implementation of the base class' factory function
ImageLibrary* ImageLibrary::create()
{
	return new ImageLibraryImlib2();
}


void ImageLibraryImlib2::addAboutData(KAboutData& aboutData)
{
	// BSDL from SourceForge home page
	aboutData.addComponent(getLibraryName(), i18n("Image processing library"), getLibraryVersion(),
		"https://docs.enlightenment.org/api/imlib2/html/", KAboutLicense::BSDL);
}


/*! \brief Initializes the Imlib2 library.
 *
 * This function must only be called once per instance!
 */
bool ImageLibraryImlib2::initialize()
{
	const KuickConfig& config = KuickConfig::get();
	Colormap colorMap = DefaultColormap(display, DefaultScreen(display));

	imlib_context_set_display(display);
	imlib_context_set_visual(visual);
	imlib_context_set_colormap(colorMap);
	imlib_set_cache_size(config.maxCache * 1024);
	// the maximum number of colors to allocate for 8bpp and less
	imlib_set_color_usage(128);
	// dither for depths < 24bpp
	imlib_context_set_dither((config.dither8bit || config.dither16bit) ? 1 : 0);

	return true;
}


/*
	the library-specific API implementation
*/

ImageLibrary::ImageHandleData* ImageLibraryImlib2::loadImageWithLibrary(const QString& filename)
{
	// first, try to load the image with Imlib2
	Imlib_Image image = imlib_load_image(QFile::encodeName(filename).data());

	// if that failed, e.g. because the image format is not supported by Imlib, try loading the image with Qt
	if (image == nullptr) {
		QImage qtImage(filename);
		image = createImlibImageFromQImage(qtImage);
	}

	return createImageHandleData(image);
}

ImageLibrary::ImageHandleData* ImageLibraryImlib2::createImageFromQImage(const QImage& image)
{
	return createImageHandleData(createImlibImageFromQImage(image));
}

ImageLibrary::ImageHandleData* ImageLibraryImlib2::copyImage(const ImageHandleData* data)
{
	const ImageHandleDataImlib2* d = castData(data);
	imlib_context_set_image(d->image);
	return createImageHandleData(imlib_clone_image(), d->modifiers);
}

bool ImageLibraryImlib2::saveImage(const ImageHandleData* data, const QString& filename)
{
	const ImageHandleDataImlib2* d = castData(data);
	Imlib_Image image = d->image;

	// if there are image modifications create a copy of the image and apply them to it
	bool freeSaveImage = getImageModifiers(d).isSet();
	imlib_context_set_image(image);

	if (freeSaveImage) {
		image = imlib_clone_image();
		if (image == nullptr) {
			qWarning("Imlib2: failed to create copy of image \"%s\" for saving", qPrintable(filename));
			return false;
		}

		imlib_context_set_image(image);
		applyImageModifiers(d->modifiers);
		imlib_apply_color_modifier();
	}

	// save the image and clean up
	int err;
	imlib_save_image_with_errno_return(QFile::encodeName(filename), &err);
	if (err != 0) qWarning("Imlib2: failed to save image to \"%s\": errno = %d", qPrintable(filename), err);
	if (freeSaveImage) imlib_free_image_and_decache();

	return err == 0;
}


QSize ImageLibraryImlib2::getImageSize(const ImageHandleData* data) const
{
	imlib_context_set_image(castData(data)->image);
	return QSize(imlib_image_get_width(), imlib_image_get_height());
}


void ImageLibraryImlib2::flipImage(ImageHandleData* data, FlipMode flipMode)
{
	imlib_context_set_image(castData(data)->image);
	if (flipMode & FlipHorizontal) imlib_image_flip_horizontal();
	if (flipMode & FlipVertical) imlib_image_flip_vertical();
}

void ImageLibraryImlib2::rotateImage(ImageHandleData* data, Rotation rotation)
{
	bool diagonalFlip = false;;
	FlipMode flipMode = FlipNone;

	switch (rotation) {
		case ROT_0:
			// no-op
			break;

		case ROT_90:
			diagonalFlip = true;
			flipMode = FlipHorizontal;
			break;

		case ROT_180:
			flipMode = static_cast<FlipMode>(FlipHorizontal | FlipVertical);
			break;

		case ROT_270:
			diagonalFlip = true;
			flipMode = FlipVertical;
	}

	imlib_context_set_image(castData(data)->image);
	if (diagonalFlip) imlib_image_flip_diagonal();
	if (flipMode != FlipNone) flipImage(data, flipMode);
}

void ImageLibraryImlib2::resizeImageFast(ImageHandleData* data, int newWidth, int newHeight)
{
	ImageHandleDataImlib2* d = castData(data);
	const QSize size = getImageSize(data);
	imlib_context_set_image(d->image);
	Imlib_Image copy = imlib_create_cropped_scaled_image(0, 0, size.width(), size.height(), newWidth, newHeight);
	if (copy != nullptr) {
		freeImage(data);
		d->image = copy;
	}
}


ImageModifiers ImageLibraryImlib2::getImageModifiers(const ImageHandleData* data) const
{
	return castData(data)->modifiers;
}

void ImageLibraryImlib2::setImageModifiers(ImageHandleData* data, const ImageModifiers& modifiers)
{
	castData(data)->modifiers = modifiers;
}


QImage ImageLibraryImlib2::toQImage(const ImageHandleData* data)
{
	// Imlib2 copies Imlib's behavior and only renders the image modifiers into a Pixmap.
	// Create a copy of the source image (don't modify the source) and apply the modifiers directly to the copied image.
	const ImageHandleDataImlib2* d = castData(data);
	imlib_context_set_image(d->image);
	Imlib_Image sourceImage = imlib_clone_image();
	bool freeSourceImage;

	if (sourceImage != nullptr) {
		freeSourceImage = true;
		imlib_context_set_image(sourceImage);
		applyImageModifiers(d->modifiers);
		imlib_apply_color_modifier();
	} else {
		freeSourceImage = false;
		sourceImage = d->image;
	}

	// copy the source image's pixel data to the final QImage
	const int width = imlib_image_get_width();
	const int height = imlib_image_get_height();
	const DATA32* sourceImageData = imlib_image_get_data_for_reading_only();
	const DATA32* sourceImageDataPtr = sourceImageData;

	// create the target image
	QImage targetImage(width, height, QImage::Format_RGB32);
	for (int y = 0; y < height; y++) {
		QRgb* targetImageData = reinterpret_cast<QRgb*>(targetImage.scanLine(y));
		for (int x = 0; x < width; x++) {
			DATA32 pixel = *(sourceImageDataPtr++);
			uchar r = (pixel & 0x00ff0000) >> 16;
			uchar g = (pixel & 0x0000ff00) >> 8;
			uchar b = (pixel & 0x000000ff);
			targetImageData[x] = qRgb(r, g, b);
		}
	}

	// clean up
	if (freeSourceImage) imlib_free_image_and_decache();

	return targetImage;
}


bool ImageLibraryImlib2::freeImage(ImageHandleData* data)
{
	ImageHandleDataImlib2* d = castData(data);
	imlib_context_set_image(d->image);
	imlib_free_image_and_decache();
	d->image = nullptr;
	return true;
}


Imlib_Image ImageLibraryImlib2::createImlibImageFromQImage(const QImage& image)
{
	// ensure that the image has a depth of 32 bits
	if (image.isNull()) return nullptr;
	const QImage sourceImage = image.depth() == 32 ? image : image.convertToFormat(QImage::Format_RGB32);

	// convert
	const int width = sourceImage.width();
	const int height = sourceImage.height();
	DATA32* imageData = new DATA32[width * height];
	DATA32* imageDataPtr = imageData;

	for (int y = 0; y < height; y++) {
		const QRgb* scanLine = reinterpret_cast<const QRgb*>(sourceImage.constScanLine(y));
		for (int x = 0; x < width; x++) {
			*(imageDataPtr++) = scanLine[x];
		}
	}

	// create the Imlib image from this data
	Imlib_Image imlibImage = imlib_create_image_using_copied_data(width, height, imageData);
	delete[] imageData;

	return imlibImage;
}


// even though the destructor is empty, it is defined here to avoid inlining
ImageHandleDataImlib2::~ImageHandleDataImlib2()
{
}




/*
	utility functions
*/

ImageLibrary::ImageHandleData* ImageLibraryImlib2::createImageHandleData(Imlib_Image image, const ImageModifiers& modifiers) const
{
	return image != nullptr ? new ImageHandleDataImlib2(image, modifiers) : nullptr;
}

bool ImageLibraryImlib2::applyImageModifiers(const ImageModifiers& modifiers)
{
	// make sure there's an active color modifier (this will never be deleted!)
	if (imlib_context_get_color_modifier() == nullptr) {
		Imlib_Color_Modifier mod = imlib_create_color_modifier();
		if (mod == nullptr) return false;
		imlib_context_set_color_modifier(mod);
	}

	// reset it and apply the given values
	imlib_reset_color_modifier();
	if (modifiers.isSet()) {
		imlib_modify_color_modifier_brightness(static_cast<double>(modifiers.getBrightness()) / 256);
		imlib_modify_color_modifier_contrast(static_cast<double>(modifiers.getContrast()) / 256 + 1);
		imlib_modify_color_modifier_gamma(static_cast<double>(modifiers.getGamma()) / 256 + 1);
	}

	return true;
}
