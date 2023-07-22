#include "kuickconfig.h"

#include <KConfigGroup>
#include <KSharedConfig>


static int GetRotationValue(Rotation rotation);
static Rotation GetRotationFromValue(int rotation, Rotation defaultValue);

// this instance always exists: it is loaded by KuickShow::KuickShow(), updated by KuickConfigDialog::applyConfig(),
// and saved by KuickShow::slotConfigApplied()
KuickConfig KuickConfig::singleton;




KuickConfig::KuickConfig()
{
	fileFilter = "*.jpeg *.jpg *.gif *.xpm *.ppm *.pgm *.pbm *.pnm *.png *.bmp *.psd *.eim *.tif *.tiff *.xcf";  // *.mng";
	slideDelay = 3000;
	slideshowCycles = 1;
	slideshowFullscreen = true;
	slideshowStartAtFirst = true;

	scrollSteps = 1;
	zoomSteps = 1.5;
	modificationCacheSize = 500;

	startInLastDir = true;

	preloadImage = true;
	autoRotation = true;
	fullScreen = false;

	backgroundColor = Qt::black;

	maxCachedImages = 4;
	maxCache = 10240;

	isModsEnabled = true;

	brightness = 0;
	contrast = 0;
	gamma = 0;

	brightnessSteps = 1;
	contrastSteps = 1;
	gammaSteps = 1;

	brightnessFactor = 10;
	contrastFactor = 10;
	gammaFactor = 10;

	flipVertically = false;
	flipHorizontally = false;
	downScale = true;
	upScale = false;
	maxUpScale = 3;
	maxZoomFactor = 4.0;
	rotation = ROT_0;

	ownPalette = true;
	fastRemap = true;
	fastRender = true;
	dither16bit = false;
	dither8bit = true;
	smoothScale = false;
}

KuickConfig::~KuickConfig()
{
}


void KuickConfig::load()
{
	const auto config = KSharedConfig::openConfig();
	const KConfigGroup generalGroup(config, "GeneralConfiguration");
	const KConfigGroup imlibGroup(config, "ImlibConfiguration");
	KuickConfig defaultConfig;

	fileFilter = generalGroup.readEntry("FileFilter", defaultConfig.fileFilter);
	slideDelay = generalGroup.readEntry("SlideShowDelay", defaultConfig.slideDelay);
	slideshowCycles = generalGroup.readEntry("SlideshowCycles", defaultConfig.slideshowCycles);
	slideshowFullscreen = generalGroup.readEntry("SlideshowFullscreen", defaultConfig.slideshowFullscreen);
	slideshowStartAtFirst = generalGroup.readEntry("SlideshowStartAtFirst", defaultConfig.slideshowStartAtFirst);

	scrollSteps = generalGroup.readEntry("ScrollingStepSize", defaultConfig.scrollSteps);
	zoomSteps = generalGroup.readEntry("ZoomStepSize", defaultConfig.zoomSteps);
	modificationCacheSize = qMax(0, generalGroup.readEntry("ModificationCacheSize", defaultConfig.modificationCacheSize));

	startInLastDir = generalGroup.readEntry("StartInLastDir", defaultConfig.startInLastDir);

	preloadImage = generalGroup.readEntry("PreloadNextImage", defaultConfig.preloadImage);
	autoRotation = generalGroup.readEntry("AutoRotation", defaultConfig.autoRotation);
	fullScreen = generalGroup.readEntry("Fullscreen", defaultConfig.fullScreen);

	backgroundColor = generalGroup.readEntry("BackgroundColor", defaultConfig.backgroundColor);

	maxCachedImages = generalGroup.readEntry("MaxCachedImages", defaultConfig.maxCachedImages);
	maxCache = imlibGroup.readEntry("MaxCacheSize", defaultConfig.maxCache);

	isModsEnabled = generalGroup.readEntry("ApplyDefaultModifications", defaultConfig.isModsEnabled);

	brightness = imlibGroup.readEntry("BrightnessDefault", defaultConfig.brightness);
	contrast = imlibGroup.readEntry("ContrastDefault", defaultConfig.contrast);
	gamma = imlibGroup.readEntry("GammaDefault", defaultConfig.gamma);

	brightnessSteps = generalGroup.readEntry("BrightnessStepSize",defaultConfig.brightnessSteps);
	contrastSteps = generalGroup.readEntry("ContrastStepSize", defaultConfig.contrastSteps);
	gammaSteps = generalGroup.readEntry("GammaStepSize", defaultConfig.gammaSteps);

	brightnessFactor = qAbs(imlibGroup.readEntry("BrightnessFactor", defaultConfig.brightnessFactor));
	contrastFactor = qAbs(imlibGroup.readEntry("ContrastFactor", defaultConfig.contrastFactor));
	gammaFactor = qAbs(imlibGroup.readEntry("GammaFactor", defaultConfig.gammaFactor));

	flipVertically = generalGroup.readEntry("FlipVertically", defaultConfig.flipVertically);
	flipHorizontally = generalGroup.readEntry("FlipHorizontally", defaultConfig.flipHorizontally);
	downScale = generalGroup.readEntry("ShrinkToScreenSize", defaultConfig.downScale);
	upScale = generalGroup.readEntry("ZoomToScreenSize", defaultConfig.upScale);
	maxUpScale = generalGroup.readEntry("MaxUpscale Factor", defaultConfig.maxUpScale);
	maxZoomFactor = generalGroup.readEntry("MaximumZoomFactorByDesktop", defaultConfig.maxZoomFactor);
	int rawRotation = generalGroup.readEntry("Rotation", GetRotationValue(defaultConfig.rotation));
	rotation = GetRotationFromValue(rawRotation, defaultConfig.rotation);

	ownPalette = imlibGroup.readEntry("UseOwnPalette", defaultConfig.ownPalette);
	fastRemap = imlibGroup.readEntry("FastRemapping", defaultConfig.fastRemap);
	fastRender = imlibGroup.readEntry("FastRendering", defaultConfig.fastRender);
	dither16bit = imlibGroup.readEntry("Dither16Bit", defaultConfig.dither16bit);
	dither8bit = imlibGroup.readEntry("Dither8Bit", defaultConfig.dither8bit);
	smoothScale = imlibGroup.readEntry("SmoothScaling", defaultConfig.smoothScale);
}

void KuickConfig::save()
{
	auto config = KSharedConfig::openConfig();
	KConfigGroup generalGroup(config, "GeneralConfiguration");
	KConfigGroup imlibGroup(config, "ImlibConfiguration");

	generalGroup.writeEntry("FileFilter", fileFilter);
	generalGroup.writeEntry("SlideShowDelay", slideDelay);
	generalGroup.writeEntry("SlideshowCycles", slideshowCycles);
	generalGroup.writeEntry("SlideshowFullscreen", slideshowFullscreen);
	generalGroup.writeEntry("SlideshowStartAtFirst", slideshowStartAtFirst);

	generalGroup.writeEntry("ScrollingStepSize", scrollSteps);
	generalGroup.writeEntry("ZoomStepSize", zoomSteps);
	generalGroup.writeEntry("ModificationCacheSize", modificationCacheSize);

	generalGroup.writeEntry("StartInLastDir", startInLastDir);

	generalGroup.writeEntry("PreloadNextImage", preloadImage);
	generalGroup.writeEntry("AutoRotation", autoRotation);
	generalGroup.writeEntry("Fullscreen", fullScreen);

	generalGroup.writeEntry("BackgroundColor", backgroundColor);

	generalGroup.writeEntry("MaxCachedImages", maxCachedImages);
	imlibGroup.writeEntry("MaxCacheSize", maxCache);

	generalGroup.writeEntry("ApplyDefaultModifications", isModsEnabled);

	imlibGroup.writeEntry("BrightnessDefault", brightness);
	imlibGroup.writeEntry("ContrastDefault", contrast);
	imlibGroup.writeEntry("GammaDefault", gamma);

	generalGroup.writeEntry("BrightnessStepSize", brightnessSteps);
	generalGroup.writeEntry("ContrastStepSize", contrastSteps);
	generalGroup.writeEntry("GammaStepSize", gammaSteps);

	imlibGroup.writeEntry("BrightnessFactor", brightnessFactor);
	imlibGroup.writeEntry("ContrastFactor", contrastFactor);
	imlibGroup.writeEntry("GammaFactor", gammaFactor);

	generalGroup.writeEntry("FlipVertically", flipVertically);
	generalGroup.writeEntry("FlipHorizontally", flipHorizontally);
	generalGroup.writeEntry("ShrinkToScreenSize", downScale);
	generalGroup.writeEntry("ZoomToScreenSize", upScale);
	generalGroup.writeEntry("MaxUpscale Factor", maxUpScale);
	generalGroup.writeEntry("MaximumZoomFactorByDesktop", maxZoomFactor);
	generalGroup.writeEntry("Rotation", GetRotationValue(rotation));

	imlibGroup.writeEntry("UseOwnPalette", ownPalette);
	imlibGroup.writeEntry("FastRemapping", fastRemap);
	imlibGroup.writeEntry("FastRendering", fastRender);
	imlibGroup.writeEntry("Dither16Bit", dither16bit);
	imlibGroup.writeEntry("Dither8Bit", dither8bit);
	imlibGroup.writeEntry("SmoothScaling", smoothScale);

	config->sync();
}




/*
	utility functions
*/

static int GetRotationValue(Rotation rotation)
{
	return static_cast<int>(rotation);
}

static Rotation GetRotationFromValue(int rotation, Rotation defaultValue)
{
	// compatibility with KuickShow <= 0.8.3
	switch (rotation) {
		case ROT_0:
		case ROT_90:
		case ROT_180:
		case ROT_270:
			return static_cast<Rotation>(rotation);

		case 90:  return ROT_90;
		case 180: return ROT_180;
		case 270: return ROT_270;

		default:
			// everything else is invalid and the default rotation is used
			return defaultValue;
	}
}
