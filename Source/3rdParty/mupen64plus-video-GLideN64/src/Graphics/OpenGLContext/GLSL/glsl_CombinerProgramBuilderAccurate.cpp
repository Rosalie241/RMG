#include <Config.h>
#include "glsl_CombinerProgramBuilderAccurate.h"
#include "glsl_CombinerProgramUniformFactoryAccurate.h"

namespace {
using namespace glsl;

class VertexShaderTexturedTriangle : public ShaderPart
{
public:
	VertexShaderTexturedTriangle(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"IN highp vec4 aPosition;							\n"
			"IN lowp vec4 aColor;								\n"
			"IN highp vec2 aTexCoord;							\n"
			"IN lowp float aNumLights;							\n"
			"IN highp vec4 aModify;								\n"
			"IN highp vec2 aBaryCoords;							\n"
			"													\n"
			"uniform int uTexturePersp;							\n"
			"uniform lowp int uTextureFilterMode;				\n"
			"													\n"
			"uniform lowp int uFogUsage;						\n"
			"uniform mediump vec2 uFogScale;					\n"
			"uniform mediump vec2 uScreenCoordsScale;			\n"
			"													\n"
			"uniform mediump vec2 uTexScale;					\n"
			"uniform mediump vec2 uVTrans;						\n"
			"uniform mediump vec2 uVScale;						\n"
			"uniform mediump vec2 uAdjustTrans;					\n"
			"uniform mediump vec2 uAdjustScale;					\n"
			"uniform lowp ivec2 uCacheFrameBuffer;				\n"
			"OUT highp vec2 vTexCoord;							\n"
			"OUT mediump vec2 vLodTexCoord;						\n"
			"OUT lowp float vNumLights;							\n"
			"OUT lowp vec4 vShadeColor;							\n"
			"OUT highp vec4 vBaryCoords;						\n"
		;
		if (!_glinfo.isGLESX || _glinfo.noPerspective)
			m_part += "noperspective OUT lowp vec4 vShadeColorNoperspective;\n";
		else
			m_part += "OUT lowp vec4 vShadeColorNoperspective;				\n";
		m_part +=
			"void main()													\n"
			"{																\n"
			"  gl_Position = aPosition;										\n"
			"  vShadeColor = aColor;										\n"
			"  vec2 texCoord = aTexCoord;									\n"
			"  texCoord *= uTexScale;										\n"
			"  if (uTexturePersp == 0 && aModify[2] == 0.0) texCoord *= 0.5;\n"
			"  vTexCoord = texCoord;										\n"
			"  vLodTexCoord = texCoord;										\n"
			"  vNumLights = aNumLights;										\n"
			"  if ((aModify[0]) != 0.0) {									\n"
			"    gl_Position.xy *= gl_Position.w;							\n"
			"  }															\n"
			"  else {														\n"
			"    gl_Position.xy = gl_Position.xy * uVScale.xy + uVTrans.xy * gl_Position.ww; \n"
			"    gl_Position.xy = floor(gl_Position.xy * vec2(4.0)) * vec2(0.25); \n"
			"    gl_Position.xy = gl_Position.xy * uAdjustScale + gl_Position.ww * uAdjustTrans; \n"
			"  }															\n"
			"  if ((aModify[1]) != 0.0)										\n"
			"    gl_Position.z *= gl_Position.w;							\n"
			"  if ((aModify[3]) != 0.0)										\n"
			"    vNumLights = 0.0;											\n"
			"  if (uFogUsage > 0) {											\n"
			"    lowp float fp;												\n"
			"    if (aPosition.z < -aPosition.w && aModify[1] == 0.0)		\n"
			"      fp = -uFogScale.s + uFogScale.t;							\n"
			"    else														\n"
			"      fp = aPosition.z/aPosition.w*uFogScale.s + uFogScale.t;	\n"
			"    fp = clamp(fp, 0.0, 1.0);									\n"
			"    if (uFogUsage == 1)										\n"
			"      vShadeColor.a = fp;										\n"
			"    else														\n"
			"      vShadeColor.rgb = vec3(fp);								\n"
			"  }															\n"
			"  vBaryCoords = vec4(aBaryCoords, 1.0 - aBaryCoords.x - aBaryCoords.y, 0.5);	\n"
			"  vShadeColorNoperspective = vShadeColor;							\n"
			;
	}
};

class VertexShaderTexturedRect : public ShaderPart
{
public:
	VertexShaderTexturedRect(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"IN highp vec4 aRectPosition;						\n"
			"IN highp vec2 aTexCoord0;							\n"
			"IN highp vec2 aBaryCoords;							\n"
			"													\n"
			"OUT highp vec2 vTexCoord;							\n"
			"OUT lowp vec4 vShadeColor;							\n"
			"OUT highp vec4 vBaryCoords;						\n"
			;
		if (!_glinfo.isGLESX || _glinfo.noPerspective)
			m_part += "noperspective OUT lowp vec4 vShadeColorNoperspective;\n";
		else
			m_part += "OUT lowp vec4 vShadeColorNoperspective;				\n";
		m_part +=
			"uniform lowp vec4 uRectColor;						\n"
			"void main()										\n"
			"{													\n"
			"  gl_Position = aRectPosition;						\n"
			"  vShadeColor = uRectColor;						\n"
			"  vShadeColorNoperspective = uRectColor;			\n"
			"  vTexCoord = aTexCoord0;							\n"
			"  vBaryCoords = vec4(aBaryCoords, vec2(1.0) - aBaryCoords);	\n"
			;
	}
};

class ShaderFragmentCorrectTexCoords : public ShaderPart {
public:
	ShaderFragmentCorrectTexCoords() {
		m_part +=
			" highp vec2 mTexCoord = vTexCoord + vec2(0.0001);						\n"
			" mTexCoord += uTexCoordOffset;											\n"
			" if (uUseTexCoordBounds != 0)											\n"
			" mTexCoord = clamp(mTexCoord, uTexCoordBounds.xy, uTexCoordBounds.zw);	\n"
			"																		\n"
			;
	}
};

class ShaderFragmentGlobalVariablesTex : public ShaderPart
{
public:
	ShaderFragmentGlobalVariablesTex(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"uniform sampler2D uTex0;		\n"
			"uniform sampler2D uTex1;		\n"
			"uniform lowp vec4 uFogColor;	\n"
			"uniform lowp vec4 uCenterColor;\n"
			"uniform lowp vec4 uScaleColor;	\n"
			"uniform lowp vec4 uBlendColor;	\n"
			"uniform lowp vec4 uEnvColor;	\n"
			"uniform lowp vec4 uPrimColor;	\n"
			"uniform lowp float uPrimLod;	\n"
			"uniform lowp float uK4;		\n"
			"uniform lowp float uK5;		\n"
			"uniform lowp int uAlphaCompareMode;	\n"
			"uniform lowp ivec2 uFbMonochrome;		\n"
			"uniform lowp ivec2 uFbFixedAlpha;		\n"
			"uniform lowp int uEnableAlphaTest;		\n"
			"uniform lowp int uCvgXAlpha;			\n"
			"uniform lowp int uAlphaCvgSel;			\n"
			"uniform lowp float uAlphaTestValue;	\n"
			"uniform lowp int uDepthSource;			\n"
			"uniform highp float uPrimDepth;		\n"
			"uniform mediump vec2 uScreenScale;		\n"
			"uniform highp vec2 uTexClamp[2];		\n"
			"uniform highp vec2 uTexWrap[2];		\n"
			"uniform lowp vec2 uTexWrapEn[2];		\n"
			"uniform lowp vec2 uTexMirrorEn[2];		\n"
			"uniform lowp vec2 uTexClampEn[2];		\n"
			"uniform highp vec2 uTexSize[2];		\n"
			"uniform highp vec2 uShiftScale[2];		\n"
			"uniform highp vec2 uTexOffset[2];		\n"
			"uniform highp vec2 uHDRatio[2];		\n"
			"uniform highp vec2 uTexCoordOffset;	\n"
			"uniform highp vec2 uBilinearOffset;	\n"
			"uniform highp vec2 uCacheOffset[2];	\n"
			"uniform lowp int uUseTexCoordBounds;	\n"
			"uniform highp vec4 uTexCoordBounds;	\n"
			"uniform lowp int uScreenSpaceTriangle;	\n"
			"highp vec2 texCoord0;					\n"
			"highp vec2 texCoord1;					\n"
			"highp vec2 tcData0[5];					\n"
			"highp vec2 tcData1[5];					\n"
			"uniform lowp int uCvgDest;				\n"
			"uniform lowp int uBlendAlphaMode;		\n"
			"lowp float cvg;		\n"
			;

		if (config.generalEmulation.enableLegacyBlending != 0) {
			m_part +=
				"uniform lowp int uFogUsage;		\n"
			;
		} else {
			m_part +=
				"uniform lowp ivec4 uBlendMux1;		\n"
				"uniform lowp int uForceBlendCycle1;\n"
			;
		}

		if (!_glinfo.isGLES2) {
			m_part +=
				"uniform sampler2D uDepthTex;		\n"
				"uniform lowp int uAlphaDitherMode;	\n"
				"uniform lowp int uColorDitherMode;	\n"
				"uniform lowp int uRenderTarget;	\n"
				"uniform mediump vec2 uDepthScale;	\n"
				;
			if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
				m_part +=
					"uniform lowp int uEnableDepthCompare;	\n"
					;
			}
		} else {
			m_part +=
				"lowp int nCurrentTile;			\n"
			;
		}

		if (config.video.multisampling > 0) {
			m_part +=
				"uniform lowp ivec2 uMSTexEnabled;	\n"
				"uniform lowp sampler2DMS uMSTex0;	\n"
				"uniform lowp sampler2DMS uMSTex1;	\n"
			;
		}

		if (!_glinfo.isGLESX || _glinfo.noPerspective)
			m_part += "noperspective IN lowp vec4 vShadeColorNoperspective;	\n";
		else
			m_part += "IN lowp vec4 vShadeColorNoperspective;				\n";

		m_part +=
			"IN lowp vec4 vShadeColor;		\n"
			"IN highp vec2 vTexCoord;		\n"
			"IN mediump vec2 vLodTexCoord;	\n"
			"IN lowp float vNumLights;		\n"
			"IN highp vec4 vBaryCoords;		\n"
		;

		if (_glinfo.dual_source_blending) {
			m_part +=
				"layout(location = 0, index = 0) OUT lowp vec4 fragColor; 	\n"  // MAIN FRAGMENT SHADER OUTPUT
				"layout(location = 0, index = 1) OUT lowp vec4 fragColor1;	\n"  // SECONDARY FRAGMENT SHADER OUTPUT
				"#define LAST_FRAG_COLOR vec4(0.0)							\n"  // DUMMY
				"#define LAST_FRAG_ALPHA 1.0								\n"  // DUMMY
				;
		} else if (_glinfo.ext_fetch) {
			m_part +=
				"layout(location = 0) inout lowp vec4 fragColor;		\n"  // MAIN FRAGMENT SHADER OUTPUT
				"lowp vec4 fragColor1;									\n"  // DUMMY
				"#define LAST_FRAG_COLOR fragColor						\n"  // CURRENT FRAMEBUFFER COLOR/ALPHA
				"#define LAST_FRAG_ALPHA fragColor.a					\n"  // CURRENT FRAMEBUFFER ALPHA
				;
		} else if (_glinfo.ext_fetch_arm) {
			m_part +=
				"OUT lowp vec4 fragColor;								\n"  // MAIN FRAGMENT SHADER OUTPUT
				"lowp vec4 fragColor1;									\n"  // DUMMY
				"#define LAST_FRAG_COLOR gl_LastFragColorARM			\n"  // CURRENT FRAMEBUFFER COLOR/ALPHA
				"#define LAST_FRAG_ALPHA gl_LastFragColorARM.a			\n"  // CURRENT FRAMEBUFFER ALPHA
				;
		} else {
			m_part +=
				"OUT lowp vec4 fragColor;								\n"  // MAIN FRAGMENT SHADER OUTPUT
				"lowp vec4 fragColor1;									\n"  // DUMMY
				"#define LAST_FRAG_COLOR vec4(0.0)						\n"  // DUMMY
				"#define LAST_FRAG_ALPHA 1.0							\n"  // DUMMY
				;
		}

		if (config.frameBufferEmulation.N64DepthCompare == Config::dcFast && _glinfo.n64DepthWithFbFetch) {
			m_part +=
				"layout(location = 1) inout highp vec4 depthZ;	\n"
				"layout(location = 2) inout highp vec4 depthDeltaZ;	\n"
				;
		}

		if (_glinfo.isGLES2)
			m_part +=
			"uniform mediump vec2 uTextureSize[2];	\n"
			;
	}
};

class ShaderFragmentHeaderTextureEngine : public ShaderPart
{
public:
	ShaderFragmentHeaderTextureEngine(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"highp vec2 clampWrapMirror(in highp vec2 vTexCoord,	\n"
			"	in highp vec2 vWrap, in highp vec2 vClamp,			\n"
			"	in lowp vec2 vClampEn, in lowp vec2 vMirrorEn );	\n"
			"void textureEngine0(in highp vec2 texCoord, out highp vec2 tcData[5]); \n"
			"void textureEngine1(in highp vec2 texCoord, out highp vec2 tcData[5]); \n"
			;
	}
};

class ShaderFragmentHeaderReadMSTex : public ShaderPart
{
public:
	ShaderFragmentHeaderReadMSTex(const opengl::GLInfo & _glinfo) : m_glinfo(_glinfo)
	{
	}

	void write(std::stringstream & shader) const override
	{
		if (!m_glinfo.isGLES2 &&
			config.video.multisampling > 0 &&
			(CombinerProgramBuilder::s_cycleType == G_CYC_COPY || CombinerProgramBuilder::s_textureConvert.useTextureFiltering()))
		{
			shader <<
				"lowp vec4 readTexMS(in lowp sampler2DMS mstex, in highp vec2 tcData[5], in lowp int fbMonochrome, in lowp int fbFixedAlpha);\n";
		}
	}

private:
	const opengl::GLInfo& m_glinfo;
};

class ShaderFragmentHeaderReadTex : public ShaderPart
{
public:
	ShaderFragmentHeaderReadTex(const opengl::GLInfo & _glinfo) : m_glinfo(_glinfo)
	{
	}

	void write(std::stringstream & shader) const override
	{
		std::string shaderPart;
		if (!m_glinfo.isGLES2) {

			if (CombinerProgramBuilder::s_textureConvert.useTextureFiltering()) {
				shaderPart += "uniform lowp int uTextureFilterMode;								\n";
				shaderPart += "#define TEX_NEAREST(name, tex, tcData)							\\\n"
					"{																			\\\n"
					" name = texelFetch(tex, ivec2(tcData[0]), 0); \\\n"
					"}																			\n"
					;
				switch (config.texture.bilinearMode + config.texture.enableHalosRemoval * 2) {
				case BILINEAR_3POINT:
					// 3 point texture filtering.
					// Original author: ArthurCarvalho
					// GLSL implementation: twinaphex, mupen64plus-libretro project.
					shaderPart +=
						"#define TEX_FILTER(name, tex, tcData)												\\\n"
						"  {																					\\\n"
						"  lowp float bottomRightTri = step(1.0, tcData[4].s + tcData[4].t);					\\\n"
						"  lowp vec4 c00 = texelFetch(tex, ivec2(tcData[0]), 0); \\\n"
						"  lowp vec4 c01 = texelFetch(tex, ivec2(tcData[1]), 0); \\\n"
						"  lowp vec4 c10 = texelFetch(tex, ivec2(tcData[2]), 0); \\\n"
						"  lowp vec4 c11 = texelFetch(tex, ivec2(tcData[3]), 0); \\\n"
						"  lowp vec4 c0 = c00 + tcData[4].s*(c10-c00) + tcData[4].t*(c01-c00);			\\\n"
						"  lowp vec4 c1 = c11 + (1.0-tcData[4].s)*(c01-c11) + (1.0-tcData[4].t)*(c10-c11); \\\n"
						"  name = c0 + bottomRightTri * (c1-c0); \\\n"
						"  }																					\n"
						;
				break;
				case BILINEAR_STANDARD:
					shaderPart +=
						"#define TEX_FILTER(name, tex, tcData)																		\\\n"
						"{																											\\\n"
						"  lowp vec4 c00 = texelFetch(tex, ivec2(tcData[0]), 0); \\\n"
						"  lowp vec4 c01 = texelFetch(tex, ivec2(tcData[1]), 0); \\\n"
						"  lowp vec4 c10 = texelFetch(tex, ivec2(tcData[2]), 0); \\\n"
						"  lowp vec4 c11 = texelFetch(tex, ivec2(tcData[3]), 0); \\\n"
						"  lowp vec4 c0 = c00 + tcData[4].s * (c10-c00);						\\\n"
						"  lowp vec4 c1 = c01 + tcData[4].s * (c11-c01);						\\\n"
						"  name = c0 + tcData[4].t * (c1-c0);									\\\n"
						"}																												\n"
						;
				break;
				case BILINEAR_3POINT_WITH_COLOR_BLEEDING:
					// 3 point texture filtering.
					// Original author: ArthurCarvalho
					// GLSL implementation: twinaphex, mupen64plus-libretro project.
					shaderPart +=
						"#define TEX_FILTER(name, tex, tcData)												\\\n"
						"{																						\\\n"
						"  lowp float bottomRightTri = step(1.0, tcData[4].s + tcData[4].t);					\\\n"
						"  lowp vec4 c00 = texelFetch(tex, ivec2(tcData[0]), 0);								\\\n"
						"  lowp vec4 c01 = texelFetch(tex, ivec2(tcData[1]), 0);								\\\n"
						"  lowp vec4 c10 = texelFetch(tex, ivec2(tcData[2]), 0);								\\\n"
						"  lowp vec4 c11 = texelFetch(tex, ivec2(tcData[3]), 0);								\\\n"
						"  if(uEnableAlphaTest == 1 ){															\\\n" // Calculate premultiplied color values
						"    c00.rgb *= c00.a;																	\\\n"
						"    c01.rgb *= c01.a;																	\\\n"
						"    c10.rgb *= c10.a;																	\\\n"
						"    c11.rgb *= c11.a;																	\\\n"
						"  }																					\\\n"
						"  lowp vec4 c0 = c00 + tcData[4].s*(c10-c00) + tcData[4].t*(c01-c00);				\\\n"
						"  lowp vec4 c1 = c11 + (1.0-tcData[4].s)*(c01-c11) + (1.0-tcData[4].t)*(c10-c11);	\\\n"
						"  name = c0 + bottomRightTri * (c1-c0); \\\n"
						"  if(uEnableAlphaTest == 1 ) name.rgb /= name.a;										\\\n" // Divide alpha to get actual color value
						"}																						\n"
						;
				break;
				case BILINEAR_STANDARD_WITH_COLOR_BLEEDING_AND_PREMULTIPLIED_ALPHA:
					shaderPart +=
						"#define TEX_FILTER(name, tex, tcData)																	\\\n"
						"{																										\\\n"
						"  lowp vec4 c00 = texelFetch(tex, ivec2(tcData[0]), 0);												\\\n"
						"  lowp vec4 c01 = texelFetch(tex, ivec2(tcData[1]), 0);												\\\n"
						"  lowp vec4 c10 = texelFetch(tex, ivec2(tcData[2]), 0);												\\\n"
						"  lowp vec4 c11 = texelFetch(tex, ivec2(tcData[3]), 0);												\\\n"
						"  if(uEnableAlphaTest == 1){																			\\\n" // Calculate premultiplied color values
						"    c00.rgb *= c00.a;																					\\\n"
						"    c01.rgb *= c01.a;																					\\\n"
						"    c10.rgb *= c10.a;																					\\\n"
						"    c11.rgb *= c11.a;																					\\\n"
						"  }																									\\\n"
						"  lowp vec4 c0 = c00 + tcData[4].s * (c10-c00);														\\\n"
						"  lowp vec4 c1 = c01 + tcData[4].s * (c11-c01);														\\\n"
						"  name = c0 + tcData[4].t * (c1-c0);																	\\\n"
						"  if(uEnableAlphaTest == 1)  name.rgb /= name.a;														\\\n"
						"}																										\n"
						;
				break;
				}
				shaderPart +=
					"#define READ_TEX(name, tex, tcData, fbMonochrome, fbFixedAlpha)	\\\n"
					"  {																\\\n"
					"  if (fbMonochrome == 3) {											\\\n"
					"    mediump ivec2 coord = ivec2(gl_FragCoord.xy);					\\\n"
					"    name = texelFetch(tex, coord, 0);								\\\n"
					"  } else {															\\\n"
					"    if (uTextureFilterMode == 0)									\\\n"
					"  {																\\\n"
					"    TEX_NEAREST(name, tex, tcData);								\\\n"
					"  }																\\\n"
					"    else TEX_FILTER(name, tex, tcData);			 				\\\n"
					"  }																\\\n"
					"  if (fbMonochrome == 1) name = vec4(name.r);						\\\n"
					"  else if (fbMonochrome == 2) 										\\\n"
					"    name.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), name.rgb));	\\\n"
					"  else if (fbMonochrome == 3) { 									\\\n"
					"    name.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), name.rgb));	\\\n"
					"    name.a = 0.0;													\\\n"
					"  }																\\\n"
					"  if (fbFixedAlpha == 1) name.a = 0.825;							\\\n"
					"  }																\n"
					;
			}

			if (CombinerProgramBuilder::s_textureConvert.useYUVCoversion()) {
				shaderPart +=
					"uniform lowp ivec2 uTextureFormat;									\n"
					"uniform lowp int uTextureConvert;									\n"
					"uniform mediump ivec4 uConvertParams;								\n"
					"#define YUVCONVERT(name, format)									\\\n"
					"  mediump ivec4 icolor = ivec4(name*255.0);						\\\n"
					"  if (format == 1)													\\\n"
					"    icolor.rg -= 128;												\\\n"
					"  mediump ivec4 iconvert;											\\\n"
					"  iconvert.r = icolor.b + (uConvertParams[0]*icolor.g + 128)/256;	\\\n"
					"  iconvert.g = icolor.b + (uConvertParams[1]*icolor.r + uConvertParams[2]*icolor.g + 128)/256;	\\\n"
					"  iconvert.b = icolor.b + (uConvertParams[3]*icolor.r + 128)/256;	\\\n"
					"  iconvert.a = icolor.b;											\\\n"
					"  name = vec4(iconvert)/255.0;										\n"
					"#define YUVCONVERT_TEX0(name, tex, tcData, format)				\\\n"
					"  {																\\\n"
					"  name = texelFetch(tex, ivec2(tcData[0]), 0);									\\\n"
					"  YUVCONVERT(name, format)											\\\n"
					"  }																\n"
					"#define YUVCONVERT_TEX1(name, tex, tcData, format, prev)			\\\n"
					"  {																\\\n"
					"  if (uTextureConvert != 0) name = prev;							\\\n"
					"  else name = texelFetch(tex, ivec2(tcData[0]), 0);								\\\n"
					"  YUVCONVERT(name, format)											\\\n"
					"  }																\n"
					;
			}

		} else {
			if (CombinerProgramBuilder::s_textureConvert.useTextureFiltering()) {
				shaderPart +=
					"uniform lowp int uTextureFilterMode;								\n"
					"lowp vec4 readTex(in sampler2D tex, in highp vec2 tcData[5], in lowp int fbMonochrome, in lowp int fbFixedAlpha);	\n"
					;
			}
			if (CombinerProgramBuilder::s_textureConvert.useYUVCoversion()) {
				shaderPart +=
					"uniform lowp ivec2 uTextureFormat;									\n"
					"uniform lowp int uTextureConvert;									\n"
					"uniform mediump ivec4 uConvertParams;								\n"
					"lowp vec4 YUV_Convert(in sampler2D tex, in highp vec2 tcData[5], in lowp int convert, in lowp int format, in lowp vec4 prev);	\n"
					;
			}
		}

		shader << shaderPart;
	}

private:
	const opengl::GLInfo& m_glinfo;
};

class ShaderFragmentHeaderReadTexCopyMode : public ShaderPart
{
public:
	ShaderFragmentHeaderReadTexCopyMode (const opengl::GLInfo & _glinfo)
	{
		if (!_glinfo.isGLES2) {
			m_part =
				"#define READ_TEX(name, tex, tcData, fbMonochrome, fbFixedAlpha)	\\\n"
				"  {																\\\n"
				"  if (fbMonochrome == 3) {											\\\n"
				"    mediump ivec2 coord = ivec2(gl_FragCoord.xy);					\\\n"
				"    name = texelFetch(tex, coord, 0);								\\\n"
				"  } else {															\\\n"
				"    name = texelFetch(tex, ivec2(tcData[0]),0);					\\\n"
				"  }																\\\n"
				"  if (fbMonochrome == 1) name = vec4(name.r);						\\\n"
				"  else if (fbMonochrome == 2) 										\\\n"
				"    name.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), name.rgb));	\\\n"
				"  else if (fbMonochrome == 3) { 									\\\n"
				"    name.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), name.rgb));	\\\n"
				"    name.a = 0.0;													\\\n"
				"  }																\\\n"
				"  if (fbFixedAlpha == 1) name.a = 0.825;							\\\n"
				"  }																\n"
				;
		} else {
			m_part =
				"lowp vec4 readTex(in sampler2D tex, in highp vec2 tcData[5], in lowp int fbMonochrome, in lowp int fbFixedAlpha);	\n"
			;
		}
	}
};

class ShaderFragmentReadTexCopyMode : public ShaderPart
{
public:
	ShaderFragmentReadTexCopyMode(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			m_part =
				"  nCurrentTile = 0; \n"
				"  lowp vec4 readtex0 = readTex(uTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0]);		\n"
				;
		} else {
			if (config.video.multisampling > 0) {
				m_part =
					"  lowp vec4 readtex0;																	\n"
					"  if (uMSTexEnabled[0] == 0) {															\n"
					"      READ_TEX(readtex0, uTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0])		\n"
					"  } else readtex0 = readTexMS(uMSTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0]);\n"
					;
			} else {
				m_part =
					"  lowp vec4 readtex0;																	\n"
					"  READ_TEX(readtex0, uTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0])			\n"
					;
			}
		}
	}
};

class ShaderFragmentReadTex0 : public ShaderPart
{
public:
	ShaderFragmentReadTex0(const opengl::GLInfo & _glinfo) : m_glinfo(_glinfo)
	{
	}

	void write(std::stringstream & shader) const override
	{
		std::string shaderPart;

		if (m_glinfo.isGLES2) {

			shaderPart = "  nCurrentTile = 0; \n";
			if (CombinerProgramBuilder::s_textureConvert.getBilerp0()) {
				shaderPart += "  lowp vec4 readtex0 = readTex(uTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0]);		\n";
			} else {
				shaderPart += "  lowp vec4 tmpTex = vec4(0.0);																\n"
							  "  lowp vec4 readtex0 = YUV_Convert(uTex0, tcData0, 0, uTextureFormat[0], tmpTex);			\n";
			}

		} else {

			if (!CombinerProgramBuilder::s_textureConvert.getBilerp0()) {
				shaderPart = "  lowp vec4 readtex0;																			\n"
							 "  YUVCONVERT_TEX0(readtex0, uTex0, tcData0, uTextureFormat[0])								\n";
			} else {
				if (config.video.multisampling > 0) {
					shaderPart =
						"  lowp vec4 readtex0;																				\n"
						"  if (uMSTexEnabled[0] == 0) {																		\n"
						"    READ_TEX(readtex0, uTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0])						\n"
						"  } else readtex0 = readTexMS(uMSTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0]);			\n";
				} else {
					shaderPart = "  lowp vec4 readtex0;																		\n"
								" READ_TEX(readtex0, uTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0])				\n";
				}
			}

		}

		shader << shaderPart;
	}

private:
	const opengl::GLInfo& m_glinfo;
};

class ShaderFragmentReadTex1 : public ShaderPart
{
public:
	ShaderFragmentReadTex1(const opengl::GLInfo & _glinfo) : m_glinfo(_glinfo)
	{
	}

	void write(std::stringstream & shader) const override
	{
		std::string shaderPart;

		if (m_glinfo.isGLES2) {

			shaderPart = "  nCurrentTile = 1; \n";

			if (CombinerProgramBuilder::s_textureConvert.getBilerp1()) {
				shaderPart += "  lowp vec4 readtex1 = readTex(uTex1, tcData1, uFbMonochrome[1], uFbFixedAlpha[1]);				\n";
			} else {
				shaderPart += "  lowp vec4 readtex1 = YUV_Convert(uTex1, tcData1, uTextureConvert, uTextureFormat[1], readtex0);	\n";
			}

		} else {

			if (!CombinerProgramBuilder::s_textureConvert.getBilerp1()) {
				shaderPart =
					"  lowp vec4 readtex1;																							\n"
					"    YUVCONVERT_TEX1(readtex1, uTex1, tcData1, uTextureFormat[1], readtex0)					\n";
			} else {
				if (config.video.multisampling > 0) {
					shaderPart =
						"  lowp vec4 readtex1;																						\n"
						"  if (uMSTexEnabled[1] == 0) {																				\n"
						"    READ_TEX(readtex1, uTex1, tcData1, uFbMonochrome[1], uFbFixedAlpha[1])								\n"
						"  } else readtex1 = readTexMS(uMSTex1, tcData1, uFbMonochrome[1], uFbFixedAlpha[1]);					\n";
				} else {
					shaderPart = "  lowp vec4 readtex1;																				\n"
								"  READ_TEX(readtex1, uTex1, tcData1, uFbMonochrome[1], uFbFixedAlpha[1])						\n";

				}
			}

		}

		shader << shaderPart;
	}

private:
	const opengl::GLInfo& m_glinfo;
};

class ShaderMipmap : public ShaderPart
{
public:
	ShaderMipmap(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			static const std::string strReadTex0 =
				"lowp vec4 TextureMipMap0(in sampler2D tex, in highp vec2 tcData[5])							\n"
				"{																								\n"
				"  mediump vec2 texSize = uTextureSize[0];														\n"
				"  lowp vec4 c00 = texture2D(tex, (tcData[0] + 0.5)/texSize);									\n"
				"  lowp vec4 c01 = texture2D(tex, (tcData[1] + 0.5)/texSize);									\n"
				"  lowp vec4 c10 = texture2D(tex, (tcData[2] + 0.5)/texSize);									\n"
				"  lowp vec4 c11 = texture2D(tex, (tcData[3] + 0.5)/texSize);									\n"
				;
			static const std::string strReadTex1 =
				"mediump float get_high4(in float byte) {														\n"
				"  return floor(byte/16.0);																		\n"
				"}																								\n"
				"mediump float get_low4(in float byte) {														\n"
				"  return byte - 16.0*floor(byte/16.0);															\n"
				"}																								\n"
				"lowp vec4 TextureMipMap1(in sampler2D tex, in highp vec2 tcData[5], in lowp float lod)			\n"
				"{																								\n"
				// Fetch from texture atlas
				// First 8 texels contain info about tile size and offset, 1 texel per tile
				"  mediump vec2 texSize = uTextureSize[1];															\n"
				"  mediump vec4 texWdthAndOff0 = 255.0 * texture2D(tex, vec2(0.5, 0.5)/texSize);					\n"
				"  mediump vec4 texWdthAndOff = 255.0 * texture2D(tex, vec2(lod + 0.5, 0.5)/texSize);				\n"
				"  mediump float lod_scales = pow(2.0, get_high4(texWdthAndOff0.a) - get_high4(texWdthAndOff.a));	\n"
				"  mediump float lod_scalet = pow(2.0, get_low4(texWdthAndOff0.a) - get_low4(texWdthAndOff.a));		\n"
				"  mediump vec2 lod_scale = vec2(lod_scales, lod_scalet);										\n"
				"  mediump float offset = texWdthAndOff.r + texWdthAndOff.g * 256.0;							\n"
				"  mediump float width = texWdthAndOff.b;														\n"
				"  mediump vec2 Coords00 = floor(tcData[0] * lod_scale);										\n"
				"  mediump float offset00 = offset + width * Coords00.t + Coords00.s;							\n"
				"  mediump float Y00 = floor(offset00 / mipmapTileWidth);										\n"
				"  lowp vec4 c00 = texture2D(tex, (vec2(offset00 - mipmapTileWidth * Y00, Y00) + 0.5)/texSize);	\n"
				"  mediump vec2 Coords01 = floor(tcData[1] * lod_scale);										\n"
				"  mediump float offset01 = offset + width * Coords01.t + Coords01.s;							\n"
				"  mediump float Y01 = floor(offset01 / mipmapTileWidth);										\n"
				"  lowp vec4 c01 = texture2D(tex, (vec2(offset01 - mipmapTileWidth * Y01, Y01) + 0.5)/texSize);	\n"
				"  mediump vec2 Coords10 = floor(tcData[2] * lod_scale);										\n"
				"  mediump float offset10 = offset + width * Coords10.t + Coords10.s;							\n"
				"  mediump float Y10 = floor(offset10 / mipmapTileWidth);										\n"
				"  lowp vec4 c10 = texture2D(tex, (vec2(offset10 - mipmapTileWidth * Y10, Y10) + 0.5)/texSize);	\n"
				"  mediump vec2 Coords11 = floor(tcData[3] * lod_scale);										\n"
				"  mediump float offset11 = offset + width * Coords11.t + Coords11.s;							\n"
				"  mediump float Y11 = floor(offset11 / mipmapTileWidth);										\n"
				"  lowp vec4 c11 = texture2D(tex, (vec2(offset11 - mipmapTileWidth * Y11, Y11) + 0.5)/texSize);	\n"
				;
			static const std::string strBillinear3PointEndGles2 =
				"  lowp vec4 c0 = c00 + tcData[4].s*(c10-c00) + tcData[4].t*(c01-c00);							\n"
				"  lowp vec4 c1 = c11 + (1.0-tcData[4].s)*(c01-c11) + (1.0-tcData[4].t)*(c10-c11);				\n"
				"  lowp float bottomRightTri = step(1.0, tcData[4].s + tcData[4].t);							\n"
				"  return c0 + bottomRightTri * (c1-c0);														\n"
				"  return c00;																					\n"
				"}																								\n"
				;
			static const std::string strBillinearStandardEndGles2 =
				"  lowp vec4 c0 = c00 + tcData[4].s * (c10-c00);												\n"
				"  lowp vec4 c1 = c01 + tcData[4].s * (c11-c01);												\n"
				"  return c0 + tcData[4].t * (c1-c0);															\n"
				"  return c00;																					\n"
				"}																								\n"
				;

			if (config.texture.bilinearMode == BILINEAR_3POINT) {
				m_part = strReadTex0;
				m_part += strBillinear3PointEndGles2;
				m_part += strReadTex1;
				m_part += strBillinear3PointEndGles2;
			} else {
				m_part = strReadTex0;
				m_part += strBillinearStandardEndGles2;
				m_part += strReadTex1;
				m_part += strBillinearStandardEndGles2;
			}

			m_part +=
				"uniform lowp int uEnableLod;												\n"
				"uniform mediump float uMinLod;												\n"
				"uniform lowp int uMaxTile;													\n"
				"uniform lowp int uNoAtlasTex;												\n"
				"uniform lowp int uTextureDetail;											\n"
				"																			\n"
				"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1) {		\n"
				;

			if (config.generalEmulation.enableLOD == 0) {
				m_part +=
					"  mediump float lod = 1.0;												\n"
					;
			} else {
				m_part +=
					"  mediump vec2 dx = abs(dFdx(vLodTexCoord)) * uScreenScale;			\n"
					"  mediump vec2 dy = abs(dFdy(vLodTexCoord)) * uScreenScale;			\n"
					"  mediump float lod = max(max(dx.x, dx.y), max(dy.x, dy.y));			\n"
					;
			}
			m_part +=
				"#define MIN(x, y) y < x ? y : x											\n"
				"#define MAX(x, y) x < y ? y : x											\n"
				"  lowp int lod_max_tile = uTextureDetail != 2 ? 7 : 6; 					\n"
				"  lowp int max_tile = MIN(lod_max_tile, uMaxTile); 						\n"
				"  mediump float min_lod = uTextureDetail != 0 ? uMinLod : 1.0;				\n"
				"  mediump float max_lod = pow(2.0, float(max_tile)) - 1.0 / 32.0;			\n"
				"  mediump float lod_clamp = min(max(lod, min_lod), max_lod);				\n"
	            // Simulate clamp function, needed for GLES 2.0 and integer types
				"  mediump int lod_clamp_int = int(log2(lod_clamp));						\n"
				"  mediump int lod_clamp_max = MAX(lod_clamp_int, 0);						\n"
				"  lowp int lod_tile = MIN(lod_clamp_max, max_tile);						\n"
				"  lowp int tile0 = 0;														\n"
				"  lowp int tile1 = 1;														\n"
				"  if (uEnableLod != 0) {													\n"
				"    if (lod_clamp < 1.0 && uTextureDetail == 0) {							\n"
				"      tile0 = 0;															\n"
				"      tile1 = 0;															\n"
				"    } else if (lod_clamp >= 1.0 && uTextureDetail == 2) {					\n"
				"      tile0 = lod_tile + 1;												\n"
				"      tile1 = lod_tile + 2;												\n"
				"    } else {																\n"
				"      tile0 = lod_tile;													\n"
				"      tile1 = lod_tile + 1;												\n"
				"    }																		\n"
				"  }																		\n"
				"  mediump float lod_frac = lod_clamp / pow(2.0, float(lod_tile));			\n"
				"  if (uTextureDetail == 1 || lod_clamp >= 1.0) {							\n"
				"    lod_frac = clamp(lod_frac - 1.0, -1.0, 1.0);							\n"
				"  }																		\n"
				"																			\n"
				"  if (tile0 == 0) readtex0 = TextureMipMap0(uTex0, tcData0);				\n"
				"  else if (uNoAtlasTex != 0) readtex0 = TextureMipMap0(uTex1, tcData1);	\n"
				"  else readtex0 = TextureMipMap1(uTex1, tcData1, float(tile0 - 1));		\n"
				"  if (tile1 == 0) readtex1 = TextureMipMap0(uTex0, tcData0);				\n"
				"  else if (uNoAtlasTex != 0) readtex1 = TextureMipMap0(uTex1, tcData1);	\n"
				"  else readtex1 = TextureMipMap1(uTex1, tcData1, float(tile1 - 1));		\n"
				"  return lod_frac;															\n"
				"}																			\n"
				;
		} else {
			static const std::string strReadTex0 =
				"#define READ_TEX0_MIPMAP(name, tex, tcData)											\\\n"
				"{																						\\\n"
				"  lowp vec4 c00 = texelFetch(tex, ivec2(tcData[0]), 0);								\\\n"
				"  lowp vec4 c01 = texelFetch(tex, ivec2(tcData[1]), 0);								\\\n"
				"  lowp vec4 c10 = texelFetch(tex, ivec2(tcData[2]), 0);								\\\n"
				"  lowp vec4 c11 = texelFetch(tex, ivec2(tcData[3]), 0);								\\\n"
				;
			static const std::string strReadTex1 =
				"#define GET_HIGH4(byte) floor(byte/16.0) \n"
				"#define GET_LOW4(byte) (byte - 16.0*floor(byte/16.0)) \n"
				"#define READ_TEX1_MIPMAP(name, tex, tcData, tile)										\\\n"
				"{																						\\\n"
				// Fetch from texture atlas
				// First 8 texels contain info about tile size and offset, 1 texel per tile
				"  mediump vec4 texWdthAndOff0 = 255.0 * texelFetch(tex, ivec2(0, 0), 0);				\\\n"
				"  mediump vec4 texWdthAndOff = 255.0 * texelFetch(tex, ivec2(int(tile), 0), 0);		\\\n"
				"  mediump float lod_scales = pow(2.0, GET_HIGH4(texWdthAndOff0.a) - GET_HIGH4(texWdthAndOff.a)); \\\n"
				"  mediump float lod_scalet = pow(2.0, GET_LOW4(texWdthAndOff0.a) - GET_LOW4(texWdthAndOff.a)); \\\n"
				"  mediump vec2 lod_scale = vec2(lod_scales, lod_scalet);								\\\n"
				"  mediump int offset = int(texWdthAndOff.r) + int(texWdthAndOff.g) * 256;				\\\n"
				"  mediump int width = int(texWdthAndOff.b);											\\\n"
				"  mediump ivec2 iCoords00 = ivec2(tcData[0] * lod_scale);								\\\n"
				"  mediump int offset00 = offset + width * iCoords00.t + iCoords00.s;					\\\n"
				"  mediump int Y00 = offset00/mipmapTileWidth;											\\\n"
				"  lowp vec4 c00 = texelFetch(tex, ivec2(offset00 - mipmapTileWidth * Y00, Y00), 0);	\\\n"
				"  mediump ivec2 iCoords01 = ivec2(tcData[1] * lod_scale);								\\\n"
				"  mediump int offset01 = offset + width * iCoords01.t + iCoords01.s;					\\\n"
				"  mediump int Y01 = offset01/mipmapTileWidth;											\\\n"
				"  lowp vec4 c01 = texelFetch(tex, ivec2(offset01 - mipmapTileWidth * Y01, Y01), 0);	\\\n"
				"  mediump ivec2 iCoords10 = ivec2(tcData[2] * lod_scale);								\\\n"
				"  mediump int offset10 = offset + width * iCoords10.t + iCoords10.s;					\\\n"
				"  mediump int Y10 = offset10/mipmapTileWidth;											\\\n"
				"  lowp vec4 c10 = texelFetch(tex, ivec2(offset10 - mipmapTileWidth * Y10, Y10), 0);	\\\n"
				"  mediump ivec2 iCoords11 = ivec2(tcData[3] * lod_scale);								\\\n"
				"  mediump int offset11 = offset + width * iCoords11.t + iCoords11.s;					\\\n"
				"  mediump int Y11 = offset11/mipmapTileWidth;											\\\n"
				"  lowp vec4 c11 = texelFetch(tex, ivec2(offset11 - mipmapTileWidth * Y11, Y11), 0);	\\\n"
				;
			static const std::string strBillinear3PointEnd =
				"  lowp vec4 c0 = c00 + tcData[4].s*(c10-c00) + tcData[4].t*(c01-c00);					\\\n"
				"  lowp vec4 c1 = c11 + (1.0-tcData[4].s)*(c01-c11) + (1.0-tcData[4].t)*(c10-c11);		\\\n"
				"  lowp float bottomRightTri = step(1.0, tcData[4].s + tcData[4].t);					\\\n"
				"  name = c0 + bottomRightTri * (c1-c0);												\\\n"
				"}																						\n"
				;
			static const std::string strBillinearStandardEnd =
				"  lowp vec4 c0 = c00 + tcData[4].s * (c10-c00);										\\\n"
				"  lowp vec4 c1 = c01 + tcData[4].s * (c11-c01);										\\\n"
				"  name = c0 + tcData[4].t * (c1-c0);													\\\n"
				"}																						\n"
				;
			if (config.texture.bilinearMode == BILINEAR_3POINT) {
				m_part = strReadTex0;
				m_part += strBillinear3PointEnd;
				m_part += strReadTex1;
				m_part += strBillinear3PointEnd;
			} else {
				m_part = strReadTex0;
				m_part += strBillinearStandardEnd;
				m_part += strReadTex1;
				m_part += strBillinearStandardEnd;
			}
			m_part +=
				"uniform lowp int uEnableLod;											\n"
				"uniform mediump float uMinLod;											\n"
				"uniform lowp int uMaxTile;												\n"
				"uniform lowp int uNoAtlasTex;											\n"
				"uniform lowp int uTextureDetail;										\n"
				"																		\n"
				"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1) {	\n"
				;
			if (config.generalEmulation.enableLOD == 0) {
				m_part +=
					"  mediump float lod = 1.0;											\n"
					;
			} else {
				m_part +=
					"  mediump vec2 dx = abs(dFdx(vLodTexCoord)) * uScreenScale;		\n"
					"  mediump vec2 dy = abs(dFdy(vLodTexCoord)) * uScreenScale;		\n"
					"  mediump float lod = max(max(dx.x, dx.y), max(dy.x, dy.y));		\n"
					;
			}
			m_part +=
				"  lowp int max_tile = min(uTextureDetail != 2 ? 7 : 6, uMaxTile);		\n"
				"  mediump float min_lod = uTextureDetail != 0 ? uMinLod : 1.0;			\n"
				"  mediump float max_lod = pow(2.0, float(max_tile)) - 1.0 / 32.0;		\n"
				"  mediump float lod_clamp = min(max(lod, min_lod), max_lod);			\n"
				"  lowp int lod_tile = clamp(int(log2(lod_clamp)), 0 , max_tile);		\n"
				"  lowp int tile0 = 0;													\n"
				"  lowp int tile1 = 1;													\n"
				"  if (uEnableLod != 0) {												\n"
				"    if (lod_clamp < 1.0 && uTextureDetail == 0) {						\n"
				"      tile0 = 0;														\n"
				"      tile1 = 0;														\n"
				"    } else if (lod_clamp >= 1.0 && uTextureDetail == 2) {				\n"
				"      tile0 = lod_tile + 1;											\n"
				"      tile1 = lod_tile + 2;											\n"
				"    } else {															\n"
				"      tile0 = lod_tile;												\n"
				"      tile1 = lod_tile + 1;											\n"
				"    }																	\n"
				"  }																	\n"
				"  mediump float lod_frac = lod_clamp / pow(2.0, float(lod_tile));		\n"
				"  if (uTextureDetail == 1 || lod_clamp >= 1.0) {						\n"
				"    lod_frac = clamp(lod_frac - 1.0, -1.0, 1.0);						\n"
				"  }																	\n"
				"																		\n"
				"  if(tile0 == 0) {READ_TEX0_MIPMAP(readtex0, uTex0, tcData0);}			\n"
				"  else if (uNoAtlasTex != 0) {READ_TEX0_MIPMAP(readtex0, uTex1, tcData1);}	\n"
				"  else {READ_TEX1_MIPMAP(readtex0, uTex1, tcData1, tile0 - 1);}		\n"
				"  if(tile1 == 0) {READ_TEX0_MIPMAP(readtex1, uTex0, tcData0);}			\n"
				"  else if (uNoAtlasTex != 0) {READ_TEX0_MIPMAP(readtex1, uTex1, tcData1);}	\n"
				"  else {READ_TEX1_MIPMAP(readtex1, uTex1, tcData1, tile1 - 1);}		\n"
				"  return lod_frac;														\n"
				"}																		\n"
				;
		}
	}
};

class ShaderReadtex : public ShaderPart
{
public:
	ShaderReadtex(const opengl::GLInfo & _glinfo) : m_glinfo(_glinfo)
	{
	}

	void write(std::stringstream & shader) const override
	{
		std::string shaderPart;

		if (m_glinfo.isGLES2) {
			shaderPart +=
				"lowp vec4 TextureNearest(in sampler2D tex, in highp vec2 tcData[5])		\n"
				"{																					\n"
				"  mediump vec2 texSize;															\n"
				"  if (nCurrentTile == 0)															\n"
				"    texSize = uTextureSize[0];														\n"
				"  else																				\n"
				"    texSize = uTextureSize[1];														\n"
				"  return texture2D(tex, (tcData[0] + 0.5) / texSize);								\n"
				"  }																				\n"
				;
			if (CombinerProgramBuilder::s_textureConvert.useYUVCoversion())
				shaderPart +=
				"lowp vec4 YUV_Convert(in sampler2D tex, in highp vec2 tcData[5], in lowp int convert, in lowp int format, in lowp vec4 prev)	\n"
				"{																	\n"
				"  lowp vec4 texColor;												\n"
				"  if (convert != 0) texColor = prev;								\n"
				"  else texColor = TextureNearest(tex, tcData);						\n"
				"  mediump ivec4 icolor = ivec4(texColor*255.0);					\n"
				"  if (format == 1)													\n"
				"    icolor.rg -= 128;												\n"
				"  mediump ivec4 iconvert;											\n"
				"  iconvert.r = icolor.b + (uConvertParams[0]*icolor.g + 128)/256;	\n"
				"  iconvert.g = icolor.b + (uConvertParams[1]*icolor.r + uConvertParams[2]*icolor.g + 128)/256;	\n"
				"  iconvert.b = icolor.b + (uConvertParams[3]*icolor.r + 128)/256;	\n"
				"  iconvert.a = icolor.b;											\n"
				"  return vec4(iconvert)/255.0;										\n"
				"  }																\n"
			;
			if (CombinerProgramBuilder::s_textureConvert.useTextureFiltering()) {
				if (config.texture.bilinearMode == BILINEAR_3POINT) {
					shaderPart +=
						// 3 point texture filtering.
						// Original author: ArthurCarvalho
						// GLSL implementation: twinaphex, mupen64plus-libretro project.
						"lowp vec4 TextureFilter(in sampler2D tex, in highp vec2 tcData[5])		\n"
						"{																					\n"
						"  mediump vec2 texSize;															\n"
						"  if (nCurrentTile == 0)															\n"
						"    texSize = uTextureSize[0];														\n"
						"  else																				\n"
						"    texSize = uTextureSize[1];														\n"
						"  lowp float bottomRightTri = step(1.0, tcData[4].s + tcData[4].t);				\n"
						"  lowp vec4 c00 = texture2D(tex, (tcData[0] + 0.5)/texSize);						\n"
						"  lowp vec4 c01 = texture2D(tex, (tcData[1] + 0.5)/texSize);						\n"
						"  lowp vec4 c10 = texture2D(tex, (tcData[2] + 0.5)/texSize);						\n"
						"  lowp vec4 c11 = texture2D(tex, (tcData[3] + 0.5)/texSize);						\n"
						"  lowp vec4 c0 = c00 + tcData[4].s*(c10-c00) + tcData[4].t*(c01-c00);				\n"
						"  lowp vec4 c1 = c11 + (1.0-tcData[4].s)*(c01-c11) + (1.0-tcData[4].t)*(c10-c11);	\n"
						"  return c0 + bottomRightTri * (c1-c0);											\n"
						"  }																				\n"
						;
				} else {
					shaderPart +=
						// bilinear filtering.
						"lowp vec4 TextureFilter(in sampler2D tex, in highp vec2 tcData[5])		\n"
						"{																					\n"
						"  mediump vec2 texSize;															\n"
						"  if (nCurrentTile == 0)															\n"
						"    texSize = uTextureSize[0];														\n"
						"  else																				\n"
						"    texSize = uTextureSize[1];														\n"
						"  lowp vec4 c00 = texture2D(tex, (tcData[0] + 0.5)/texSize);						\n"
						"  lowp vec4 c01 = texture2D(tex, (tcData[1] + 0.5)/texSize);						\n"
						"  lowp vec4 c10 = texture2D(tex, (tcData[2] + 0.5)/texSize);						\n"
						"  lowp vec4 c11 = texture2D(tex, (tcData[3] + 0.5)/texSize);						\n"
						"  lowp vec4 c0 = c00 + tcData[4].s * (c10-c00);									\n"
						"  lowp vec4 c1 = c01 + tcData[4].s * (c11-c01);									\n"
						"  return c0 + tcData[4].t * (c1-c0);												\n"
						"  }																				\n"
						;
				}
				shaderPart +=
					"lowp vec4 readTex(in sampler2D tex, in highp vec2 tcData[5], in lowp int fbMonochrome, in lowp int fbFixedAlpha)	\n"
					"{																			\n"
					"  lowp vec4 texColor;														\n"
					"  if (uTextureFilterMode == 0) texColor = TextureNearest(tex, tcData);		\n"
					"  else texColor = TextureFilter(tex, tcData);							\n"
					"  if (fbMonochrome == 1) texColor = vec4(texColor.r);						\n"
					"  else if (fbMonochrome == 2) 												\n"
					"    texColor.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), texColor.rgb));	\n"
					"  if (fbFixedAlpha == 1) texColor.a = 0.825;								\n"
					"  return texColor;															\n"
					"}																			\n"
					;
			}
		} else {
			if (config.video.multisampling > 0 && CombinerProgramBuilder::s_textureConvert.useTextureFiltering()) {
				shaderPart =
					"uniform lowp int uMSAASamples;												\n"
					"lowp vec4 sampleMS(in lowp sampler2DMS mstex, in mediump ivec2 ipos)		\n"
					"{																			\n"
					"  lowp vec4 texel = vec4(0.0);												\n"
					"  for (int i = 0; i < uMSAASamples; ++i)									\n"
					"    texel += texelFetch(mstex, ipos, i);									\n"
					"  return texel / float(uMSAASamples);										\n"
					"}																			\n"
					"																			\n"
					"lowp vec4 readTexMS(in lowp sampler2DMS mstex, in highp vec2 tcData[5], in lowp int fbMonochrome, in lowp int fbFixedAlpha)	\n"
					"{																			\n"
					"  mediump ivec2 itexCoord;													\n"
					"  if (fbMonochrome == 3) {													\n"
					"    itexCoord = ivec2(gl_FragCoord.xy);									\n"
					"  } else {																	\n"
					"    itexCoord = ivec2(tcData[0]);											\n"
					"  }																		\n"
					"  lowp vec4 texColor = sampleMS(mstex, itexCoord);							\n"
					"  if (fbMonochrome == 1) texColor = vec4(texColor.r);						\n"
					"  else if (fbMonochrome == 2) 												\n"
					"    texColor.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), texColor.rgb));	\n"
					"  else if (fbMonochrome == 3) { 											\n"
					"    texColor.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), texColor.rgb));	\n"
					"    texColor.a = 0.0;														\n"
					"  }																		\n"
					"  if (fbFixedAlpha == 1) texColor.a = 0.825;								\n"
					"  return texColor;															\n"
					"}																			\n"
				;
			}
		}

		shader << shaderPart;
	}

private:
	const opengl::GLInfo& m_glinfo;
};

class ShaderReadtexCopyMode : public ShaderPart
{
public:
	ShaderReadtexCopyMode(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			m_part =
				"lowp vec4 TextureNearest(in sampler2D tex, in highp vec2 tcData[5])		\n"
				"{																					\n"
				"  mediump vec2 texSize;															\n"
				"  if (nCurrentTile == 0)															\n"
				"    texSize = uTextureSize[0];														\n"
				"  else																				\n"
				"    texSize = uTextureSize[1];														\n"
				"  return texture2D(tex, (tcData[0] + 0.5) / texSize);								\n"
				"  }																				\n"
				"lowp vec4 readTex(in sampler2D tex, in highp vec2 tcData[5], in lowp int fbMonochrome, in lowp int fbFixedAlpha)	\n"
				"{																			\n"
				"  lowp vec4 texColor = TextureNearest(tex, tcData);						\n"
				"  if (fbMonochrome == 1) texColor = vec4(texColor.r);						\n"
				"  else if (fbMonochrome == 2) 												\n"
				"    texColor.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), texColor.rgb));	\n"
				"  if (fbFixedAlpha == 1) texColor.a = 0.825;								\n"
				"  return texColor;															\n"
				"}																			\n"
				;
		} else {
			if (config.video.multisampling > 0) {
				m_part =
					"uniform lowp int uMSAASamples;												\n"
					"lowp vec4 sampleMS(in lowp sampler2DMS mstex, in mediump ivec2 ipos)		\n"
					"{																			\n"
					"  lowp vec4 texel = vec4(0.0);												\n"
					"  for (int i = 0; i < uMSAASamples; ++i)									\n"
					"    texel += texelFetch(mstex, ipos, i);									\n"
					"  return texel / float(uMSAASamples);										\n"
					"}																			\n"
					"																			\n"
					"lowp vec4 readTexMS(in lowp sampler2DMS mstex, in highp vec2 tcData[5], in lowp int fbMonochrome, in lowp int fbFixedAlpha)	\n"
					"{																			\n"
					"  mediump ivec2 itexCoord;													\n"
					"  if (fbMonochrome == 3) {													\n"
					"    itexCoord = ivec2(gl_FragCoord.xy);									\n"
					"  } else {																	\n"
					"    itexCoord = ivec2(tcData[0]);											\n"
					"  }																		\n"
					"  lowp vec4 texColor = sampleMS(mstex, itexCoord);							\n"
					"  if (fbMonochrome == 1) texColor = vec4(texColor.r);						\n"
					"  else if (fbMonochrome == 2) 												\n"
					"    texColor.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), texColor.rgb));	\n"
					"  else if (fbMonochrome == 3) { 											\n"
					"    texColor.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), texColor.rgb));	\n"
					"    texColor.a = 0.0;														\n"
					"  }																		\n"
					"  if (fbFixedAlpha == 1) texColor.a = 0.825;								\n"
					"  return texColor;															\n"
					"}																			\n"
				;
			}
		}
	}
};

class ShaderTextureEngine : public ShaderPart
{
public:
	ShaderTextureEngine(const opengl::GLInfo _glinfo) {
		m_part =
			"highp vec2 clampWrapMirror(in highp vec2 vTexCoord, in highp vec2 vWrap,		\n"
			"	in highp vec2 vClamp, in lowp vec2 vWrapEn, in lowp vec2 vClampEn, in lowp vec2 vMirrorEn)		\n"
			"{																				\n"
			"	highp vec2 texCoord = vTexCoord;											\n"
			"	highp vec2 clampedCoord = clamp(texCoord, vec2(0.0), vClamp);				\n"
			"	texCoord += vClampEn*(clampedCoord-texCoord);								\n"
			"   lowp vec2 needMirror = step(vWrap, mod(texCoord, 2.0*vWrap));				\n"
			"	highp vec2 invertedCoord = mod(-texCoord-vec2(1.0), vWrap);	 				\n"
			"	texCoord += vMirrorEn*needMirror*(invertedCoord-texCoord);	 				\n"
			"   highp vec2 wrappedCoord = mod(texCoord,vWrap);								\n"
			"	texCoord += vWrapEn*(wrappedCoord-texCoord);								\n"
			"	return texCoord;															\n"
			"}																				\n"

			"highp vec2 wrap2D(in highp vec2 tc, in highp vec2 size)						\n"
			"{																				\n"
			"  highp float divs = floor(tc.s / size.s);										\n"
			"  highp float divt = floor((tc.t + divs) / size.t);							\n"
			"  return vec2(tc.s - divs * size.s, tc.t + divs - divt*size.t);				\n"
			"}																				\n"

			"void textureEngine0(in highp vec2 texCoord, out highp vec2 tcData[5]) \n"
			"{  \n"
			"  highp vec2 tileCoord = (WRAP(texCoord * uShiftScale[0] - uTexOffset[0], -1024.0, 1024.0));\n"
			"  tileCoord = (tileCoord + uBilinearOffset) * uHDRatio[0] - uBilinearOffset; \n"
			"  mediump vec2 intPart = floor(tileCoord); \n"
			"  highp vec2 tc00 = clampWrapMirror(intPart, uTexWrap[0], uTexClamp[0], uTexWrapEn[0], uTexClampEn[0], uTexMirrorEn[0]); \n"
			"  highp vec2 tc11 = clampWrapMirror(intPart + vec2(1.0,1.0), uTexWrap[0], uTexClamp[0], uTexWrapEn[0], uTexClampEn[0], uTexMirrorEn[0]); \n"
			"  tcData[0] = wrap2D(tc00, uTexSize[0]) + uCacheOffset[0]; \n"
			"  tcData[3] = wrap2D(tc11, uTexSize[0]) + uCacheOffset[0]; \n"
			"  tcData[1] = vec2(tcData[0].s, tcData[3].t); \n"
			"  tcData[2] = vec2(tcData[3].s, tcData[0].t); \n"
			"  tcData[4] = tileCoord - intPart; \n"
			"}  \n"

			"void textureEngine1(in highp vec2 texCoord, out highp vec2 tcData[5]) \n"
			"{  \n"
			"  highp vec2 tileCoord = (WRAP(texCoord * uShiftScale[1] - uTexOffset[1], -1024.0, 1024.0)); \n"
			"  tileCoord = (tileCoord + uBilinearOffset) * uHDRatio[1] - uBilinearOffset; \n"
			"  mediump vec2 intPart = floor(tileCoord); \n"
			"  highp vec2 tc00 = clampWrapMirror(intPart, uTexWrap[1], uTexClamp[1], uTexWrapEn[1], uTexClampEn[1], uTexMirrorEn[1]); \n"
			"  highp vec2 tc11 = clampWrapMirror(intPart + vec2(1.0,1.0), uTexWrap[1], uTexClamp[1], uTexWrapEn[1], uTexClampEn[1], uTexMirrorEn[1]); \n"
			"  tcData[0] = wrap2D(tc00, uTexSize[1]) + uCacheOffset[1]; \n"
			"  tcData[3] = wrap2D(tc11, uTexSize[1]) + uCacheOffset[1]; \n"
			"  tcData[1] = vec2(tcData[0].s, tcData[3].t); \n"
			"  tcData[2] = vec2(tcData[3].s, tcData[0].t); \n"
			"  tcData[4] = tileCoord - intPart; \n"
			"}  \n"

			;
	}
};

class ShaderFragmentTextureEngineTex0 : public ShaderPart {
public:
	ShaderFragmentTextureEngineTex0(const opengl::GLInfo _glinfo)
	{
		m_part =
			"textureEngine0(mTexCoord, tcData0); \n"
			;
	}
};

class ShaderFragmentTextureEngineTex1 : public ShaderPart {
public:
	ShaderFragmentTextureEngineTex1(const opengl::GLInfo _glinfo)
	{
		m_part =
			"textureEngine1(mTexCoord, tcData1); \n"
			;
	}
};

} // nameless namespace

namespace glsl {

CombinerProgramBuilderAccurate::CombinerProgramBuilderAccurate(const opengl::GLInfo & _glinfo, opengl::CachedUseProgram * _useProgram)
: CombinerProgramBuilderCommon(_glinfo, _useProgram, std::make_unique<CombinerProgramUniformFactoryAccurate>(_glinfo))
, m_vertexTexturedTriangle(new VertexShaderTexturedTriangle(_glinfo))
, m_vertexTexturedRect(new VertexShaderTexturedRect(_glinfo))
, m_fragmentCorrectTexCoords(new ShaderFragmentCorrectTexCoords())
, m_fragmentGlobalVariablesTex(new ShaderFragmentGlobalVariablesTex(_glinfo))
, m_fragmentHeaderTextureEngine(new ShaderFragmentHeaderTextureEngine(_glinfo))
, m_fragmentHeaderReadMSTex(new ShaderFragmentHeaderReadMSTex(_glinfo))
, m_fragmentHeaderReadTex(new ShaderFragmentHeaderReadTex(_glinfo))
, m_fragmentHeaderReadTexCopyMode(new ShaderFragmentHeaderReadTexCopyMode(_glinfo))
, m_fragmentReadTex0(new ShaderFragmentReadTex0(_glinfo))
, m_fragmentReadTex1(new ShaderFragmentReadTex1(_glinfo))
, m_fragmentTextureEngineTex0(new ShaderFragmentTextureEngineTex0(_glinfo))
, m_fragmentTextureEngineTex1(new ShaderFragmentTextureEngineTex1(_glinfo))
, m_fragmentReadTexCopyMode(new ShaderFragmentReadTexCopyMode(_glinfo))
, m_shaderMipmap(new ShaderMipmap(_glinfo))
, m_shaderReadtex(new ShaderReadtex(_glinfo))
, m_shaderReadtexCopyMode(new ShaderReadtexCopyMode(_glinfo))
, m_shaderTextureEngine(new ShaderTextureEngine(_glinfo))
{
}

const ShaderPart * CombinerProgramBuilderAccurate::getVertexShaderTexturedRect() const
{
	return m_vertexTexturedRect.get();
}

const ShaderPart * CombinerProgramBuilderAccurate::getVertexShaderTexturedTriangle() const
{
	return m_vertexTexturedTriangle.get();
}

void CombinerProgramBuilderAccurate::_writeFragmentCorrectTexCoords(std::stringstream& ssShader)const
{
	m_fragmentCorrectTexCoords->write(ssShader);
}

void CombinerProgramBuilderAccurate::_writeFragmentGlobalVariablesTex(std::stringstream& ssShader) const
{
	m_fragmentGlobalVariablesTex->write(ssShader);
}

void CombinerProgramBuilderAccurate::_writeFragmentHeaderReadMSTex(std::stringstream& ssShader) const
{
	m_fragmentHeaderReadMSTex->write(ssShader);
}

void CombinerProgramBuilderAccurate::_writeFragmentHeaderReadTex(std::stringstream& ssShader) const
{
	m_fragmentHeaderReadTex->write(ssShader);
}

void CombinerProgramBuilderAccurate::_writeFragmentHeaderReadTexCopyMode(std::stringstream& ssShader) const
{
	m_fragmentHeaderReadTexCopyMode->write(ssShader);
}

void CombinerProgramBuilderAccurate::_writeFragmentHeaderClampWrapMirrorEngine(std::stringstream& ssShader) const
{
	m_fragmentHeaderTextureEngine->write(ssShader);
}

void CombinerProgramBuilderAccurate::_writeFragmentClampWrapMirrorEngineTex0(std::stringstream& ssShader) const
{
	m_fragmentTextureEngineTex0->write(ssShader);
}

void CombinerProgramBuilderAccurate::_writeFragmentClampWrapMirrorEngineTex1(std::stringstream& ssShader) const
{
	m_fragmentTextureEngineTex1->write(ssShader);
}

void CombinerProgramBuilderAccurate::_writeFragmentReadTex0(std::stringstream& ssShader) const
{
	m_fragmentReadTex0->write(ssShader);
}

void CombinerProgramBuilderAccurate::_writeFragmentReadTex1(std::stringstream& ssShader) const
{
	m_fragmentReadTex1->write(ssShader);
}

void CombinerProgramBuilderAccurate::_writeFragmentReadTexCopyMode(std::stringstream& ssShader) const
{
	m_fragmentReadTexCopyMode->write(ssShader);
}

void CombinerProgramBuilderAccurate::_writeShaderClampWrapMirrorEngine(std::stringstream& ssShader) const
{
	m_shaderTextureEngine->write(ssShader);
}

void CombinerProgramBuilderAccurate::_writeShaderMipmap(std::stringstream& ssShader) const
{
	m_shaderMipmap->write(ssShader);
}

void CombinerProgramBuilderAccurate::_writeShaderReadtex(std::stringstream& ssShader) const
{
	m_shaderReadtex->write(ssShader);
}

void CombinerProgramBuilderAccurate::_writeShaderReadtexCopyMode(std::stringstream& ssShader) const
{
	m_shaderReadtexCopyMode->write(ssShader);
}

}
