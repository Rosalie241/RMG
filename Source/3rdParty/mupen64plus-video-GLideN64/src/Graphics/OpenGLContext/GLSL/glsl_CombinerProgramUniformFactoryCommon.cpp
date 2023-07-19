#include "glsl_CombinerProgramUniformFactoryCommon.h"

#include <Config.h>
#include <Graphics/Parameters.h>
#include <Graphics/Context.h>

#include <Textures.h>
#include <NoiseTexture.h>
#include <FrameBuffer.h>
#include <DisplayWindow.h>
#include <RSP.h>
#include <VI.h>

namespace {
using namespace glsl;
/*---------------UniformGroup-------------*/

class UNoiseTex : public UniformGroup
{
public:
	UNoiseTex(GLuint _program) {
		LocateUniform(uTexNoise);
	}

	void update(bool _force) override
	{
		uTexNoise.set(int(graphics::textureIndices::NoiseTex), _force);
	}

private:
	iUniform uTexNoise;
};

class UDepthTex : public UniformGroup
{
public:
	UDepthTex(GLuint _program) {
		LocateUniform(uDepthTex);
	}

	void update(bool _force) override
	{
		uDepthTex.set(int(graphics::textureIndices::DepthTex), _force);
	}

private:
	iUniform uDepthTex;
};

class UZLutTexture : public UniformGroup
{
public:
	UZLutTexture(GLuint _program) {
		LocateUniform(uZlutImage);
	}

	void update(bool _force) override
	{
		uZlutImage.set(int(graphics::textureIndices::ZLUTTex), _force);
	}

private:
	iUniform uZlutImage;
};

class UTextures : public UniformGroup
{
public:
	UTextures(GLuint _program) {
		LocateUniform(uTex0);
		LocateUniform(uTex1);
	}

	void update(bool _force) override
	{
		uTex0.set(0, _force);
		uTex1.set(1, _force);
	}

private:
	iUniform uTex0;
	iUniform uTex1;
};

class UMSAATextures : public UniformGroup
{
public:
	UMSAATextures(GLuint _program) {
		LocateUniform(uMSTex0);
		LocateUniform(uMSTex1);
		LocateUniform(uMSAASamples);
	}

	void update(bool _force) override
	{
		uMSTex0.set(int(graphics::textureIndices::MSTex[0]), _force);
		uMSTex1.set(int(graphics::textureIndices::MSTex[1]), _force);
		uMSAASamples.set(config.video.multisampling, _force);
	}

private:
	iUniform uMSTex0;
	iUniform uMSTex1;
	iUniform uMSAASamples;
};

class UScreenSpaceTriangleInfo : public UniformGroup
{
public:
	UScreenSpaceTriangleInfo(GLuint _program) {
		LocateUniform(uScreenSpaceTriangle);
	}

	void update(bool _force) override
	{
		uScreenSpaceTriangle.set(
			(dwnd().getDrawer().getDrawingState() == DrawingState::ScreenSpaceTriangle) ? 1 : 0, _force);
	}

private:
	iUniform uScreenSpaceTriangle;
};

class UFrameBufferInfo : public UniformGroup
{
public:
	UFrameBufferInfo(GLuint _program) {
		LocateUniform(uFbMonochrome);
		LocateUniform(uFbFixedAlpha);
		LocateUniform(uMSTexEnabled);
	}

	void update(bool _force) override
	{
		int nFbMonochromeMode0 = 0, nFbMonochromeMode1 = 0;
		int nFbFixedAlpha0 = 0, nFbFixedAlpha1 = 0;
		int nMSTex0Enabled = 0, nMSTex1Enabled = 0;
		TextureCache & cache = textureCache();
		if (cache.current[0] != nullptr && cache.current[0]->frameBufferTexture != CachedTexture::fbNone) {
			if (cache.current[0]->size == G_IM_SIZ_8b) {
				nFbMonochromeMode0 = 1;
				if (gDP.otherMode.imageRead == 0)
					nFbFixedAlpha0 = 1;
			} else if (gSP.textureTile[0]->size == G_IM_SIZ_16b && gSP.textureTile[0]->format == G_IM_FMT_IA) {
				nFbMonochromeMode0 = 2;
			} else if ((config.generalEmulation.hacks & hack_ZeldaMonochrome) != 0 &&
					   cache.current[0]->size == G_IM_SIZ_16b &&
					   gSP.textureTile[0]->size == G_IM_SIZ_8b &&
					   gSP.textureTile[0]->format == G_IM_FMT_CI) {
				// Zelda monochrome effect
				nFbMonochromeMode0 = 3;
				nFbMonochromeMode1 = 3;
			}

			nMSTex0Enabled = cache.current[0]->frameBufferTexture == CachedTexture::fbMultiSample ? 1 : 0;
		}
		if (cache.current[1] != nullptr && cache.current[1]->frameBufferTexture != CachedTexture::fbNone) {
			if (cache.current[1]->size == G_IM_SIZ_8b) {
				nFbMonochromeMode1 = 1;
				if (gDP.otherMode.imageRead == 0)
					nFbFixedAlpha1 = 1;
			}
			else if (gSP.textureTile[1]->size == G_IM_SIZ_16b && gSP.textureTile[1]->format == G_IM_FMT_IA)
				nFbMonochromeMode1 = 2;
			nMSTex1Enabled = cache.current[1]->frameBufferTexture == CachedTexture::fbMultiSample ? 1 : 0;
		}
		uFbMonochrome.set(nFbMonochromeMode0, nFbMonochromeMode1, _force);
		uFbFixedAlpha.set(nFbFixedAlpha0, nFbFixedAlpha1, _force);
		uMSTexEnabled.set(nMSTex0Enabled, nMSTex1Enabled, _force);
		gDP.changed &= ~CHANGED_FB_TEXTURE;
	}

private:
	iv2Uniform uFbMonochrome;
	iv2Uniform uFbFixedAlpha;
	iv2Uniform uMSTexEnabled;
};


class UFog : public UniformGroup
{
public:
	UFog(GLuint _program) {
		LocateUniform(uFogUsage);
		LocateUniform(uFogScale);
	}

	void update(bool _force) override
	{
		if (RSP.LLE) {
			uFogUsage.set(0, _force);
			return;
		}

		int nFogUsage = ((gSP.geometryMode & G_FOG) != 0) ? 1 : 0;
		if (GBI.getMicrocodeType() == F3DAM) {
			const s16 fogMode = ((gSP.geometryMode >> 13) & 9) + 0xFFF8;
			if (fogMode == 0)
				nFogUsage = 1;
			else if (fogMode > 0)
				nFogUsage = 2;
		}
		uFogUsage.set(nFogUsage, _force);
		uFogScale.set(gSP.fog.multiplierf, gSP.fog.offsetf, _force);
	}

private:
	iUniform uFogUsage;
	fv2Uniform uFogScale;
};

class UBlendMode1Cycle : public UniformGroup
{
public:
	UBlendMode1Cycle(GLuint _program) {
		LocateUniform(uBlendMux1);
		LocateUniform(uForceBlendCycle1);
	}

	void update(bool _force) override
	{
		uBlendMux1.set(gDP.otherMode.c1_m1a,
			gDP.otherMode.c1_m1b,
			gDP.otherMode.c1_m2a,
			gDP.otherMode.c1_m2b,
			_force);

		const int forceBlend1 = (int)gDP.otherMode.forceBlender;
		uForceBlendCycle1.set(forceBlend1, _force);
	}

private:
	i4Uniform uBlendMux1;
	iUniform uForceBlendCycle1;
};

class UBlendMode2Cycle : public UniformGroup
{
public:
	UBlendMode2Cycle(GLuint _program) {
		LocateUniform(uBlendMux1);
		LocateUniform(uBlendMux2);
		LocateUniform(uForceBlendCycle1);
		LocateUniform(uForceBlendCycle2);
	}

	void update(bool _force) override
	{
		uBlendMux1.set(gDP.otherMode.c1_m1a,
			gDP.otherMode.c1_m1b,
			gDP.otherMode.c1_m2a,
			gDP.otherMode.c1_m2b,
			_force);

		uBlendMux2.set(gDP.otherMode.c2_m1a,
			gDP.otherMode.c2_m1b,
			gDP.otherMode.c2_m2a,
			gDP.otherMode.c2_m2b,
			_force);

		const int forceBlend1 = 1;
		uForceBlendCycle1.set(forceBlend1, _force);
		const int forceBlend2 = gDP.otherMode.forceBlender;
		uForceBlendCycle2.set(forceBlend2, _force);

		if (!(graphics::Context::DualSourceBlending || graphics::Context::FramebufferFetchColor) || dwnd().getDrawer().isTexrectDrawerMode()) {
			// Modes, which shader blender can't emulate
			const u32 mode = _SHIFTR(gDP.otherMode.l, 16, 16);
			switch (mode) {
			case 0x0040:
				// Mia Hamm Soccer
				// clr_in * a_in + clr_mem * (1-a)
				// clr_in * a_in + clr_in * (1-a)
			case 0x0050:
				// A Bug's Life
				// clr_in * a_in + clr_mem * (1-a)
				// clr_in * a_in + clr_mem * (1-a)
				uForceBlendCycle1.set(0, _force);
				uForceBlendCycle2.set(0, _force);
				break;
			case 0x0150:
				// Tony Hawk
				// clr_in * a_in + clr_mem * (1-a)
				// clr_in * a_fog + clr_mem * (1-a_fog)
				if ((config.generalEmulation.hacks & hack_TonyHawk) != 0) {
					uForceBlendCycle1.set(0, _force);
					uForceBlendCycle2.set(0, _force);
				}
				break;
			}
		}

	}

private:
	i4Uniform uBlendMux1;
	i4Uniform uBlendMux2;
	iUniform uForceBlendCycle1;
	iUniform uForceBlendCycle2;
};

class UBlendCvg : public UniformGroup
{
public:
	UBlendCvg(GLuint _program) {
		LocateUniform(uCvgDest);
		LocateUniform(uBlendAlphaMode);
	}

	void update(bool _force) override
	{
		uCvgDest.set(gDP.otherMode.cvgDest, _force);
		if (dwnd().getDrawer().isTexrectDrawerMode())
			uBlendAlphaMode.set(2, _force); // No alpha blend in texrect drawing mode
		else
			uBlendAlphaMode.set(gDP.otherMode.forceBlender, _force);
	}
private:
	iUniform uCvgDest;
	iUniform uBlendAlphaMode;
};

class UDitherMode : public UniformGroup
{
public:
	UDitherMode(GLuint _program, bool _usesNoise)
	: m_usesNoise(_usesNoise)
	{
		LocateUniform(uAlphaCompareMode);
		LocateUniform(uAlphaDitherMode);
		LocateUniform(uColorDitherMode);
	}

	void update(bool _force) override
	{
		if (gDP.otherMode.cycleType < G_CYC_COPY) {
			uAlphaCompareMode.set(gDP.otherMode.alphaCompare, _force);
			uAlphaDitherMode.set(gDP.otherMode.alphaDither, _force);
			uColorDitherMode.set(gDP.otherMode.colorDither, _force);
		}
		else {
			uAlphaCompareMode.set(0, _force);
			uAlphaDitherMode.set(0, _force);
			uColorDitherMode.set(0, _force);
		}

		bool updateNoiseTex = m_usesNoise;
		updateNoiseTex |= (gDP.otherMode.cycleType < G_CYC_COPY) && (gDP.otherMode.colorDither == G_CD_NOISE || gDP.otherMode.alphaDither == G_AD_NOISE || gDP.otherMode.alphaCompare == G_AC_DITHER);
		if (updateNoiseTex)
			g_noiseTexture.update();
	}

private:
	iUniform uAlphaCompareMode;
	iUniform uAlphaDitherMode;
	iUniform uColorDitherMode;
	bool m_usesNoise;
};

class UScreenScale : public UniformGroup
{
public:
	UScreenScale(GLuint _program) {
		LocateUniform(uScreenScale);
	}

	void update(bool _force) override
	{
		if (dwnd().getDrawer().isTexrectDrawerMode()) {
			uScreenScale.set(1.0f, 1.0f, _force);
			return;
		}

		FrameBuffer * pBuffer = frameBufferList().getCurrent();
		if (pBuffer == nullptr)
			uScreenScale.set(dwnd().getScaleX(), dwnd().getScaleY(), _force);
		else
			uScreenScale.set(pBuffer->m_scale, pBuffer->m_scale, _force);
	}

private:
	fv2Uniform uScreenScale;
};

class UTexturePersp : public UniformGroup
{
public:
	UTexturePersp(GLuint _program) {
		LocateUniform(uTexturePersp);
	}

	void update(bool _force) override
	{
		const u32 texturePersp = (RSP.LLE || GBI.isTexturePersp()) ? gDP.otherMode.texturePersp : 1U;
		uTexturePersp.set(texturePersp, _force);
	}

private:
	iUniform uTexturePersp;
};

class UTextureFetchMode : public UniformGroup
{
public:
	UTextureFetchMode(GLuint _program) {
		LocateUniform(uTextureFilterMode);
		LocateUniform(uTextureFormat);
		LocateUniform(uTextureConvert);
		LocateUniform(uConvertParams);
	}

	void update(bool _force) override
	{
		int textureFilter = gDP.otherMode.textureFilter;
		uTextureFilterMode.set(textureFilter, _force);
		uTextureFormat.set(gSP.textureTile[0]->format, gSP.textureTile[1]->format, _force);
		uTextureConvert.set(gDP.otherMode.convert_one, _force);
		if (gDP.otherMode.bi_lerp0 == 0 || gDP.otherMode.bi_lerp1 == 0)
			uConvertParams.set(gDP.convert.k0, gDP.convert.k1, gDP.convert.k2, gDP.convert.k3, _force);
	}

private:
	iUniform uTextureFilterMode;
	iv2Uniform uTextureFormat;
	iUniform uTextureConvert;
	i4Uniform uConvertParams;
};

class UAlphaTestInfo : public UniformGroup
{
public:
	UAlphaTestInfo(GLuint _program) {
		LocateUniform(uEnableAlphaTest);
		LocateUniform(uAlphaCvgSel);
		LocateUniform(uCvgXAlpha);
		LocateUniform(uAlphaTestValue);
	}

	void update(bool _force) override
	{
		if (gDP.otherMode.cycleType == G_CYC_FILL) {
			uEnableAlphaTest.set(0, _force);
			uAlphaCvgSel.set(0, _force);

		} else if (gDP.otherMode.cycleType == G_CYC_COPY) {
			uAlphaCvgSel.set(0, _force);
			if (gDP.otherMode.alphaCompare & G_AC_THRESHOLD) {
				uEnableAlphaTest.set(1, _force);
				uAlphaTestValue.set(0.5f, _force);
			} else {
				uEnableAlphaTest.set(0, _force);
			}
		} else if ((gDP.otherMode.alphaCompare & G_AC_THRESHOLD) != 0) {
			uEnableAlphaTest.set(1, _force);
			uAlphaTestValue.set(gDP.blendColor.a, _force);
			uAlphaCvgSel.set(gDP.otherMode.alphaCvgSel, _force);
		} else {
			uEnableAlphaTest.set(0, _force);
			uAlphaCvgSel.set(gDP.otherMode.alphaCvgSel, _force);
		}

		uCvgXAlpha.set(gDP.otherMode.cvgXAlpha, _force);
	}

private:
	iUniform uEnableAlphaTest;
	iUniform uAlphaCvgSel;
	iUniform uCvgXAlpha;
	fUniform uAlphaTestValue;
};

class UViewportInfo : public UniformGroup
{
public:
	UViewportInfo(GLuint _program) {
		LocateUniform(uVTrans);
		LocateUniform(uVScale);
		LocateUniform(uAdjustTrans);
		LocateUniform(uAdjustScale);
	}

	void update(bool _force) override
	{
		const bool isOrthographicProjection = gSP.matrix.projection[3][2] == -1.f;
		float adjustTrans[2] = { 0.0f, 0.0f };
		float adjustScale[2] = { 1.0f, 1.0f };
		if (dwnd().isAdjustScreen() && (gDP.colorImage.width > VI.width * 98 / 100)) {
			if (isOrthographicProjection) {
				adjustScale[1] = 1.0f / dwnd().getAdjustScale();
				adjustTrans[1] = static_cast<f32>(gDP.colorImage.width) * 3.0f / 4.0f * (1.0f - adjustScale[1]) / 2.0f;
			} else {
				adjustScale[0] = dwnd().getAdjustScale();
				adjustTrans[0] = static_cast<f32>(gDP.colorImage.width) * (1.0f - adjustScale[0]) / 2.0f;
			}
		}
		uVTrans.set(gSP.viewport.vtrans[0], gSP.viewport.vtrans[1], _force);
		uVScale.set(gSP.viewport.vscale[0], -gSP.viewport.vscale[1], _force);
		uAdjustTrans.set(adjustTrans[0], adjustTrans[1], _force);
		uAdjustScale.set(adjustScale[0], adjustScale[1], _force);
	}

private:
	fv2Uniform uVTrans;
	fv2Uniform uVScale;
	fv2Uniform uAdjustTrans;
	fv2Uniform uAdjustScale;
};

class UDepthScale : public UniformGroup
{
public:
	UDepthScale(GLuint _program) {
		LocateUniform(uDepthScale);
	}

	void update(bool _force) override
	{
		if (RSP.LLE)
			uDepthScale.set(0.5f, 0.5f, _force);
		else
			uDepthScale.set(gSP.viewport.vscale[2], gSP.viewport.vtrans[2], _force);
	}

private:
	fv2Uniform uDepthScale;
};

class UDepthInfo : public UniformGroup
{
public:
	UDepthInfo(GLuint _program) {
		LocateUniform(uEnableDepth);
		LocateUniform(uEnableDepthCompare);
		LocateUniform(uEnableDepthUpdate);
		LocateUniform(uDepthMode);
		LocateUniform(uDepthSource);
		LocateUniform(uPrimDepth);
		LocateUniform(uDeltaZ);
	}

	void update(bool _force) override
	{
		FrameBuffer * pBuffer = frameBufferList().getCurrent();
		if (pBuffer == nullptr || pBuffer->m_pDepthBuffer == nullptr)
			return;

		const bool nDepthEnabled = ((gSP.geometryMode & G_ZBUFFER) || gDP.otherMode.depthSource == G_ZS_PRIM) &&
									gDP.otherMode.cycleType <= G_CYC_2CYCLE;
		uEnableDepth.set(nDepthEnabled ? 1 : 0, _force);
		if (nDepthEnabled) {
			uEnableDepthCompare.set(gDP.otherMode.depthCompare, _force);
			uEnableDepthUpdate.set(gDP.otherMode.depthUpdate, _force);
		} else {
			uEnableDepthCompare.set(0, _force);
			uEnableDepthUpdate.set(0, _force);
		}
		uDepthMode.set(gDP.otherMode.depthMode, _force);
		uDepthSource.set(gDP.otherMode.depthSource, _force);
		if (gDP.otherMode.depthSource == G_ZS_PRIM) {
			uDeltaZ.set(gDP.primDepth.deltaZ, _force);
			uPrimDepth.set(gDP.primDepth.z, _force);
		}
	}

private:
	iUniform uEnableDepth;
	iUniform uEnableDepthCompare;
	iUniform uEnableDepthUpdate;
	iUniform uDepthMode;
	iUniform uDepthSource;
	fUniform uPrimDepth;
	fUniform uDeltaZ;
};

class UDepthSource : public UniformGroup
{
public:
	UDepthSource(GLuint _program) {
		LocateUniform(uDepthSource);
		LocateUniform(uPrimDepth);
	}

	void update(bool _force) override
	{
		uDepthSource.set(gDP.otherMode.depthSource, _force);
		if (gDP.otherMode.depthSource == G_ZS_PRIM)
			uPrimDepth.set(gDP.primDepth.z, _force);
	}

private:
	iUniform uDepthSource;
	fUniform uPrimDepth;
};

class URenderTarget : public UniformGroup
{
public:
	URenderTarget(GLuint _program) {
		LocateUniform(uRenderTarget);
	}

	void update(bool _force) override
	{
		int renderTarget = 0;
		auto bgCopyMode = dwnd().getDrawer().getBgDepthCopyMode();
		if (bgCopyMode == GraphicsDrawer::BgDepthCopyMode::eBg1cyc)
			renderTarget = 3;
		else if (bgCopyMode == GraphicsDrawer::BgDepthCopyMode::eBgCopy)
			renderTarget = 4;
		else if (bgCopyMode == GraphicsDrawer::BgDepthCopyMode::eCopyDone)
			int renderTarget = 0;
		else if (isCurrentColorImageDepthImage())
			renderTarget = isDepthCompareEnabled() ? 2 : 1;
		uRenderTarget.set(renderTarget, _force);
	}

private:
	iUniform uRenderTarget;
};

class UClampMode : public UniformGroup
{
public:
	UClampMode(GLuint _program) {
		LocateUniform(uClampMode);
	}

	void update(bool _force) override
	{
		int clampMode = -1;
		switch (gfxContext.getClampMode())
		{
			case graphics::ClampMode::ClippingEnabled:
				clampMode = 0;
				break;
			case graphics::ClampMode::NoNearPlaneClipping:
				clampMode = 1;
				break;
			case graphics::ClampMode::NoClipping:
				clampMode = 2;
				break;
		}
		uClampMode.set(clampMode, _force);
	}

private:
	iUniform uClampMode;
};

class UPolygonOffset : public UniformGroup
{
public:
	UPolygonOffset(GLuint _program) {
		LocateUniform(uPolygonOffset);
	}

	void update(bool _force) override
	{
		f32 offset = gfxContext.isEnabled(graphics::enable::POLYGON_OFFSET_FILL) ? 0.003f : 0.0f;
		uPolygonOffset.set(offset, _force);
	}

private:
	fUniform uPolygonOffset;
};

class UScreenCoordsScale : public UniformGroup
{
public:
	UScreenCoordsScale(GLuint _program) {
		LocateUniform(uScreenCoordsScale);
	}

	void update(bool _force) override
	{
		f32 scaleX, scaleY;
		calcCoordsScales(frameBufferList().getCurrent(), scaleX, scaleY);
		uScreenCoordsScale.set(2.0f*scaleX, -2.0f*scaleY, _force);
	}

private:
	fv2Uniform uScreenCoordsScale;
};

class UColors : public UniformGroup
{
public:
	UColors(GLuint _program) {
		LocateUniform(uFogColor);
		LocateUniform(uCenterColor);
		LocateUniform(uScaleColor);
		LocateUniform(uBlendColor);
		LocateUniform(uEnvColor);
		LocateUniform(uPrimColor);
		LocateUniform(uPrimLod);
		LocateUniform(uK4);
		LocateUniform(uK5);
	}

	void update(bool _force) override
	{
		uFogColor.set(&gDP.fogColor.r, _force);
		uCenterColor.set(&gDP.key.center.r, _force);
		uScaleColor.set(&gDP.key.scale.r, _force);
		uBlendColor.set(&gDP.blendColor.r, _force);
		uEnvColor.set(&gDP.envColor.r, _force);
		uPrimColor.set(&gDP.primColor.r, _force);
		uPrimLod.set(gDP.primColor.l, _force);
		uK4.set(_FIXED2FLOATCOLOR(gDP.convert.k4, 8 ), _force);
		uK5.set(_FIXED2FLOATCOLOR(gDP.convert.k5, 8 ), _force);
	}

private:
	fv4Uniform uFogColor;
	fv4Uniform uCenterColor;
	fv4Uniform uScaleColor;
	fv4Uniform uBlendColor;
	fv4Uniform uEnvColor;
	fv4Uniform uPrimColor;
	fUniform uPrimLod;
	fUniform uK4;
	fUniform uK5;
};

class URectColor : public UniformGroup
{
public:
	URectColor(GLuint _program) {
		LocateUniform(uRectColor);
	}

	void update(bool _force) override
	{
		uRectColor.set(&gDP.rectColor.r, _force);
	}

private:
	fv4Uniform uRectColor;
};

class ULights : public UniformGroup
{
public:
	ULights(GLuint _program)
	{
		char buf[32];
		for (s32 i = 0; i < 8; ++i) {
			sprintf(buf, "uLightDirection[%d]", i);
			uLightDirection[i].loc = glGetUniformLocation(_program, buf);
			sprintf(buf, "uLightColor[%d]", i);
			uLightColor[i].loc = glGetUniformLocation(_program, buf);
		}
	}

	void update(bool _force) override
	{
		for (u32 i = 0; i <= gSP.numLights; ++i) {
			uLightDirection[i].set(gSP.lights.xyz[i], _force);
			uLightColor[i].set(gSP.lights.rgb[i], _force);
		}
	}

private:
	fv3Uniform uLightDirection[8];
	fv3Uniform uLightColor[8];
};

} //nameless namespace

/*---------------CombinerProgramUniformFactoryCommon-------------*/
namespace glsl {

void CombinerProgramUniformFactoryCommon::_addNoiseTex(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UNoiseTex(_program));
}

void CombinerProgramUniformFactoryCommon::_addScreenSpaceTriangleInfo(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UScreenSpaceTriangleInfo(_program));
}

void CombinerProgramUniformFactoryCommon::_addRasterInfo(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UScreenSpaceTriangleInfo(_program));
}

void CombinerProgramUniformFactoryCommon::_addViewportInfo(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UViewportInfo(_program));
}

void CombinerProgramUniformFactoryCommon::_addDepthTex(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UDepthTex(_program));
}

void CombinerProgramUniformFactoryCommon::_addDepthScale(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UDepthScale(_program));
}

void CombinerProgramUniformFactoryCommon::_addTextures(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UTextures(_program));
}

void CombinerProgramUniformFactoryCommon::_addMSAATextures(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UMSAATextures(_program));
}

void CombinerProgramUniformFactoryCommon::_addFrameBufferInfo(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UFrameBufferInfo(_program));
}

void CombinerProgramUniformFactoryCommon::_addTextureFetchMode(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UTextureFetchMode(_program));
}

void CombinerProgramUniformFactoryCommon::_addTexturePersp(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UTexturePersp(_program));
}

void CombinerProgramUniformFactoryCommon::_addFog(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UFog(_program));
}

void CombinerProgramUniformFactoryCommon::_addBlendMode1Cycle(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UBlendMode1Cycle(_program));
}

void CombinerProgramUniformFactoryCommon::_addBlendMode2Cycle(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UBlendMode2Cycle(_program));
}

void CombinerProgramUniformFactoryCommon::_addBlendCvg(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UBlendCvg(_program));
}

void CombinerProgramUniformFactoryCommon::_addDitherMode(GLuint _program, UniformGroups &_uniforms, bool _usesNoise) const
{
	_uniforms.emplace_back(new UDitherMode(_program, _usesNoise));
}

void CombinerProgramUniformFactoryCommon::_addScreenScale(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UScreenScale(_program));
}

void CombinerProgramUniformFactoryCommon::_addAlphaTestInfo(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UAlphaTestInfo(_program));
}

void CombinerProgramUniformFactoryCommon::_addZLutTexture(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UZLutTexture(_program));
}

void CombinerProgramUniformFactoryCommon::_addDepthInfo(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UDepthInfo(_program));
}

void CombinerProgramUniformFactoryCommon::_addDepthSource(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UDepthSource(_program));
}

void CombinerProgramUniformFactoryCommon::_addRenderTarget(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new URenderTarget(_program));
}

void CombinerProgramUniformFactoryCommon::_addClampMode(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UClampMode(_program));
}

void CombinerProgramUniformFactoryCommon::_addPolygonOffset(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UPolygonOffset(_program));
}

void CombinerProgramUniformFactoryCommon::_addScreenCoordsScale(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UScreenCoordsScale(_program));
}

void CombinerProgramUniformFactoryCommon::_addColors(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new UColors(_program));
}

void CombinerProgramUniformFactoryCommon::_addRectColor(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new URectColor(_program));
}

void CombinerProgramUniformFactoryCommon::_addLights(GLuint _program, UniformGroups &_uniforms) const
{
	_uniforms.emplace_back(new ULights(_program));
}

CombinerProgramUniformFactoryCommon::CombinerProgramUniformFactoryCommon(const opengl::GLInfo & _glInfo)
: CombinerProgramUniformFactory(_glInfo)
{

}

}
