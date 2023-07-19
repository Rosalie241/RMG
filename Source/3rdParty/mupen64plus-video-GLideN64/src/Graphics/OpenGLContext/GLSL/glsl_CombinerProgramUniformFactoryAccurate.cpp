#include "glsl_CombinerProgramUniformFactoryAccurate.h"
#include <Config.h>
#include <FrameBuffer.h>
#include <Textures.h>
#include <DisplayWindow.h>
#include <Debugger.h>

#include <cmath>

namespace {
using namespace glsl;

class URasterInfo : public UniformGroup {
public:
	URasterInfo(GLuint _program) {
		LocateUniform(uVertexOffset);
		LocateUniform(uTexCoordOffset);
		LocateUniform(uUseTexCoordBounds);
		LocateUniform(uTexCoordBounds);
	}

	void update(bool _force) override {
		const bool isNativeRes = config.frameBufferEmulation.nativeResFactor == 1 && config.video.multisampling == 0;
		const bool isTexRect = dwnd().getDrawer().getDrawingState() == DrawingState::TexRect;
		const bool useTexCoordBounds = gDP.m_texCoordBounds.valid && !isNativeRes;
		/* At rasterization stage, the N64 places samples on the top left of the fragment while OpenGL		*/
		/* places them in the fragment center. As a result, a normal approach results in shifted texture	*/
		/* coordinates. In native resolution, this difference can be negated by shifting vertices by 0.5.	*/
		/* In higher resolutions, there	are more samples than the game intends, so shifting is not very		*/
		/* effective. Still, an heuristic is applied to render texture rectangles as correctly as possible  */
		/* in higher resolutions too. See issue #2324 for details. 											*/
		const float vertexOffset = isNativeRes ? 0.5f : 0.0f;
		float texCoordOffset[2] = { 0.0f, 0.0f };
		if (isTexRect && !isNativeRes) {
			float scale[2] = { 0.0f, 0.0f };
			if (config.graphics2D.enableNativeResTexrects != 0 && gDP.otherMode.textureFilter != G_TF_POINT) {
				scale[0] = scale[1] = 1.0f;
			} else {
				scale[0] = scale[1] = static_cast<float>(config.frameBufferEmulation.nativeResFactor);
			}

			if (config.frameBufferEmulation.nativeResFactor != 0) {
				if (gDP.otherMode.textureFilter != G_TF_POINT && gDP.otherMode.cycleType != G_CYC_COPY) {
					texCoordOffset[0] = -0.5f * gDP.lastTexRectInfo.dsdx;
					texCoordOffset[1] = -0.5f * gDP.lastTexRectInfo.dtdy;
				} else {
					texCoordOffset[0] = (gDP.lastTexRectInfo.dsdx >= 0.0f ? -0.5f / scale[0] : -1.0f + 0.5f / scale[0]) * gDP.lastTexRectInfo.dsdx;
					texCoordOffset[1] = (gDP.lastTexRectInfo.dtdy >= 0.0f ? -0.5f / scale[1] : -1.0f + 0.5f / scale[1]) * gDP.lastTexRectInfo.dtdy;
				}
			} else {
				texCoordOffset[0] = (gDP.lastTexRectInfo.dsdx >= 0.0f ? 0.0f : -1.0f) * gDP.lastTexRectInfo.dsdx;
				texCoordOffset[1] = (gDP.lastTexRectInfo.dtdy >= 0.0f ? 0.0f : -1.0f) * gDP.lastTexRectInfo.dtdy;
				if (gDP.otherMode.textureFilter != G_TF_POINT && gDP.otherMode.cycleType != G_CYC_COPY) {
					texCoordOffset[0] -= 0.5f;
					texCoordOffset[1] -= 0.5f;
				}
			}
		}
		/* Hack for framebuffer textures. See #519 and #2112 */
		if ((config.generalEmulation.hacks & hack_fbTextureOffset) != 0) {
			const CachedTexture* _pTexture = textureCache().current[0];
			if (_pTexture != nullptr) {
				if (gDP.otherMode.textureFilter != G_TF_POINT && _pTexture->frameBufferTexture != CachedTexture::fbNone) {
					texCoordOffset[0] -= 1.0f;
					texCoordOffset[1] -= 1.0f;
				}
			}
		}
		float tcbounds[4] = {};
		if (useTexCoordBounds) {
			tcbounds[0] = gDP.m_texCoordBounds.uls;
			tcbounds[1] = gDP.m_texCoordBounds.ult;
			tcbounds[2] = gDP.m_texCoordBounds.lrs;
			tcbounds[3] = gDP.m_texCoordBounds.lrt;
		}

		uVertexOffset.set(vertexOffset, vertexOffset, _force);
		uTexCoordOffset.set(texCoordOffset[0], texCoordOffset[1], _force);
		uUseTexCoordBounds.set(useTexCoordBounds ? 1 : 0, _force);
		uTexCoordBounds.set(tcbounds, _force);
		gDP.m_texCoordBounds.valid = false;
	}

private:
	fv2Uniform uVertexOffset;
	fv2Uniform uTexCoordOffset;
	iUniform uUseTexCoordBounds;
	fv4Uniform uTexCoordBounds;
};

class UMipmap : public UniformGroup
{
public:
	UMipmap(GLuint _program) {
		LocateUniform(uMinLod);
		LocateUniform(uMaxTile);
		LocateUniform(uEnableLod);
		LocateUniform(uNoAtlasTex);
		LocateUniform(uTextureDetail);
	}

	void update(bool _force) override
	{
		uMinLod.set(gDP.primColor.m, _force);
		uEnableLod.set(gDP.otherMode.textureLOD == G_TL_LOD ? 1 : 0, _force);
		uTextureDetail.set(gDP.otherMode.textureDetail, _force);

		u32 maxTile = gSP.texture.level;
		const CachedTexture * _pTexture = textureCache().current[1];
		if (_pTexture != nullptr && _pTexture->max_level == 0)
			maxTile = std::min(gSP.texture.level, 1u); // Hack for HD textures
		uMaxTile.set(maxTile, _force);

		bool bNoAtlasTex = (_pTexture != nullptr && _pTexture->bHDTexture) ||
							maxTile == 0 ||
							gDP.otherMode.textureLOD != G_TL_LOD ||
							(gDP.otherMode.textureDetail != G_TD_DETAIL && maxTile == 1);
		uNoAtlasTex.set(bNoAtlasTex ? 1 : 0, _force);
	}

private:
	fUniform uMinLod;
	iUniform uMaxTile;
	iUniform uEnableLod;
	iUniform uNoAtlasTex;
	iUniform uTextureDetail;
};

class UTextureSize : public UniformGroup
{
public:
	UTextureSize(GLuint _program, bool _useT0, bool _useT1)
	: m_useT0(_useT0)
	, m_useT1(_useT1)
	{
		LocateUniform(uTextureSize[0]);
		LocateUniform(uTextureSize[1]);
	}

	void update(bool _force) override
	{
		TextureCache & cache = textureCache();
		if (m_useT0 && cache.current[0] != nullptr)
			uTextureSize[0].set(static_cast<float>(cache.current[0]->width),
				static_cast<float>(cache.current[0]->height), _force);
		if (m_useT1 && cache.current[1] != nullptr) {
			CachedTexture * pTexture = cache.current[1];
			if (pTexture->max_level == 0)
				uTextureSize[1].set(static_cast<float>(pTexture->width),
					static_cast<float>(pTexture->height), _force);
			else
				uTextureSize[1].set(static_cast<float>(pTexture->mipmapAtlasWidth),
					static_cast<float>(pTexture->mipmapAtlasHeight), _force);
		}
	}

private:
	fv2Uniform uTextureSize[2];
	bool m_useT0;
	bool m_useT1;
};

class UTextureParams : public UniformGroup
{
public:
	UTextureParams(GLuint _program, bool _useT0, bool _useT1)
	{
		m_useTile[0] = _useT0;
		m_useTile[1] = _useT1;
		LocateUniform(uTexScale);
		LocateUniform(uCacheFrameBuffer);
	}

	void update(bool _force) override
	{
		int nFB[2] = { 0, 0 };
		TextureCache & cache = textureCache();
		for (u32 t = 0; t < 2; ++t) {
			if (!m_useTile[t])
				continue;
			CachedTexture *_pTexture = cache.current[t];
			if (_pTexture != nullptr) {
				nFB[t] = _pTexture->frameBufferTexture;
			}
		}

		uCacheFrameBuffer.set(nFB[0], nFB[1], _force);
		uTexScale.set(gSP.texture.scales, gSP.texture.scalet, _force);
	}

private:
	bool m_useTile[2];
	fv2Uniform uTexScale;
	iv2Uniform uCacheFrameBuffer;
};

class UTextureEngine : public UniformGroup
{
public:
	UTextureEngine(GLuint _program, bool _useT0, bool _useT1)
	{
		m_useTile[0] = _useT0;
		m_useTile[1] = _useT1;
		LocateUniform(uTexWrap[0]);
		LocateUniform(uTexWrap[1]);
		LocateUniform(uTexClamp[0]);
		LocateUniform(uTexClamp[1]);
		LocateUniform(uTexWrapEn[0]);
		LocateUniform(uTexWrapEn[1]);
		LocateUniform(uTexClampEn[0]);
		LocateUniform(uTexClampEn[1]);
		LocateUniform(uTexMirrorEn[0]);
		LocateUniform(uTexMirrorEn[1]);
		LocateUniform(uTexSize[0]);
		LocateUniform(uTexSize[1]);
		LocateUniform(uShiftScale[0]);
		LocateUniform(uShiftScale[1]);
		LocateUniform(uTexOffset[0]);
		LocateUniform(uTexOffset[1]);
		LocateUniform(uHDRatio[0]);
		LocateUniform(uHDRatio[1]);
		LocateUniform(uCacheOffset[0]);
		LocateUniform(uCacheOffset[1]);
		LocateUniform(uBilinearOffset);
	}

	void update(bool _force) override
	{
		std::array<f32, 2> aTexWrap[2] = { { 1024.0f, 1024.0f }, { 1024.0f, 1024.0f } };
		std::array<f32, 2> aTexClamp[2] = { { 1024.0f, 1024.0f }, { 1024.0f, 1024.0f } };
		std::array<f32, 2> aTexWrapEn[2] = { { 0.0f, 0.0f }, { 0.0f, 0.0f } };
		std::array<f32, 2> aTexClampEn[2] = { { 0.0f, 0.0f }, { 0.0f, 0.0f } };
		std::array<f32, 2> aTexMirrorEn[2] = { { 0.0f, 0.0f }, { 0.0f,0.0f }};
		std::array<f32, 2> aTexSize[2] = { { 1024.0f, 1024.0f }, { 1024.0f, 1024.0f } };

		std::array<f32, 2> aShiftScale[2] = { { 1.0f, 1.0f }, { 1.0f,1.0f } };
		std::array<f32, 2> aTexOffset[2] = { { 0.0f, 0.0f }, { 0.0f, 0.0f } };

		std::array<f32, 2> aHDRatio[2] = { { 1.0f, 1.0f }, { 1.0f, 1.0f } };
		std::array<f32, 2> aCacheOffset[2] = { { 0.0f, 0.0f }, { 0.0f, 0.0f } };

		const float bilinearOffset = gDP.otherMode.textureFilter != G_TF_POINT && gDP.otherMode.cycleType != G_CYC_COPY ? 0.5f : 0.0f;
		uBilinearOffset.set(bilinearOffset, bilinearOffset, _force);

		TextureCache & cache = textureCache();
		for (u32 t = 0; t < 2; ++t) {
			if (!m_useTile[t])
				continue;

			gDPTile * pTile = gSP.textureTile[t];
			CachedTexture * pTexture = cache.current[t];
			if (pTile == nullptr || pTexture == nullptr)
				continue;

			aTexSize[t][0] = pTexture->width * pTexture->hdRatioS;
			aTexSize[t][1] = pTexture->height * pTexture->hdRatioT;


			if (pTile->textureMode != TEXTUREMODE_BGIMAGE && pTile->textureMode != TEXTUREMODE_FRAMEBUFFER_BG) {
				float fuls = pTile->fuls;
				float fult = pTile->fult;
				if (pTile->frameBufferAddress > 0u) {
					FrameBuffer * pBuffer = frameBufferList().getBuffer(pTile->frameBufferAddress);
					if (pBuffer != nullptr) {
						if (pTile->masks > 0 && pTile->clamps == 0u)
							fuls = float(pTile->uls % (1 << pTile->masks));
						if (pTile->maskt > 0 && pTile->clampt == 0u)
							fult = float(pTile->ult % (1 << pTile->maskt));
					} else {
						pTile->frameBufferAddress = 0u;
					}
				}
				aTexOffset[t][0] = fuls;
				aTexOffset[t][1] = fult;

				aShiftScale[t][0] = calcShiftScaleS(*pTile);
				aShiftScale[t][1] = calcShiftScaleT(*pTile);
			}

			aHDRatio[t][0] = pTexture->hdRatioS;
			aHDRatio[t][1] = pTexture->hdRatioT;

			aCacheOffset[t][0] = pTexture->offsetS * pTexture->hdRatioS;
			aCacheOffset[t][1] = pTexture->offsetT * pTexture->hdRatioT;

			/* Not sure if special treatment of framebuffer textures is correct */
			if (pTexture->frameBufferTexture != CachedTexture::fbNone)
			{
				aTexClamp[t][0] = f32(pTexture->width) * pTexture->hdRatioS - 1.0f;
				aTexClamp[t][1] = f32(pTexture->height) * pTexture->hdRatioT - 1.0f;
				aTexWrapEn[t][0] = 0.0;
				aTexWrapEn[t][1] = 0.0;
				aTexClampEn[t][0] = 1.0;
				aTexClampEn[t][1] = 1.0;
				aTexMirrorEn[t][0] = 0.0;
				aTexMirrorEn[t][1] = 0.0;
			} else if (pTile->textureMode != TEXTUREMODE_NORMAL || g_debugger.isDebugMode()) {
				aTexWrapEn[t][0] = 0.0;
				aTexWrapEn[t][1] = 0.0;
				aTexClampEn[t][0] = 0.0;
				aTexClampEn[t][1] = 0.0;
				aTexMirrorEn[t][0] = 0.0;
				aTexMirrorEn[t][1] = 0.0;
			} else {
				aTexWrap[t][0] = f32(1 << pTile->masks) * pTexture->hdRatioS;
				aTexWrap[t][1] = f32(1 << pTile->maskt) * pTexture->hdRatioT;
				aTexClamp[t][0] = f32(pTile->lrs - pTile->uls + 1) * pTexture->hdRatioS - 1.0f;
				aTexClamp[t][1] = f32(pTile->lrt - pTile->ult + 1) * pTexture->hdRatioT - 1.0f;
				aTexWrapEn[t][0] = f32(pTile->masks == 0 ? 0 : 1);
				aTexWrapEn[t][1] = f32(pTile->maskt == 0 ? 0 : 1);
				aTexClampEn[t][0] = f32(gDP.otherMode.cycleType == G_CYC_COPY ? 0 : (pTile->masks == 0 ? 1 : pTile->clamps));
				aTexClampEn[t][1] = f32(gDP.otherMode.cycleType == G_CYC_COPY ? 0 : (pTile->maskt == 0 ? 1 : pTile->clampt));
				aTexMirrorEn[t][0] = f32(pTile->masks == 0 ? 0 : pTile->mirrors);
				aTexMirrorEn[t][1] = f32(pTile->maskt == 0 ? 0 : pTile->mirrort);
			}

			uTexWrap[t].set(aTexWrap[t][0], aTexWrap[t][1], _force);
			uTexClamp[t].set(aTexClamp[t][0], aTexClamp[t][1], _force);
			uTexWrapEn[t].set(aTexWrapEn[t][0], aTexWrapEn[t][1], _force);
			uTexWrapEn[t].set(aTexWrapEn[t][0], aTexWrapEn[t][1], _force);
			uTexClampEn[t].set(aTexClampEn[t][0], aTexClampEn[t][1], _force);
			uTexMirrorEn[t].set(aTexMirrorEn[t][0], aTexMirrorEn[t][1], _force);
			uTexSize[t].set(aTexSize[t][0], aTexSize[t][1], _force);
			uShiftScale[t].set(aShiftScale[t][0], aShiftScale[t][1], _force);
			uTexOffset[t].set(aTexOffset[t][0], aTexOffset[t][1], _force);
			uHDRatio[t].set(aHDRatio[t][0], aHDRatio[t][1], _force);
			uCacheOffset[t].set(aCacheOffset[t][0], aCacheOffset[t][1], _force);
		}

	}

private:
	bool m_useTile[2];
	fv2Uniform uTexWrap[2];
	fv2Uniform uTexClamp[2];
	fv2Uniform uTexWrapEn[2];
	fv2Uniform uTexClampEn[2];
	fv2Uniform uTexMirrorEn[2];
	fv2Uniform uTexSize[2];
	fv2Uniform uShiftScale[2];
	fv2Uniform uTexOffset[2];
	fv2Uniform uHDRatio[2];
	fv2Uniform uCacheOffset[2];
	fv2Uniform uBilinearOffset;
};

} // nameless namespace

/*---------------CombinerProgramUniformFactoryCommon-------------*/
namespace glsl {

void CombinerProgramUniformFactoryAccurate::_addRasterInfo(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new URasterInfo(_program));
}

void CombinerProgramUniformFactoryAccurate::_addMipmap(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UMipmap(_program));
}

void CombinerProgramUniformFactoryAccurate::_addMipmap2(GLuint _program, UniformGroups &_uniforms) const
{
}

void CombinerProgramUniformFactoryAccurate::_addTextureSize(GLuint _program, UniformGroups &_uniforms, bool _usesTile0, bool _usesTile1) const
{
	_uniforms.emplace_back(new UTextureSize(_program, _usesTile0, _usesTile1));
}

void CombinerProgramUniformFactoryAccurate::_addTextureParams(GLuint _program, UniformGroups &_uniforms, bool _usesTile0, bool _usesTile1) const
{
	_uniforms.emplace_back(new UTextureParams(_program, _usesTile0, _usesTile1));
}

void CombinerProgramUniformFactoryAccurate::_addClampWrapMirrorEngine(GLuint _program, UniformGroups &_uniforms, bool _usesTile0, bool _usesTile1) const
{
	_uniforms.emplace_back(new UTextureEngine(_program, _usesTile0, _usesTile1));
}

CombinerProgramUniformFactoryAccurate::CombinerProgramUniformFactoryAccurate(const opengl::GLInfo & _glInfo)
		: CombinerProgramUniformFactoryCommon(_glInfo)
{
}

}
