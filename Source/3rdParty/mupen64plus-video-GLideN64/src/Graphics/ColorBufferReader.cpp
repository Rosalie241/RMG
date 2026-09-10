
#include "ColorBufferReader.h"
#include "FramebufferTextureFormats.h"
#include "Context.h"
#include "Parameters.h"
#include <algorithm>
#include <cstring>

namespace graphics {

	ColorBufferReader::ColorBufferReader(CachedTexture * _pTexture)
		: m_pTexture(_pTexture)
	{
		m_pixelData.resize(m_pTexture->textureBytes);
		m_tempPixelData.resize(m_pTexture->textureBytes);
	}

	const u8* ColorBufferReader::_convertFloatTextureBuffer(const u8* _gpuData, size_t _gpuDataSize, u32 _width,
		u32 _height, u32 _heightOffset, u32 _stride)
	{
		const u32 colorsPerPixel = 4;
		const u32 widthPixels = _width * colorsPerPixel;
		const u32 stridePixels = _stride * colorsPerPixel;

		// _height is supplied by the caller and is not bounded by the texture,
		// while m_tempPixelData is sized for the texture. An oversized _height
		// therefore made this copy run past the end of the staging buffer.
		// Computed in size_t: the old int could overflow before being used.
		// Also bounded by what is actually readable through _gpuData.
		const size_t bytesRequested =
			static_cast<size_t>(m_pTexture->width) * _height * colorsPerPixel * sizeof(float);
		const size_t bytesToCopy =
			std::min(std::min(bytesRequested, m_tempPixelData.size()), _gpuDataSize);
		std::copy_n(_gpuData, bytesToCopy, m_tempPixelData.data());

		u8* pixelDataAlloc = m_pixelData.data();
		const float* pixelData = reinterpret_cast<const float*>(m_tempPixelData.data());

		// Rows must fit the destination...
		if (widthPixels != 0 && _height * widthPixels > m_pixelData.size())
			_height = static_cast<u32>(m_pixelData.size()) / widthPixels;

		// ...and must stay inside the part of the staging buffer that was
		// actually filled above. The last element the loop reads is
		// (_height - 1 + _heightOffset) * stridePixels + widthPixels - 1, and
		// _heightOffset and _stride are non-zero for the EGL image reader.
		const size_t floatsCopied = bytesToCopy / sizeof(float);
		size_t rowsReadable = 0;
		if (floatsCopied >= widthPixels) {
			const size_t rowsSpanned = stridePixels != 0 ?
				(floatsCopied - widthPixels) / stridePixels + 1 : 1;
			if (rowsSpanned > _heightOffset)
				rowsReadable = rowsSpanned - _heightOffset;
		}
		if (_height > rowsReadable)
			_height = static_cast<u32>(rowsReadable);

		for (u32 heightIndex = 0; heightIndex < _height; ++heightIndex) {
			for (u32 widthIndex = 0; widthIndex < widthPixels; ++widthIndex) {
				u8& dest = *(pixelDataAlloc + heightIndex*widthPixels + widthIndex);
				const float& src = *(pixelData + (heightIndex+_heightOffset)*stridePixels + widthIndex);
				dest = static_cast<u8>(src*255.0);
			}
		}

		return pixelDataAlloc;
	}

	const u8* ColorBufferReader::_convertIntegerTextureBuffer(const u8* _gpuData, size_t _gpuDataSize, u32 _width,
		u32 _height, u32 _heightOffset, u32 _stride, u32 _colorsPerPixel)
	{
		const u32 widthBytes = _width * _colorsPerPixel;
		const u32 strideBytes = _stride * _colorsPerPixel;

		u8* pixelDataAlloc = m_pixelData.data();

		// Rows must fit the destination...
		if (widthBytes != 0 && _height * widthBytes > m_pixelData.size())
			_height = static_cast<u32>(m_pixelData.size()) / widthBytes;

		// ...and must stay inside the source. Unlike the float path there is
		// no staging copy here, so the memcpy below reads straight out of the
		// GPU buffer: the last byte touched is
		// (_height - 1 + _heightOffset) * strideBytes + widthBytes - 1.
		size_t rowsReadable = 0;
		if (_gpuDataSize >= widthBytes) {
			const size_t rowsSpanned = strideBytes != 0 ?
				(_gpuDataSize - widthBytes) / strideBytes + 1 : 1;
			if (rowsSpanned > _heightOffset)
				rowsReadable = rowsSpanned - _heightOffset;
		}
		if (_height > rowsReadable)
			_height = static_cast<u32>(rowsReadable);

		for (u32 index = 0; index < _height; ++index) {
			memcpy(pixelDataAlloc + index * widthBytes, _gpuData + ((index + _heightOffset) * strideBytes), widthBytes);
		}

		return pixelDataAlloc;
	}


	const u8 * ColorBufferReader::readPixels(s32 _x0, s32 _y0, u32 _width, u32 _height, u32 _size, bool _sync)
	{
		const FramebufferTextureFormats & fbTexFormat = gfxContext.getFramebufferTextureFormats();

		ReadColorBufferParams params;
		params.x0 = _x0;
		params.y0 = _y0;
		params.width = _width;
		params.height = _height;
		params.sync = _sync;

		if (_size > G_IM_SIZ_8b) {
			params.colorFormat = fbTexFormat.colorFormat;
			params.colorType = fbTexFormat.colorType;
			params.colorFormatBytes = fbTexFormat.colorFormatBytes;
		} else {
			params.colorFormat = fbTexFormat.monochromeFormat;
			params.colorType = fbTexFormat.monochromeType;
			params.colorFormatBytes = fbTexFormat.monochromeFormatBytes;
		}

		u32 heightOffset = 0;
		u32 stride = 0;
		size_t gpuDataSize = 0;
		const u8* pixelData = _readPixels(params, heightOffset, stride, gpuDataSize);

		if (pixelData == nullptr)
			return nullptr;

		if (params.colorType == datatype::FLOAT && _size > G_IM_SIZ_8b) {
			return _convertFloatTextureBuffer(pixelData, gpuDataSize, params.width, params.height, heightOffset,
											  stride);
		} else {
			return _convertIntegerTextureBuffer(pixelData, gpuDataSize, params.width, params.height, heightOffset,
												stride, params.colorFormatBytes);
		}
	}
}