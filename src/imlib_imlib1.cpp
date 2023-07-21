#include "imlib.h"

#include "kuickconfig.h"

#include <KLocalizedString>

#include <QByteArray>
#include <QFile>
#include <QStandardPaths>
#include <QX11Info>

#include <Imlib.h>
#include <stdlib.h>


/*
	Including <Imlib.h> has a nasty side effect:
	It pulls in <Xos.h> which defines a couple of macros like "index", "rindex", "None" or
	"Enum" which clash with the typical naming of Qt variables and parameters.

	The following lines move all macros that cause compile errors in this file.
	(Imlib.h is only #include'd here.)
*/

#undef index
#undef rindex

#undef Bool
#undef Color
#undef CursorShape
#undef Expose
#undef FocusIn
#undef FocusOut
#undef FontChange
#undef GrayScale
#undef KeyPress
#undef KeyRelease
#undef None
#undef Status
#undef Unsorted


// the image handle data
struct ImageHandleDataImlib1 : public ImageLibrary::ImageHandleData
{
	explicit ImageHandleDataImlib1(ImlibImage* image) : image(image) {}
	~ImageHandleDataImlib1();

	ImlibImage* image;
};


// the subclass of ImageLibrary
class ImageLibraryImlib1 : public ImageLibrary
{
	public:
		ImageLibraryImlib1();
		~ImageLibraryImlib1();

		bool isInitialized() const noexcept override { return initialized; }
		QString getLibraryName() const override { return "Imlib"; }
		QString getLibraryVersion() const override { return IMAGE_LIBRARY_VERSION; }
		void addAboutData(KAboutData& aboutData) override;

		bool isImageBoundToLibraryInstance() const noexcept override { return true; }
		bool supportsColorPalette() const noexcept override { return true; }
		bool supportsDithering() const noexcept override { return true; }
		bool supportsFastRendering() const noexcept override { return true; }
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
		ImlibInitParams getDefaultInitParams() const;
		bool addOwnPaletteFile(ImlibInitParams& initParams);

		// various imlib-specific data
		ImlibData* id;
		Display* display;
		Visual* visual;

		// contains the absolute path to the shipped palette file
		QByteArray paletteFilename;
		void initializePaletteFilename();

		// utility functions
		ImageHandleData* createImageHandleData(ImlibImage* image) const;
		inline const ImageHandleDataImlib1* castData(const ImageHandleData* data) const {
			return dynamic_cast<const ImageHandleDataImlib1*>(data);
		}
		inline ImageHandleDataImlib1* castData(ImageHandleData* data) const {
			return dynamic_cast<ImageHandleDataImlib1*>(data);
		}

		ImlibImage* createImlibImageFromQImage(const QImage& image);
};




/*
	implementation of ImageLibraryImlib1
*/

ImageLibraryImlib1::ImageLibraryImlib1() :
	initialized(false),
	id(nullptr), display(nullptr), visual(nullptr)
{
	display = QX11Info::display();
	visual = DefaultVisual(display, DefaultScreen(display));

	initializePaletteFilename();
	initialized = initialize();
}

ImageLibraryImlib1::~ImageLibraryImlib1()
{
	// Imlib1 doesn't provide an uninit function and the documentation doesn't state what to do with this pointer after
	// usage. The source code uses malloc(), so we're using free() here to clean up.
	free(id);
}


// implementation of the base class' factory function
ImageLibrary* ImageLibrary::create()
{
	return new ImageLibraryImlib1();
}


void ImageLibraryImlib1::addAboutData(KAboutData& aboutData)
{
	// LGPL from package README file
	aboutData.addComponent(getLibraryName(), i18n("Image loading and rendering library"), getLibraryVersion(),
		"http://web.mit.edu/graphics/src/imlib-1.7/doc/", KAboutLicense::LGPL);
}


/*! \brief Initializes the Imlib library.
 *
 * This function must only be called once per instance!
 */
bool ImageLibraryImlib1::initialize()
{
	// first, try the initialization without the shipped palette
	ImlibInitParams initParams = getDefaultInitParams();
	id = Imlib_init_with_params(display, &initParams);
	if (id == nullptr) {
		// if that failes try with the shipped palette
		initParams = getDefaultInitParams();
		if (addOwnPaletteFile(initParams)) {
			qWarning("failed to initialize Imlib, trying with shipped palette file now: %s", paletteFilename.constData());
			id = Imlib_init_with_params(display, &initParams);
		}

		if (id == nullptr) qCritical("failed to initialize Imlib");
	}

	return id != nullptr;
}

ImlibInitParams ImageLibraryImlib1::getDefaultInitParams() const
{
	const KuickConfig& config = KuickConfig::get();
	const int cacheSizeInBytes = config.maxCache * 1024;

	ImlibInitParams initParams;
	initParams.flags = PARAMS_PALETTEOVERRIDE |
		PARAMS_REMAP | PARAMS_FASTRENDER | PARAMS_HIQUALITY | PARAMS_DITHER |
		PARAMS_SHAREDMEM | PARAMS_SHAREDPIXMAPS |
		PARAMS_VISUALID |
		PARAMS_IMAGECACHESIZE | PARAMS_PIXMAPCACHESIZE;

	initParams.paletteoverride = config.ownPalette  ? 1 : 0;
	initParams.remap           = config.fastRemap   ? 1 : 0;
	initParams.fastrender      = config.fastRender  ? 1 : 0;
	initParams.hiquality       = config.dither16bit ? 1 : 0;
	initParams.dither          = config.dither8bit  ? 1 : 0;
	initParams.sharedmem       = 1;
	initParams.sharedpixmaps   = 1;
	initParams.visualid        = visual->visualid;
	initParams.imagecachesize  = cacheSizeInBytes;
	initParams.pixmapcachesize = cacheSizeInBytes;  // this must actually be in bits!

	return initParams;
}

bool ImageLibraryImlib1::addOwnPaletteFile(ImlibInitParams& initParams)
{
	if (paletteFilename.isEmpty()) return false;

	initParams.palettefile = paletteFilename.data();
	initParams.flags |= PARAMS_PALETTEOVERRIDE | PARAMS_PALETTEFILE;
	return true;
}


/*! \brief Initializes the storage for the shipped palette file's name.
 *
 * This construct is required because the file's name is stored in a QString
 */
void ImageLibraryImlib1::initializePaletteFilename()
{
	static const QString RelativePaletteFilename = "kuickshow/im_palette.pal";

	const QString filename = QStandardPaths::locate(QStandardPaths::GenericDataLocation, RelativePaletteFilename);
	if (filename.isEmpty()) {
		qWarning("failed to locate own palette file: %s", qPrintable(RelativePaletteFilename));
	} else {
		paletteFilename = filename.toLocal8Bit();
	}
}


/*
	the library-specific API implementation
*/

ImageLibrary::ImageHandleData* ImageLibraryImlib1::loadImageWithLibrary(const QString& filename)
{
	// first, try to load the image with Imlib
	ImlibImage* image = Imlib_load_image(id, QFile::encodeName(filename).data());

	// if that failed, e.g. because the image format is not supported by Imlib, try loading the image with Qt
	if (image == nullptr) {
		QImage qtImage(filename);
		image = createImlibImageFromQImage(qtImage);
	}

	return createImageHandleData(image);
}

ImageLibrary::ImageHandleData* ImageLibraryImlib1::createImageFromQImage(const QImage& image)
{
	return createImageHandleData(createImlibImageFromQImage(image));
}

ImageLibrary::ImageHandleData* ImageLibraryImlib1::copyImage(const ImageHandleData* data)
{
	ImlibImage* copy = Imlib_clone_image(id, castData(data)->image);
	return createImageHandleData(copy);
}

bool ImageLibraryImlib1::saveImage(const ImageHandleData* data, const QString& filename)
{
	const ImageHandleDataImlib1* d = castData(data);
	ImlibImage* image = d->image;
	bool freeSaveImage = false;

	// if there are image modifications create a copy of the image and apply them to it
	if (getImageModifiers(d).isSet()) {
		freeSaveImage = true;
		image = Imlib_clone_image(id, d->image);
		if (image == nullptr) {
			qWarning("Imlib: failed to create copy of image \"%s\" or saving", qPrintable(filename));
			return false;
		}

		ImlibColorModifier mod;
		Imlib_get_image_modifier(id, d->image, &mod);
		Imlib_set_image_modifier(id, image, &mod);
		Imlib_apply_modifiers_to_rgb(id, image);
	}

	// save the image and clean up
	bool success = Imlib_save_image(id, image, QFile::encodeName(filename).data(), nullptr);
	if (freeSaveImage) Imlib_kill_image(id, image);

	return success;
}


QSize ImageLibraryImlib1::getImageSize(const ImageHandleData* data) const
{
	const ImlibImage* image = castData(data)->image;
	return QSize(image->rgb_width, image->rgb_height);
}


void ImageLibraryImlib1::flipImage(ImageHandleData* data, FlipMode flipMode)
{
	ImlibImage* image = castData(data)->image;
	if (flipMode & FlipHorizontal) Imlib_flip_image_horizontal(id, image);
	if (flipMode & FlipVertical) Imlib_flip_image_vertical(id, image);
}

void ImageLibraryImlib1::rotateImage(ImageHandleData* data, Rotation rotation)
{
	int rot = 0;
	FlipMode flipMode = FlipNone;

	switch (rotation) {
		case ROT_0:
			// no-op
			break;

		case ROT_90:
			rot = -1;
			flipMode = FlipHorizontal;
			break;

		case ROT_180:
			flipMode = static_cast<FlipMode>(FlipHorizontal | FlipVertical);
			break;

		case ROT_270:
			rot = -1;
			flipMode = FlipVertical;
	}

	ImlibImage* image = castData(data)->image;
	if (rot != 0) Imlib_rotate_image(id, image, rot);
	if (flipMode != FlipNone) flipImage(data, flipMode);
}

void ImageLibraryImlib1::resizeImageFast(ImageHandleData* data, int newWidth, int newHeight)
{
	ImageHandleDataImlib1* d = castData(data);
	ImlibImage* copy = Imlib_clone_scaled_image(id, d->image, newWidth, newHeight);
	if (copy != nullptr) {
		freeImage(data);
		d->image = copy;
	}
}


// Note: Imlib's color modifier values have a range from 0 to 512 with 256 being the middle
ImageModifiers ImageLibraryImlib1::getImageModifiers(const ImageHandleData* data) const
{
	ImlibImage* image = castData(data)->image;
	ImlibColorModifier mod;
	Imlib_get_image_modifier(id, image, &mod);
	return ImageModifiers(mod.brightness - 256, mod.contrast - 256, mod.gamma - 256);
}

void ImageLibraryImlib1::setImageModifiers(ImageHandleData* data, const ImageModifiers& modifiers)
{
	ImlibImage* image = castData(data)->image;

	ImlibColorModifier mod;
	mod.brightness = modifiers.getBrightness() + 256;
	mod.contrast = modifiers.getContrast() + 256;
	mod.gamma = modifiers.getGamma() + 256;
	Imlib_set_image_modifier(id, image, &mod);
}


QImage ImageLibraryImlib1::toQImage(const ImageHandleData* data)
{
	// The original implementation used a Pixmap to apply the image modifiers. Since we're now using QImage as a base,
	// we need to copy the source image and apply the modifiers directly to it (don't touch the original).
	ImlibImage* image = castData(data)->image;
	ImlibImage* sourceImage = Imlib_clone_image(id, image);
	bool freeSourceImage;

	if (sourceImage != nullptr) {
		freeSourceImage = true;
		ImlibColorModifier mod;
		Imlib_get_image_modifier(id, image, &mod);
		Imlib_set_image_modifier(id, sourceImage, &mod);
		Imlib_apply_modifiers_to_rgb(id, sourceImage);
	} else {
		// the image-copy failed, so just use the source image without modifiers
		freeSourceImage = false;
		sourceImage = image;
	}

	// copy the source image's pixel data to the final QImage
	const int width = sourceImage->rgb_width;
	const int height = sourceImage->rgb_height;
	const uchar* sourceImageData = sourceImage->rgb_data;
	const uchar* sourceImageDataPtr = sourceImageData;

	// create the target image
	QImage targetImage(width, height, QImage::Format_RGB32);
	for (int y = 0; y < height; y++) {
		QRgb* targetImageData = reinterpret_cast<QRgb*>(targetImage.scanLine(y));
		for (int x = 0; x < width; x++) {
			uchar r = *(sourceImageDataPtr++);
			uchar g = *(sourceImageDataPtr++);
			uchar b = *(sourceImageDataPtr++);
			targetImageData[x] = qRgb(r, g, b);
		}
	}

	// clean up
	if (freeSourceImage) Imlib_kill_image(id, sourceImage);

	return targetImage;
}


bool ImageLibraryImlib1::freeImage(ImageHandleData* data)
{
	ImageHandleDataImlib1* d = castData(data);
	// use Imlib_kill_image() to avoid any additional library-side caching
	Imlib_kill_image(id, d->image);
	d->image = nullptr;
	return true;
}


ImlibImage* ImageLibraryImlib1::createImlibImageFromQImage(const QImage& image)
{
	// ensure that the image has a depth of 32 bits
	if (image.isNull()) return nullptr;
	const QImage sourceImage = image.depth() == 32 ? image : image.convertToFormat(QImage::Format_RGB32);

	// convert to 24 bpp (discard alpha)
	const int width = sourceImage.width();
	const int height = sourceImage.height();
	uchar* targetImageData = new uchar[width * height * 3];  // number of pixels with 24bbp
	uchar* targetImageDataPtr = targetImageData;

	for (int y = 0; y < height; y++) {
		const QRgb* sourceImageData = reinterpret_cast<const QRgb*>(sourceImage.constScanLine(y));
		for (int x = 0; x < width; x++) {
			const QRgb& pixel = sourceImageData[x];
			*(targetImageDataPtr++) = qRed(pixel);
			*(targetImageDataPtr++) = qGreen(pixel);
			*(targetImageDataPtr++) = qBlue(pixel);
		}
	}

	// create the Imlib image from this data
	ImlibImage* imlibImage = Imlib_create_image_from_data(id, targetImageData, nullptr, width, height);
	delete[] targetImageData;

	return imlibImage;
}


// even though the destructor is empty, it is defined here to avoid inlining
ImageHandleDataImlib1::~ImageHandleDataImlib1()
{
}




/*
	utility functions
*/

ImageLibrary::ImageHandleData* ImageLibraryImlib1::createImageHandleData(ImlibImage* image) const
{
	return image != nullptr ? new ImageHandleDataImlib1(image) : nullptr;
}
