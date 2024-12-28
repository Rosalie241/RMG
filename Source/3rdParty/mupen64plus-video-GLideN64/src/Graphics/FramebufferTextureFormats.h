#ifndef GRAPHICS_FRAMEBUFFER_TEXTUREFORMATS_H
#define GRAPHICS_FRAMEBUFFER_TEXTUREFORMATS_H
#include "Parameter.h"

namespace graphics {

	struct FramebufferTextureFormats
	{
		InternalColorFormatParam colorInternalFormat;
		ColorFormatParam colorFormat;
		DatatypeParam colorType;
		u32 colorFormatBytes;

		InternalColorFormatParam monochromeInternalFormat;
		ColorFormatParam monochromeFormat;
		DatatypeParam monochromeType;
		u32 monochromeFormatBytes;

		InternalColorFormatParam depthInternalFormat;
		ColorFormatParam depthFormat;
		DatatypeParam depthType;
		u32 depthFormatBytes;

		InternalColorFormatParam depthImageInternalFormat;
		ColorFormatParam depthImageFormat;
		DatatypeParam depthImageType;
		u32 depthImageFormatBytes;

		InternalColorFormatParam lutInternalFormat;
		ColorFormatParam lutFormat;
		DatatypeParam lutType;
		u32 lutFormatBytes;

		// Used for font atlas
		InternalColorFormatParam fontInternalFormat;
		ColorFormatParam fontFormat;
		DatatypeParam fontType;
		u32 fontFormatBytes;

		virtual ~FramebufferTextureFormats() {}
	};

}
#endif // GRAPHICS_FRAMEBUFFER_TEXTUREFORMATS_H
