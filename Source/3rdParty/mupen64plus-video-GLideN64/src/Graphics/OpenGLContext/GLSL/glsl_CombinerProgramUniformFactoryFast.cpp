#include "glsl_CombinerProgramUniformFactoryFast.h"

#include <Config.h>
#include <Textures.h>
#include <DisplayWindow.h>
#include <Debugger.h>
#include <FrameBuffer.h>

#include <cmath>

namespace {
using namespace glsl;

class URasterInfoFast : public UniformGroup {
public:
	URasterInfoFast(GLuint _program) {
		LocateUniform(uVertexOffset);
		LocateUniform(uTexCoordOffset[0]);
		LocateUniform(uTexCoordOffset[1]);
		LocateUniform(uUseTexCoordBounds);
		LocateUniform(uTexCoordBounds0);
		LocateUniform(uTexCoordBounds1);
	}

	void update(bool _force) override {
		const bool isNativeRes = config.frameBufferEmulation.nativeResFactor == 1 && config.video.multisampling == 0;
		const bool isTexRect = dwnd().getDrawer().getDrawingState() == DrawingState::TexRect;
		const bool useTexCoordBounds = isTexRect && !isNativeRes && config.graphics2D.enableTexCoordBounds;
		float texCoordOffset[2][2] = { 0.0f, 0.0f };
		if (isTexRect && !isNativeRes) {
			float scale[2] = { 0.0f, 0.0f };
			if (config.graphics2D.enableNativeResTexrects != 0 && gDP.otherMode.textureFilter != G_TF_POINT) {
				scale[0] = scale[1] = 1.0f;
			} else {
				scale[0] = scale[1] = static_cast<float>(config.frameBufferEmulation.nativeResFactor);
			}

			for (int t = 0; t < 2; t++) {
				const CachedTexture* _pTexture = textureCache().current[t];
				if (_pTexture != nullptr) {
					if (config.frameBufferEmulation.nativeResFactor != 0) {
						texCoordOffset[t][0] = (gDP.lastTexRectInfo.dsdx >= 0.0f ? -0.5f / scale[0] : -1.0f + 0.5f / scale[0]) * gDP.lastTexRectInfo.dsdx * _pTexture->hdRatioS;
						texCoordOffset[t][1] = (gDP.lastTexRectInfo.dtdy >= 0.0f ? -0.5f / scale[1] : -1.0f + 0.5f / scale[1]) * gDP.lastTexRectInfo.dtdy * _pTexture->hdRatioT;
					} else {
						texCoordOffset[t][0] = (gDP.lastTexRectInfo.dsdx >= 0.0f ? 0.0f : -1.0f) * gDP.lastTexRectInfo.dsdx * _pTexture->hdRatioS;
						texCoordOffset[t][1] = (gDP.lastTexRectInfo.dtdy >= 0.0f ? 0.0f : -1.0f) * gDP.lastTexRectInfo.dtdy * _pTexture->hdRatioT;
						if (gDP.otherMode.textureFilter != G_TF_POINT && gDP.otherMode.cycleType != G_CYC_COPY) {
							texCoordOffset[t][0] -= 0.5f;
							texCoordOffset[t][1] -= 0.5f;
						}
					}
				}
			}
		}
		/* Hack for framebuffer textures. See #519 and #2112 */
		if ((config.generalEmulation.hacks & hack_fbTextureOffset) != 0) {
			for (int t = 0; t < 2; t++) {
				const CachedTexture* _pTexture = textureCache().current[t];
				if (_pTexture != nullptr) {
					if (gDP.otherMode.textureFilter != G_TF_POINT && _pTexture->frameBufferTexture != CachedTexture::fbNone) {
						texCoordOffset[t][0] -= 1.0f;
						texCoordOffset[t][1] -= 1.0f;
					}
				}
			}
		}
		float tcbounds[2][4] = {};
		if (useTexCoordBounds) {
			f32 uls, lrs, ult, lrt, S, T, shiftScaleS, shiftScaleT;
			for (int t = 0; t < 2; t++) {
				const CachedTexture * _pTexture = textureCache().current[t];
				const gDPTile * _pTile = gSP.textureTile[t];
				if (_pTexture != nullptr && _pTile != nullptr) {
					s16 shiftedS = gDP.lastTexRectInfo.s;
					shiftScaleS = calcShiftScaleS(*_pTile, &shiftedS);
					S = _FIXED2FLOAT(shiftedS, 5);

					s16 shiftedT = gDP.lastTexRectInfo.t;
					shiftScaleT = calcShiftScaleT(*_pTile, &shiftedT);
					T = _FIXED2FLOAT(shiftedT, 5);

					uls = S + (ceilf(gDP.lastTexRectInfo.ulx) - gDP.lastTexRectInfo.ulx) * gDP.lastTexRectInfo.dsdx * shiftScaleS;
					lrs = S + (ceilf(gDP.lastTexRectInfo.lrx) - gDP.lastTexRectInfo.ulx - 1.0f) * gDP.lastTexRectInfo.dsdx * shiftScaleS;
					ult = T + (ceilf(gDP.lastTexRectInfo.uly) - gDP.lastTexRectInfo.uly) * gDP.lastTexRectInfo.dtdy * shiftScaleT;
					lrt = T + (ceilf(gDP.lastTexRectInfo.lry) - gDP.lastTexRectInfo.uly - 1.0f) * gDP.lastTexRectInfo.dtdy * shiftScaleT;

					tcbounds[t][0] = (fmin(uls, lrs) - _pTile->fuls) * _pTexture->hdRatioS;
					tcbounds[t][1] = (fmin(ult, lrt) - _pTile->fult) * _pTexture->hdRatioT;
					tcbounds[t][2] = (fmax(uls, lrs) - _pTile->fuls) * _pTexture->hdRatioS;
					tcbounds[t][3] = (fmax(ult, lrt) - _pTile->fult) * _pTexture->hdRatioT;
					if (_pTexture->frameBufferTexture != CachedTexture::fbNone) {
						tcbounds[t][0] += _pTexture->offsetS * _pTexture->hdRatioS;
						tcbounds[t][1] += _pTexture->offsetT * _pTexture->hdRatioT;
						tcbounds[t][2] += _pTexture->offsetS * _pTexture->hdRatioS;
						tcbounds[t][3] += _pTexture->offsetT * _pTexture->hdRatioT;
					}
				}
			}
		}

		uVertexOffset.set(0.0, 0.0, _force);
		uTexCoordOffset[0].set(texCoordOffset[0][0], texCoordOffset[0][1], _force);
		uTexCoordOffset[1].set(texCoordOffset[1][0], texCoordOffset[1][1], _force);
		uUseTexCoordBounds.set(useTexCoordBounds ? 1 : 0, _force);
		uTexCoordBounds0.set(tcbounds[0], _force);
		uTexCoordBounds1.set(tcbounds[1], _force);
	}

private:
	fv2Uniform uVertexOffset;
	fv2Uniform uTexCoordOffset[2];
	iUniform uUseTexCoordBounds;
	fv4Uniform uTexCoordBounds0;
	fv4Uniform uTexCoordBounds1;
};

class UMipmap1 : public UniformGroup
{
public:
	UMipmap1(GLuint _program) {
		LocateUniform(uMinLod);
		LocateUniform(uMaxTile);
	}

	void update(bool _force) override
	{
		uMinLod.set(gDP.primColor.m, _force);
		uMaxTile.set(gSP.texture.level, _force);
	}

private:
	fUniform uMinLod;
	iUniform uMaxTile;
};

class UMipmap2 : public UniformGroup
{
public:
	UMipmap2(GLuint _program) {
		LocateUniform(uEnableLod);
		LocateUniform(uTextureDetail);
	}

	void update(bool _force) override
	{
		const int uCalcLOD = (gDP.otherMode.textureLOD == G_TL_LOD) ? 1 : 0;
		uEnableLod.set(uCalcLOD, _force);
		uTextureDetail.set(gDP.otherMode.textureDetail, _force);
	}

private:
	iUniform uEnableLod;
	iUniform uTextureDetail;
};

class UTextureSizeFast : public UniformGroup
{
public:
	UTextureSizeFast(GLuint _program, bool _useT0, bool _useT1)
	: m_useT0(_useT0)
	, m_useT1(_useT1)
	{
		LocateUniform(uTextureSize[0]);
		LocateUniform(uTextureSize[1]);
	}

	void update(bool _force) override
	{
		TextureCache & cache = textureCache();
		if (m_useT0 && cache.current[0] != NULL)
			uTextureSize[0].set((float)cache.current[0]->width, (float)cache.current[0]->height, _force);
		if (m_useT1 && cache.current[1] != NULL)
			uTextureSize[1].set((float)cache.current[1]->width, (float)cache.current[1]->height, _force);
	}

private:
	fv2Uniform uTextureSize[2];
	bool m_useT0;
	bool m_useT1;
};

class UTextureParamsFast : public UniformGroup
{
public:
	UTextureParamsFast(GLuint _program, bool _useT0, bool _useT1)
	{
		m_useTile[0] = _useT0;
		m_useTile[1] = _useT1;
		LocateUniform(uTexOffset[0]);
		LocateUniform(uTexOffset[1]);
		LocateUniform(uCacheShiftScale[0]);
		LocateUniform(uCacheShiftScale[1]);
		LocateUniform(uCacheScale[0]);
		LocateUniform(uCacheScale[1]);
		LocateUniform(uCacheOffset[0]);
		LocateUniform(uCacheOffset[1]);
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

			gDPTile * pTile = gSP.textureTile[t];
			if (pTile != nullptr) {
				if (pTile->textureMode == TEXTUREMODE_BGIMAGE || pTile->textureMode == TEXTUREMODE_FRAMEBUFFER_BG)
					uTexOffset[t].set(0.0f, 0.0f, _force);
				else {
					float fuls = pTile->fuls;
					float fult = pTile->fult;
					if (pTile->frameBufferAddress > 0) {
						FrameBuffer * pBuffer = frameBufferList().getBuffer(pTile->frameBufferAddress);
						if (pBuffer != nullptr) {
							if (pTile->masks > 0 && pTile->clamps == 0)
								fuls = float(pTile->uls % (1 << pTile->masks));
							if (pTile->maskt > 0 && pTile->clampt == 0)
								fult = float(pTile->ult % (1 << pTile->maskt));
						} else {
							pTile->frameBufferAddress = 0;
						}
					}
					uTexOffset[t].set(fuls, fult, _force);
				}
			}

			CachedTexture *_pTexture = cache.current[t];
			if (_pTexture != nullptr) {
				f32 shiftScaleS = 1.0f;
				f32 shiftScaleT = 1.0f;
				getTextureShiftScale(t, cache, shiftScaleS, shiftScaleT);
				uCacheShiftScale[t].set(shiftScaleS, shiftScaleT, _force);
				uCacheScale[t].set(_pTexture->scaleS, _pTexture->scaleT, _force);
				uCacheOffset[t].set(_pTexture->offsetS, _pTexture->offsetT, _force);
				nFB[t] = _pTexture->frameBufferTexture;
			}
		}

		uCacheFrameBuffer.set(nFB[0], nFB[1], _force);
		uTexScale.set(gSP.texture.scales, gSP.texture.scalet, _force);
	}

private:
	bool m_useTile[2];
	fv2Uniform uTexOffset[2];
	fv2Uniform uCacheShiftScale[2];
	fv2Uniform uCacheScale[2];
	fv2Uniform uCacheOffset[2];
	fv2Uniform uTexScale;
	iv2Uniform uCacheFrameBuffer;
};

class UClampWrapMirrorTex : public UniformGroup
{
public:
	UClampWrapMirrorTex(GLuint _program, bool _useT0, bool _useT1)
	{
		m_useTile[0] = _useT0;
		m_useTile[1] = _useT1;
		LocateUniform(uTexClamp0);
		LocateUniform(uTexClamp1);
		LocateUniform(uTexWrap0);
		LocateUniform(uTexWrap1);
		LocateUniform(uTexMirror0);
		LocateUniform(uTexMirror1);
		LocateUniform(uTexScale0);
		LocateUniform(uTexScale1);
	}

	void update(bool _force) override
	{
		std::array<f32, 4> aTexClamp[2] = { { -10000.0f, -10000.0f, 10000.0f, 10000.0f },
											{ -10000.0f, -10000.0f, 10000.0f, 10000.0f } };
		std::array<f32, 2> aTexWrap[2] = { { 10000.0f, 10000.0f }, { 10000.0f, 10000.0f } };
		std::array<f32, 2> aTexMirror[2] = { { 0.0f, 0.0f}, { 0.0f, 0.0f } };
		std::array<f32, 2> aTexScale[2] = { { 1.0f, 1.0f },{ 1.0f, 1.0f } };
		TextureCache & cache = textureCache();
		const bool replaceTex1ByTex0 = needReplaceTex1ByTex0();
		for (u32 t = 0; t < 2; ++t) {
			if (!m_useTile[t])
				continue;

			const u32 tile = replaceTex1ByTex0 ? 0 : t;
			const gDPTile * pTile = gSP.textureTile[tile];
			CachedTexture * pTexture = cache.current[tile];
			if (pTile == nullptr || pTexture == nullptr)
				continue;

			if (gDP.otherMode.cycleType != G_CYC_COPY) {
				if (pTexture->clampS) {
					aTexClamp[t][0] = 0.0f; // S lower bound
					if (pTexture->frameBufferTexture != CachedTexture::fbNone ||
						pTile->textureMode == TEXTUREMODE_BGIMAGE)
						aTexClamp[t][2] = 1.0f;
					else {
						u32 tileWidth = ((pTile->lrs - pTile->uls) & 0x03FF) + 1;
						if (pTile->size > pTexture->size)
							tileWidth <<= pTile->size - pTexture->size;
						//	aTexClamp[t][2] = f32(tileWidth) / (pTexture->mirrorS ? f32(pTexture->width) : f32(pTexture->clampWidth)); // S upper bound
						aTexClamp[t][2] = f32(tileWidth) / f32(pTexture->width); // S upper bound
					}
				}
				if (pTexture->clampT) {
					aTexClamp[t][1] = 0.0f; // T lower bound
					if (pTexture->frameBufferTexture != CachedTexture::fbNone ||
						pTile->textureMode == TEXTUREMODE_BGIMAGE)
						aTexClamp[t][3] = 1.0f;
					else {
						const u32 tileHeight = ((pTile->lrt - pTile->ult) & 0x03FF) + 1;
						//	aTexClamp[t][3] = f32(tileHeight) / (pTexture->mirrorT ? f32(pTexture->height) : f32(pTexture->clampHeight)); // T upper bound
						aTexClamp[t][3] = f32(tileHeight) / f32(pTexture->height); // T upper bound
					}
				}
			}
			if (pTexture->maskS) {
				const f32 wrapWidth = static_cast<f32>(1 << pTile->originalMaskS);
				const f32 pow2Width = static_cast<f32>(pow2(pTexture->width));
				aTexWrap[t][0] = wrapWidth / pow2Width;
				aTexScale[t][0] = pow2Width / f32(pTexture->width);
			}
			if (pTexture->maskT) {
				const f32 wrapHeight = static_cast<f32>(1 << pTile->originalMaskT);
				const f32 pow2Height = static_cast<f32>(pow2(pTexture->height));
				aTexWrap[t][1] = wrapHeight / pow2Height;
				aTexScale[t][1] = pow2Height / f32(pTexture->height);
			}
			if (pTexture->mirrorS) {
				aTexMirror[t][0] = 1.0f;
				aTexWrap[t][0] *= 2.0f;
			}
			if (pTexture->mirrorT) {
				aTexMirror[t][1] = 1.0f;
				aTexWrap[t][1] *= 2.0f;
			}
		}

		uTexClamp0.set(aTexClamp[0].data(), _force);
		uTexClamp1.set(aTexClamp[1].data(), _force);
		uTexWrap0.set(aTexWrap[0][0], aTexWrap[0][1], _force);
		uTexWrap1.set(aTexWrap[1][0], aTexWrap[1][1], _force);
		uTexMirror0.set(aTexMirror[0][0], aTexMirror[0][1], _force);
		uTexMirror1.set(aTexMirror[1][0], aTexMirror[1][1], _force);
		uTexScale0.set(aTexScale[0][0], aTexScale[0][1], _force);
		uTexScale1.set(aTexScale[1][0], aTexScale[1][1], _force);
	}

private:
	bool m_useTile[2];
	fv4Uniform uTexClamp0;
	fv4Uniform uTexClamp1;
	fv2Uniform uTexWrap0;
	fv2Uniform uTexWrap1;
	fv2Uniform uTexMirror0;
	fv2Uniform uTexMirror1;
	fv2Uniform uTexScale0;
	fv2Uniform uTexScale1;
};

} //nameless namespace

/*---------------CombinerProgramUniformFactoryCommon-------------*/
namespace glsl {

void CombinerProgramUniformFactoryFast::_addRasterInfo(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new URasterInfoFast(_program));
}

void CombinerProgramUniformFactoryFast::_addMipmap(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UMipmap1(_program));
}

void CombinerProgramUniformFactoryFast::_addMipmap2(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UMipmap2(_program));
}

void CombinerProgramUniformFactoryFast::_addTextureSize(GLuint _program, UniformGroups &_uniforms, bool _usesTile0, bool _usesTile1) const
{
	_uniforms.emplace_back(new UTextureSizeFast(_program, _usesTile0, _usesTile1));
}

void CombinerProgramUniformFactoryFast::_addTextureParams(GLuint _program, UniformGroups &_uniforms, bool _usesTile0, bool _usesTile1) const
{
	_uniforms.emplace_back(new UTextureParamsFast(_program, _usesTile0, _usesTile1));
}

void CombinerProgramUniformFactoryFast::_addClampWrapMirrorEngine(GLuint _program, UniformGroups &_uniforms, bool _usesTile0, bool _usesTile1) const
{
	_uniforms.emplace_back(new UClampWrapMirrorTex(_program, _usesTile0, _usesTile1));
}

CombinerProgramUniformFactoryFast::CombinerProgramUniformFactoryFast(const opengl::GLInfo & _glInfo)
: CombinerProgramUniformFactoryCommon(_glInfo)
{
}

}
