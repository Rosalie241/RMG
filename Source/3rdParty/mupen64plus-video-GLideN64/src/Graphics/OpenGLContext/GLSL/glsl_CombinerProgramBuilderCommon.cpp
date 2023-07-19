#include "glsl_CombinerProgramBuilderCommon.h"

#include <iomanip> // for setprecision
#include <assert.h>
#include <Log.h>
#include <Config.h>
#include "glsl_Utils.h"
#include "glsl_CombinerInputs.h"
#include "glsl_CombinerProgramImpl.h"
#include "glsl_CombinerProgramUniformFactory.h"
#include "GraphicsDrawer.h"

namespace {
using namespace glsl;

/*---------------ShaderParts-------------*/

class VertexShaderHeader : public ShaderPart
{
public:
	VertexShaderHeader(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			m_part = "#version 100			\n";
			m_part +=
				"#if (__VERSION__ > 120)	\n"
				"# define IN in				\n"
				"# define OUT out			\n"
				"#else						\n"
				"# define IN attribute		\n"
				"# define OUT varying		\n"
	 			"#ifndef GL_FRAGMENT_PRECISION_HIGH \n"
	 			"# define highp mediump		\n"
	 			"#endif						\n"
				"#endif // __VERSION		\n"
				;
		}
		else if (_glinfo.isGLESX) {
			std::stringstream ss;
			ss << "#version " << Utils::to_string(_glinfo.majorVersion) << Utils::to_string(_glinfo.minorVersion) << "0 es " << std::endl;
			ss << "# define IN in" << std::endl << "# define OUT out" << std::endl;
			if (_glinfo.noPerspective) {
				ss << "#extension GL_NV_shader_noperspective_interpolation : enable" << std::endl
					<< "noperspective OUT highp float vZCoord;" << std::endl
					<< "uniform lowp int uClampMode;" << std::endl;
			}
			m_part = ss.str();
		}
		else {
			std::stringstream ss;
			ss << "#version " << Utils::to_string(_glinfo.majorVersion) << Utils::to_string(_glinfo.minorVersion) << "0 core " << std::endl;
			ss << "# define IN in" << std::endl << "# define OUT out" << std::endl;
			m_part = ss.str();
		}
		m_part += "uniform lowp vec2 uVertexOffset; \n";
		std::stringstream ss;
		ss << "const lowp float screenSizeDims = " << std::setprecision(1) << std::fixed << SCREEN_SIZE_DIM << ";" << std::endl;
		m_part += ss.str();
	}
};

class VertexShaderTriangle : public ShaderPart
{
public:
	VertexShaderTriangle(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"IN highp vec4 aPosition;										\n"
			"IN lowp vec4 aColor;											\n"
			"IN lowp float aNumLights;										\n"
			"IN highp vec4 aModify;											\n"
			"IN highp vec2 aBaryCoords;										\n"
			"																\n"
			"uniform lowp int uFogUsage;									\n"
			"uniform mediump vec2 uFogScale;								\n"
			"uniform mediump vec2 uScreenCoordsScale;						\n"
			"uniform mediump vec2 uVTrans;									\n"
			"uniform mediump vec2 uVScale;									\n"
			"uniform mediump vec2 uAdjustTrans;								\n"
			"uniform mediump vec2 uAdjustScale;								\n"
			"																\n"
			"OUT lowp float vNumLights;										\n"
			"OUT lowp vec4 vShadeColor;										\n"
			"OUT highp vec4 vBaryCoords;									\n"
		;
		if (!_glinfo.isGLESX || _glinfo.noPerspective)
			m_part += "noperspective OUT lowp vec4 vShadeColorNoperspective;\n";
		else
			m_part += "OUT lowp vec4 vShadeColorNoperspective;				\n";
		m_part +=
			"																\n"
			"void main()													\n"
			"{																\n"
			"  gl_Position = aPosition;										\n"
			"  vShadeColor = aColor;										\n"
			"  vNumLights = aNumLights;										\n"
			"  if ((aModify[0]) != 0.0) {									\n"
			"    gl_Position.xy *= gl_Position.w;							\n"
			"  }															\n"
			"  else {														\n"
			"    gl_Position.xy = gl_Position.xy * uVScale.xy + uVTrans.xy * gl_Position.ww; \n"
			"    gl_Position.xy = floor(gl_Position.xy * vec2(4.0)) * vec2(0.25); \n"
			"    gl_Position.xy = gl_Position.xy * uAdjustScale + gl_Position.ww * uAdjustTrans; \n"
			"  }															\n"
			"  if ((aModify[1]) != 0.0) 									\n"
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
			"  vBaryCoords = vec4(aBaryCoords, 1.0 - aBaryCoords.x - aBaryCoords.y, 0.5); \n"
			"  vShadeColorNoperspective = vShadeColor;							\n"
			;
	}
};

class VertexShaderRect : public ShaderPart
{
public:
	VertexShaderRect(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"IN highp vec4 aRectPosition;						\n"
			"IN highp vec2 aBaryCoords;							\n"
			"													\n"
			"OUT lowp vec4 vShadeColor;							\n"
			"OUT highp vec4 vBaryCoords; \n"
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
			"  vBaryCoords = vec4(aBaryCoords, vec2(1.0) - aBaryCoords);\n"
			;
	}
};

class VertexShaderEnd : public ShaderPart
{
public:
	VertexShaderEnd(const opengl::GLInfo & _glinfo)
	{
		if (!_glinfo.isGLESX) {
			m_part =
				"  gl_ClipDistance[0] = gl_Position.w - gl_Position.z;	\n"
				;
		} else if (config.generalEmulation.enableFragmentDepthWrite != 0 && _glinfo.noPerspective) {
			m_part =
				"  vZCoord = gl_Position.z / gl_Position.w;	\n"
				"  if (uClampMode > 0)	\n"
				"    gl_Position.z = 0.0;	\n"
				;
		} else if (config.generalEmulation.enableClipping != 0) {
			// Move the near plane towards the camera.
			// It helps to avoid issues with near-plane clipping in games, which do not use it.
			// Z must be scaled back in fragment shader.
			m_part = "  gl_Position.z /= 8.0;	\n";
		}
		m_part +=
			" gl_Position.xy += uVertexOffset * vec2(gl_Position.w);		\n"
			" gl_Position.xy -= vec2(0.5*screenSizeDims) * gl_Position.ww;	\n"
			" gl_Position.xy /= vec2(0.5*screenSizeDims);					\n"
			"} \n"
			;
	}
};

class FragmentShaderHeader : public ShaderPart
{
public:
	FragmentShaderHeader(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			m_part = "#version 100 \n";
			if (config.generalEmulation.enableLOD) {
				m_part += "#extension GL_EXT_shader_texture_lod : enable \n";
				m_part += "#extension GL_OES_standard_derivatives : enable \n";
			}
			m_part +=
				"#if (__VERSION__ > 120)		\n"
				"# define IN in					\n"
				"# define OUT out				\n"
				"# define texture2D texture		\n"
				"#else							\n"
				"# define IN varying			\n"
				"# define OUT					\n"
				"#ifndef GL_FRAGMENT_PRECISION_HIGH \n"
				"# define highp mediump		\n"
				"#endif						\n"
				"#endif // __VERSION __			\n"
			;
		} else if (_glinfo.isGLESX) {
			std::stringstream ss;
			ss << "#version " << Utils::to_string(_glinfo.majorVersion) << Utils::to_string(_glinfo.minorVersion) << "0 es " << std::endl;
			if (_glinfo.noPerspective)
				ss << "#extension GL_NV_shader_noperspective_interpolation : enable" << std::endl;
			if (_glinfo.dual_source_blending)
				ss << "#extension GL_EXT_blend_func_extended : enable" << std::endl;
			if (_glinfo.ext_fetch)
				ss << "#extension GL_EXT_shader_framebuffer_fetch : enable" << std::endl;
			if (_glinfo.ext_fetch_arm)
				ss << "#extension GL_ARM_shader_framebuffer_fetch : enable" << std::endl;

			if (config.frameBufferEmulation.N64DepthCompare == Config::dcFast) {
				if (_glinfo.imageTextures && _glinfo.fragment_interlockNV) {
					ss << "#extension GL_NV_fragment_shader_interlock : enable" << std::endl
						<< "layout(pixel_interlock_ordered) in;" << std::endl;
				}
			} else if (_glinfo.fetch_depth)
				ss << "#extension GL_ARM_shader_framebuffer_fetch_depth_stencil : enable" << std::endl;

			ss << "# define IN in" << std::endl
				<< "# define OUT out" << std::endl
				<< "# define texture2D texture" << std::endl;
			m_part = ss.str();
		} else {
			std::stringstream ss;
			ss << "#version " << Utils::to_string(_glinfo.majorVersion) << Utils::to_string(_glinfo.minorVersion) << "0 core " << std::endl;
			if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
				if (_glinfo.n64DepthWithFbFetch)
					ss << "#extension GL_EXT_shader_framebuffer_fetch : enable" << std::endl;
				else if (_glinfo.imageTextures) {
					if (_glinfo.majorVersion * 10 + _glinfo.minorVersion < 42) {
						ss << "#extension GL_ARB_shader_image_load_store : enable" << std::endl
							<< "#extension GL_ARB_shading_language_420pack : enable" << std::endl;
					}
					if (_glinfo.fragment_interlock)
						ss << "#extension GL_ARB_fragment_shader_interlock : enable" << std::endl
							<< "layout(pixel_interlock_ordered) in;" << std::endl;
					else if (_glinfo.fragment_interlockNV)
						ss << "#extension GL_NV_fragment_shader_interlock : enable" << std::endl
							<< "layout(pixel_interlock_ordered) in;" << std::endl;
					else if (_glinfo.fragment_ordering)
						ss << "#extension GL_INTEL_fragment_shader_ordering : enable" << std::endl;
				}
			}

			ss << "# define IN in" << std::endl
				<< "# define OUT out" << std::endl
				<< "# define texture2D texture" << std::endl;
			m_part = ss.str();
		}
		m_part +=
			// Return the vector of the standard basis of R^4 with a 1 at position <pos> and 0 otherwise.
			"  #define STVEC(pos) (step(float(pos), vec4(0.5,1.5,2.5,3.5)) - step(float(pos), vec4(-0.5,0.5,1.5,2.5))) \n";

		std::stringstream ss;
		if (_glinfo.isGLES2)
			ss << "const mediump float mipmapTileWidth = " << std::setprecision(1) << std::fixed << f32(MIPMAP_TILE_WIDTH) << ";" << std::endl;
		else
			ss << "const mediump int mipmapTileWidth = " << MIPMAP_TILE_WIDTH << ";" << std::endl;
		m_part += ss.str();
	}
};

class ShaderBlender1 : public ShaderPart
{
public:
	ShaderBlender1(const opengl::GLInfo & _glinfo)
	{
#if 1
		m_part =
			"  srcColor1 = vec4(0.0);									\n"
			"  dstFactor1 = 0.0;										\n"
			"  muxPM[0] = clampedColor;									\n"
			"  muxA[0] = clampedColor.a;								\n"
			"  muxa = MUXA(uBlendMux1[1]);								\n"
			"  muxB[0] = 1.0 - muxa;									\n"
			"  muxb = MUXB(uBlendMux1[3]);								\n"
			"  muxp = MUXPM(uBlendMux1[0]);								\n"
			"  muxm = MUXPM(uBlendMux1[2]);								\n"
			"  muxaf = MUXF(uBlendMux1[0]);								\n"
			"  muxbf = MUXF(uBlendMux1[2]);								\n"
			"  if (uForceBlendCycle1 != 0) {							\n"
			"    srcColor1 = muxp * muxa + muxm * muxb;					\n"
			"    dstFactor1 = muxaf * muxa + muxbf * muxb;				\n"
			"    srcColor1 = clamp(srcColor1, 0.0, 1.0);				\n"
			"  } else {													\n"
			"    srcColor1 = muxp;										\n"
			"    dstFactor1 = muxaf;									\n"
			"  }														\n"
			"  fragColor = srcColor1;	\n"
			"  fragColor1 = vec4(dstFactor1);							\n"
			;
#else
		// Keep old code for reference
		m_part =
			"  muxPM[0] = clampedColor;								\n"
			"  if (uForceBlendCycle1 != 0) {						\n"
			"    muxA[0] = clampedColor.a;							\n"
			"    muxB[0] = 1.0 - muxA[uBlendMux1[1]];				\n"
			"    lowp vec4 blend1 = (muxPM[uBlendMux1[0]] * muxA[uBlendMux1[1]]) + (muxPM[uBlendMux1[2]] * muxB[uBlendMux1[3]]);	\n"
			"    clampedColor.rgb = clamp(blend1.rgb, 0.0, 1.0);	\n"
			"  } else clampedColor.rgb = muxPM[uBlendMux1[0]].rgb;	\n"
			;
#endif
	}
};

class ShaderBlender2 : public ShaderPart
{
public:
	ShaderBlender2(const opengl::GLInfo & _glinfo)
	{
#if 1
		m_part =
			"  srcColor2 = vec4(0.0);									\n"
			"  dstFactor2 = 0.0;										\n"
			"  muxPM[0] = srcColor1;									\n"
			"  muxa = MUXA(uBlendMux2[1]);								\n"
			"  muxB[0] = 1.0 - muxa;									\n"
			"  muxb = MUXB(uBlendMux2[3]);								\n"
			"  muxp = MUXPM(uBlendMux2[0]);								\n"
			"  muxm = MUXPM(uBlendMux2[2]);								\n"
			"  muxF[0] = dstFactor1;									\n"
			"  muxaf = MUXF(uBlendMux2[0]);								\n"
			"  muxbf = MUXF(uBlendMux2[2]);								\n"
			"  if (uForceBlendCycle2 != 0) {							\n"
			"    srcColor2 = muxp * muxa + muxm * muxb;					\n"
			"    dstFactor2 = muxaf * muxa + muxbf * muxb;				\n"
			"    srcColor2 = clamp(srcColor2, 0.0, 1.0);				\n"
			"  } else {													\n"
			"    srcColor2 = muxp;										\n"
			"    dstFactor2 = muxaf;									\n"
			"  }														\n"
			"  fragColor = srcColor2;	\n"
			"  fragColor1 = vec4(dstFactor2);							\n"
			;

#else
		// Keep old code for reference
		m_part =
			"  muxPM[0] = clampedColor;								\n"
			"  muxPM[1] = vec4(0.0);								\n"
			"  if (uForceBlendCycle2 != 0) {						\n"
			"    muxA[0] = clampedColor.a;							\n"
			"    muxB[0] = 1.0 - muxA[uBlendMux2[1]];				\n"
			"    lowp vec4 blend2 = muxPM[uBlendMux2[0]] * muxA[uBlendMux2[1]] + muxPM[uBlendMux2[2]] * muxB[uBlendMux2[3]];	\n"
			"    clampedColor.rgb = clamp(blend2.rgb, 0.0, 1.0);	\n"
			"  } else clampedColor.rgb = muxPM[uBlendMux2[0]].rgb;	\n"
			;
#endif
	}
};

class ShaderBlenderAlpha : public ShaderPart
{
public:
	ShaderBlenderAlpha(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.dual_source_blending || _glinfo.ext_fetch || _glinfo.ext_fetch_arm) {
			m_part +=
				"if (uBlendAlphaMode != 2) {							\n"
				"  lowp vec4 srcAlpha = vec4(cvg, cvg, 1.0, 0.0);		\n"
				"  lowp vec4 dstFactorAlpha = vec4(1.0, 1.0, 0.0, 1.0);	\n"
				"  if (uBlendAlphaMode == 0)							\n"
				"    dstFactorAlpha[0] = 0.0;							\n"
				"  fragColor1.a = dstFactorAlpha[uCvgDest];				\n"
				"  fragColor.a = srcAlpha[uCvgDest] + lastFragColor.a * fragColor1.a;\n"
				"} else fragColor.a = clampedColor.a;					\n";
		}
		else {
			m_part +=
				"fragColor.a = clampedColor.a;"
				;
		}
	}
};

class ShaderLegacyBlender : public ShaderPart
{
public:
	ShaderLegacyBlender()
	{
		m_part =
			"  if (uFogUsage == 1) \n"
			"    fragColor.rgb = mix(fragColor.rgb, uFogColor.rgb, vShadeColor.a); \n"
			;
	}
};


/*
// N64 color wrap and clamp on floats
// See https://github.com/gonetz/GLideN64/issues/661 for reference
if (c < -1.0) return c + 2.0;

if (c < -0.5) return 1;

if (c < 0.0) return 0;

if (c > 2.0) return c - 2.0;

if (c > 1.5) return 0;

if (c > 1.0) return 1;

return c;
*/
class ShaderClamp : public ShaderPart
{
public:
	ShaderClamp()
	{
		m_part =
			"  lowp vec4 wrappedColor = WRAP(cmbRes, -0.51, 1.51); \n"
			"  lowp vec4 clampedColor = clamp(wrappedColor, 0.0, 1.0); \n"
			;
	}
};

/*
N64 sign-extension for C component of combiner
if (c > 1.0)
return c - 2.0;

return c;
*/
class ShaderSignExtendColorC : public ShaderPart
{
public:
	ShaderSignExtendColorC()
	{
		m_part =
			" color1 = WRAP(color1, -1.01, 1.01); \n"
			;
	}
};

class ShaderSignExtendAlphaC : public ShaderPart
{
public:
	ShaderSignExtendAlphaC()
	{
		m_part =
			" alpha1 = WRAP(alpha1, -1.01, 1.01); \n"
			;
	}
};

/*
N64 sign-extension for ABD components of combiner
if (c > 1.5)
return c - 2.0;

if (c < -0.5)
return c + 2.0;

return c;
*/
class ShaderSignExtendColorABD : public ShaderPart
{
public:
	ShaderSignExtendColorABD()
	{
		m_part =
			" color1 = WRAP(color1, -0.51, 1.51); \n"
			;
	}
};

class ShaderSignExtendAlphaABD : public ShaderPart
{
public:
	ShaderSignExtendAlphaABD()
	{
		m_part =
			"  alpha1 = WRAP(alpha1, -0.51,1.51); \n"
			;
	}
};

class ShaderAlphaTest : public ShaderPart
{
public:
	ShaderAlphaTest()
	{
		m_part =
			"  if (uEnableAlphaTest != 0) {							\n"
			"    lowp float alphaTestValue = (uAlphaCompareMode == 3) ? snoise() : uAlphaTestValue;	\n"
			"    lowp float alphaValue;								\n"
			"    if ((uAlphaCvgSel != 0) && (uCvgXAlpha == 0)) {	\n"
			"      alphaValue = 0.125;								\n"
			"    } else {											\n"
			"      alphaValue = clamp(alpha1, 0.0, 1.0);			\n"
			"    }													\n"
			"    if (alphaValue < alphaTestValue) discard;			\n"
			"  }													\n"
			;
	}
};

class ShaderDithering : public ShaderPart
{
public:
	ShaderDithering(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2)
			return;

		if (config.generalEmulation.enableDitheringPattern == 0) {
			m_part =
				"  if (uColorDitherMode == 2) {											\n"
				"    colorDither(snoiseRGB(), clampedColor.rgb);						\n"
				"  }																	\n"
				"  if (uAlphaDitherMode == 2) {											\n"
				"    alphaDither(snoiseA(), clampedColor.a);							\n"
				"  }																	\n"
				;
			return;
		}

		m_part +=
			"  lowp mat4 bayer = mat4( 0.0, 0.5, 0.125, 0.625,							\n"
			"                          0.5, 0.0, 0.625, 0.125,							\n"
			"                          0.375, 0.875, 0.25, 0.75,						\n"
			"                          0.875, 0.375, 0.75, 0.25);						\n"
			"  lowp mat4 mSquare = mat4( 0.0, 0.75, 0.125, 0.875,						\n"
			"                            0.5, 0.25, 0.625, 0.375, 						\n"
			"                            0.375, 0.625, 0.25, 0.5,						\n"
			"                            0.875, 0.125, 0.75, 0.0);						\n"
			// Try to keep dithering visible even at higher resolutions
			"  lowp float divider = 1.0 + step(3.0, uScreenScale.x);					\n"
			"  mediump ivec2 position = ivec2(mod((gl_FragCoord.xy - 0.5) / divider,4.0));\n"
			"  lowp vec4 posX = STVEC(position.x);										\n"
			"  lowp vec4 posY = STVEC(position.y);										\n"
			"  lowp float bayerThreshold = dot(bayer*posY, posX);						\n"
			"  lowp float mSquareThreshold = dot(mSquare*posY, posX);					\n"
			"  switch (uColorDitherMode) {												\n"
			"     case 0:																\n"
			"       colorDither(vec3(mSquareThreshold), clampedColor.rgb);				\n"
			"     break;																\n"
			"     case 1:																\n"
			"       colorDither(vec3(bayerThreshold), clampedColor.rgb);				\n"
			"       break;																\n"
			"     case 2:																\n"
			"       colorDither(snoiseRGB(), clampedColor.rgb);							\n"
			"       break;																\n"
			"     case 3:																\n"
			"       break;																\n"
			"  }																		\n"
			"  switch (uAlphaDitherMode) {												\n"
			"     case 0:																\n"
			"       switch (uColorDitherMode) {											\n"
			"         case 0:															\n"
			"         case 2:															\n"
			"           alphaDither(mSquareThreshold, clampedColor.a);					\n"
			"           break;															\n"
			"         case 1:															\n"
			"         case 3:															\n"
			"           alphaDither(bayerThreshold, clampedColor.a);					\n"
			"           break;															\n"
			"       }																	\n"
			"       break;																\n"
			"     case 1:																\n"
			"       switch (uColorDitherMode) {											\n"
			"         case 0:															\n"
			"         case 2:															\n"
			"           alphaDither(bayerThreshold, clampedColor.a);					\n"
			"           break;															\n"
			"         case 1:															\n"
			"         case 3:															\n"
			"           alphaDither(mSquareThreshold, clampedColor.a);					\n"
			"           break;															\n"
			"       }																	\n"
			"       break;																\n"
			"     case 2:																\n"
			"       alphaDither(snoiseA(), clampedColor.a);								\n"
			"       break;																\n"
			"     case 3:																\n"
			"       break;																\n"
			"  }																		\n"
			;
	}
};

class ShaderFragmentGlobalVariablesNotex : public ShaderPart
{
public:
	ShaderFragmentGlobalVariablesNotex(const opengl::GLInfo & _glinfo)
	{
		m_part =
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
			"uniform lowp int uScreenSpaceTriangle;	\n"
			"uniform lowp int uCvgDest; \n"
			"uniform lowp int uBlendAlphaMode; \n"
			"lowp float cvg; \n"
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

		if (!_glinfo.isGLESX || _glinfo.noPerspective)
			m_part += "noperspective IN lowp vec4 vShadeColorNoperspective;	\n";
		else
			m_part += "IN lowp vec4 vShadeColorNoperspective;				\n";

		m_part +=
			"IN lowp vec4 vShadeColor;	\n"
			"IN lowp float vNumLights;	\n"
			"IN highp vec4 vBaryCoords; \n"
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
				"layout(location = 1) inout highp vec4 depthZ;			\n"
				"layout(location = 2) inout highp vec4 depthDeltaZ;		\n"
				;
			if ((config.generalEmulation.hacks & hack_ZeldaMM) != 0u)
				m_part +=
				"layout(location = 3) inout highp vec4 depthZCopy;		\n"
				"layout(location = 4) inout highp vec4 depthDeltaZCopy;	\n"
				;
		}
	}
};

class ShaderFragmentHeaderNoise : public ShaderPart
{
public:
	ShaderFragmentHeaderNoise(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"lowp float snoise();\n"
		;
	}
};

class ShaderFragmentHeaderWriteDepth : public ShaderPart
{
public:
	ShaderFragmentHeaderWriteDepth(const opengl::GLInfo & _glinfo)
	{
		if (!_glinfo.isGLES2) {
			m_part =
				"highp float writeDepth();\n";
			;
		}
		if (_glinfo.isGLESX &&  _glinfo.noPerspective) {
			m_part =
				"noperspective IN highp float vZCoord;	\n"
				"uniform lowp float uPolygonOffset;	\n"
				"uniform lowp int uClampMode; \n"
				+ m_part
				;
		}

	}
};

class ShaderFragmentHeaderCalcLight : public ShaderPart
{
public:
	ShaderFragmentHeaderCalcLight(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"void calc_light(in lowp float fLights, in lowp vec3 input_color, out lowp vec3 output_color);\n";
			;
	}
};

class ShaderFragmentHeaderMipMap : public ShaderPart
{
public:
	ShaderFragmentHeaderMipMap(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1);\n";
		;
	}
};

class ShaderFragmentHeaderDither : public ShaderPart
{
public:
	ShaderFragmentHeaderDither(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2)
			return;

		m_part =
			"void colorDither(in lowp vec3 _threshold, inout lowp vec3 _color);\n"
			"void alphaDither(in lowp float _threshold, inout lowp float _alpha);\n"
			"lowp vec3 snoiseRGB();\n"
			"lowp float snoiseA();\n"
			;
	}
};

class ShaderFragmentHeaderDepthCompare : public ShaderPart
{
public:
	ShaderFragmentHeaderDepthCompare(const opengl::GLInfo & _glinfo)
	{
		if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
			m_part =
				"bool depth_compare(highp float curZ);	\n"
				"bool depth_render(highp float Z, highp float curZ);	\n"
				;
			if (_glinfo.imageTextures & !_glinfo.n64DepthWithFbFetch) {
				m_part +=
					"layout(binding = 2, r32f) highp uniform restrict image2D uDepthImageZ;			\n"
					"layout(binding = 3, r32f) highp uniform restrict image2D uDepthImageDeltaZ;	\n"
					;
				if ((config.generalEmulation.hacks & hack_ZeldaMM) != 0u)
					m_part +=
					"layout(binding = 4, r32f) highp uniform restrict image2D uDepthImageZCopy;		\n"
					"layout(binding = 5, r32f) highp uniform restrict image2D uDepthImageDeltaZCopy;\n"
					;
			}
		}
	}
};

class ShaderFragmentMain : public ShaderPart
{
public:
	ShaderFragmentMain(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"void main() \n"
			"{			 \n"
			;
		if (!_glinfo.isGLES2) {
			m_part +=
				"  highp float fragDepth = writeDepth();	\n"
				;
		}
		m_part +=
			"  lowp vec4 vec_color;				\n"
			"  lowp float alpha1;				\n"
			"  lowp vec3 color1, input_color;	\n"
		;
		if (config.generalEmulation.enableClipping != 0)
			m_part +=
			"  lowp vec4 shadeColor = vShadeColorNoperspective;	\n"
			;
		else
			m_part +=
			"  lowp vec4 shadeColor = uScreenSpaceTriangle == 0 ? vShadeColor : vShadeColorNoperspective;	\n"
			;

		m_part += "#define WRAP(x, low, high) (mod((x)-(low), (high)-(low)) + (low)) \n"; // Return wrapped value of x in interval [low, high)
		// m_part += "#define WRAP(x, low, high) (x) - ((high)-(low)) * floor(((x)-(low))/((high)-(low)))  \n"; // Perhaps more compatible?
		// m_part += "#define WRAP(x, low, high) (x) + ((high)-(low)) * (1.0-step(low,x)) - ((high)-(low)) * step(high,x) \n"; // Step based version. Only wraps correctly if input is in the range [low-(high-low), high + (high-low)). Similar to old code.
	}

};

class ShaderFragmentMain2Cycle : public ShaderPart
{
public:
	ShaderFragmentMain2Cycle(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"void main() \n"
			"{			 \n"
		;
		if (!_glinfo.isGLES2) {
			m_part +=
				"  highp float fragDepth = writeDepth(); \n"
			;
		}
		m_part +=
			"  lowp vec4 vec_color, combined_color;		\n"
			"  lowp float alpha1, alpha2;				\n"
			"  lowp vec3 color1, color2, input_color;	\n"
		;
		if (config.generalEmulation.enableClipping != 0)
			m_part +=
			"  lowp vec4 shadeColor = vShadeColorNoperspective;	\n"
			;
		else
			m_part +=
			"  lowp vec4 shadeColor = uScreenSpaceTriangle == 0 ? vShadeColor : vShadeColorNoperspective;	\n"
			;

		m_part += "#define WRAP(x, low, high) mod((x)-(low), (high)-(low)) + (low) \n"; // Return wrapped value of x in interval [low, high)
		// m_part += "#define WRAP(x, low, high) (x) - ((high)-(low)) * floor(((x)-(low))/((high)-(low)))  \n"; // Perhaps more compatible?
		// m_part += "#define WRAP(x, low, high) (x) + (2.0) * (1.0-step(low,x)) - (2.0) * step(high,x) \n"; // Step based version. Only wraps correctly if input is in the range [low-(high-low), high + (high-low)). Similar to old code.
	}
};

class ShaderFragmentBlendMux : public ShaderPart
{
public:
	ShaderFragmentBlendMux(const opengl::GLInfo & _glinfo)
	{
		if (config.generalEmulation.enableLegacyBlending == 0) {
			m_part =
				"  #define MUXA(pos) dot(muxA, STVEC(pos))									\n"
				"  #define MUXB(pos) dot(muxB, STVEC(pos))									\n"
				"  #define MUXPM(pos) muxPM*(STVEC(pos))									\n"
				"  #define MUXF(pos) dot(muxF, STVEC(pos))									\n"
				"  lowp vec4 lastFragColor = LAST_FRAG_COLOR;								\n"
				"  lowp float lastFragAlpha = LAST_FRAG_ALPHA;								\n"
				"  lowp mat4 muxPM = mat4(vec4(0.0), lastFragColor, uBlendColor, uFogColor); \n"
				"  lowp vec4 muxA = vec4(0.0, uFogColor.a, shadeColor.a, 0.0);				\n"
				"  lowp vec4 muxB = vec4(0.0, lastFragAlpha, 1.0, 0.0);				\n"
				"  lowp vec4 muxF = vec4(0.0, 1.0, 0.0, 0.0);								\n"
				"  lowp vec4 muxp, muxm, srcColor1, srcColor2;								\n"
				"  lowp float muxa, muxb, dstFactor1, dstFactor2, muxaf, muxbf;				\n"
				;
		}
	}
};

class ShaderFragmentReadTexMipmap : public ShaderPart
{
public:
	ShaderFragmentReadTexMipmap(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"  lowp vec4 readtex0, readtex1;						\n"
			"  lowp float lod_frac = mipmap(readtex0, readtex1);	\n"
		;
	}
};

class ShaderFragmentCallN64Depth : public ShaderPart
{
public:
	ShaderFragmentCallN64Depth(const opengl::GLInfo & _glinfo)
	{
		if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
			m_part = "  bool should_discard = false;	\n";

			if (_glinfo.imageTextures && !_glinfo.n64DepthWithFbFetch) {
				if (_glinfo.fragment_interlock)
					m_part += "  beginInvocationInterlockARB();	\n";
				else if (_glinfo.fragment_interlockNV)
					m_part += "  beginInvocationInterlockNV();	\n";
				else if (_glinfo.fragment_ordering)
					m_part += "  beginFragmentShaderOrderingINTEL();	\n";
			}

			m_part +=
				"  if (uRenderTarget != 0) { if (!depth_render(fragColor.r, fragDepth)) should_discard = true; } \n"
				"  else if (!depth_compare(fragDepth)) should_discard = true; \n"
				;

			if (_glinfo.imageTextures & !_glinfo.n64DepthWithFbFetch) {
				if (_glinfo.fragment_interlock)
					m_part += "  endInvocationInterlockARB();	\n";
				else if (_glinfo.fragment_interlockNV)
					m_part += "  endInvocationInterlockNV();	\n";
			}

			m_part += "  if (should_discard) discard;	\n";

		}
	}
};

class ShaderFragmentRenderTarget : public ShaderPart
{
public:
	ShaderFragmentRenderTarget(const opengl::GLInfo & _glinfo)
	{
		if (config.generalEmulation.enableFragmentDepthWrite != 0) {
			if ((config.generalEmulation.hacks & hack_ZeldaMM) != 0)
				// Zelda MM depth buffer copy
				m_part =
					"  if (uRenderTarget == 1 || uRenderTarget == 3) {\n"
					"    fragDepth = fragColor.r;					\n"
					"  } else if (uRenderTarget == 2) {				\n"
					"    ivec2 coord = ivec2(gl_FragCoord.xy);		\n"
					"    if (fragDepth >= texelFetch(uDepthTex, coord, 0).r) discard;	\n"
					"    fragDepth = fragColor.r;					\n"
					"  } else if (uRenderTarget == 4) {				\n"
					"    ivec2 coord = ivec2(gl_FragCoord.xy);		\n"
					"    if (texelFetch(uDepthTex, coord, 0).r > 0.0) discard;	\n"
					"    fragDepth = fragColor.r;					\n"
					"  }											\n"
					"  gl_FragDepth = fragDepth;					\n"
				;
			else
				m_part =
					"  if (uRenderTarget == 1) {					\n"
					"    fragDepth = fragColor.r;					\n"
					"  } else if (uRenderTarget == 2) {				\n"
					"    ivec2 coord = ivec2(gl_FragCoord.xy);		\n"
					"    if (fragDepth >= texelFetch(uDepthTex, coord, 0).r) discard;	\n"
					"    fragDepth = fragColor.r;					\n"
					"  }											\n"
					"  gl_FragDepth = fragDepth;	\n"
				;
		}
	}
};

class ShaderFragmentMainEnd : public ShaderPart
{
public:
	ShaderFragmentMainEnd(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			m_part =
					"  gl_FragColor = fragColor; \n"
					"} \n\n";
		} else {
			m_part =
					"} \n\n"
					;
		}
	}
};

class ShaderNoise : public ShaderPart
{
public:
	ShaderNoise(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			m_part =
				"uniform sampler2D uTexNoise;							\n"
				"lowp float snoise()									\n"
				"{														\n"
				"  mediump vec2 texSize = vec2(640.0, 580.0);			\n"
				"  mediump vec2 coord = gl_FragCoord.xy/uScreenScale/texSize;	\n"
				"  return texture2D(uTexNoise, coord).r;				\n"
				"}														\n"
				;
		} else {
			m_part =
				"uniform sampler2D uTexNoise;							\n"
				"lowp float snoise()									\n"
				"{														\n"
				"  ivec2 coord = ivec2(gl_FragCoord.xy/uScreenScale);	\n"
				"  return texelFetch(uTexNoise, coord, 0).r;			\n"
				"}														\n"
				;
		}
	}
};

class ShaderDither : public ShaderPart
{
public:
	ShaderDither(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2)
			return;

		if (config.generalEmulation.enableDitheringQuantization != 0) {
			m_part =
				"void quantizeRGB(inout lowp vec3 _color)				\n"
				"{														\n"
				"     _color.rgb = round(_color.rgb * 32.0)/32.0;		\n"
				"}														\n"
				"void quantizeA(inout lowp float _alpha)				\n"
				"{														\n"
				"     _alpha = round(_alpha * 32.0)/32.0;				\n"
				"}														\n"
				;
		} else {
			m_part =
				"void quantizeRGB(inout lowp vec3 _color){}\n"
				"void quantizeA(inout lowp float _alpha){}\n"
				;
		}

		m_part +=
			"void colorDither(in lowp vec3 _noise, inout lowp vec3 _color)\n"
			"{															\n"
			"  mediump vec3 threshold = 7.0 / 255.0 * (_noise - 0.5);	\n"
			"  _color = clamp(_color + threshold,0.0,1.0);				\n"
			"  quantizeRGB(_color);										\n"
			"}															\n"
			"void alphaDither(in lowp float _noise, inout lowp float _alpha)\n"
			"{															\n"
			"  mediump float threshold = 7.0 / 255.0 * (_noise - 0.5);	\n"
			"  _alpha = clamp(_alpha + threshold,0.0,1.0);				\n"
			"  quantizeA(_alpha);										\n"
			"}															\n"
			"lowp vec3 snoiseRGB()									\n"
			"{														\n"
			"  mediump vec2 texSize = vec2(640.0, 580.0);			\n"
			;
		if (config.generalEmulation.enableHiresNoiseDithering != 0)
			// multiplier for higher res noise effect
			m_part +=
			"  lowp float mult = 1.0 + step(2.0, uScreenScale.x);	\n";
		else
			m_part +=
			"  lowp float mult = 1.0;								\n";
		m_part +=
			"	mediump vec2 coordR = mult * ((gl_FragCoord.xy)/uScreenScale/texSize);\n"
			"	mediump vec2 coordG = mult * ((gl_FragCoord.xy + vec2( 0.0, texSize.y / 2.0 ))/uScreenScale/texSize);\n"
			"	mediump vec2 coordB = mult * ((gl_FragCoord.xy + vec2( texSize.x / 2.0,  0.0))/uScreenScale/texSize);\n"
			// Only red channel of noise texture contains noise.
			"  lowp float r = texture(uTexNoise,coordR).r;			\n"
			"  lowp float g = texture(uTexNoise,coordG).r;			\n"
			"  lowp float b = texture(uTexNoise,coordB).r;			\n"
			"														\n"
			"  return vec3(r,g,b);									\n"
			"}														\n"
			"lowp float snoiseA()									\n"
			"{														\n"
			"  mediump vec2 texSize = vec2(640.0, 580.0);			\n"
			;
		if (config.generalEmulation.enableHiresNoiseDithering != 0)
			// multiplier for higher res noise effect
			m_part +=
			"  lowp float mult = 1.0 + step(2.0, uScreenScale.x);	\n";
		else
			m_part +=
			"  lowp float mult = 1.0;								\n";
		m_part +=
			"														\n"
			"	mediump vec2 coord = mult * ((gl_FragCoord.xy)/uScreenScale/texSize);\n"
			"														\n"
			// Only red channel of noise texture contains noise.
			"  return texture(uTexNoise,coord).r;					\n"
			"}														\n"
			;
	}
};

class ShaderWriteDepth : public ShaderPart
{
public:
	ShaderWriteDepth(const opengl::GLInfo & _glinfo)
	{
		if (!_glinfo.isGLES2) {
			if (config.generalEmulation.enableFragmentDepthWrite == 0 &&
				config.frameBufferEmulation.N64DepthCompare == Config::dcDisable) {
				// Dummy write depth
				m_part =
					"highp float writeDepth()	    \n"
					"{						\n"
					"  return 0.0;	\n"
					"}						\n"
				;
			} else {
				if ((config.generalEmulation.hacks & hack_RE2) != 0) {
					m_part =
						"uniform lowp usampler2D uZlutImage;\n"
						"highp float writeDepth()																		\n"
						"{																								\n"
						;
					if (_glinfo.isGLESX) {
						if (_glinfo.noPerspective) {
							m_part +=
								"  if (uClampMode == 1 && (vZCoord > 1.0)) discard;										\n"
								"  highp float FragDepth = (uDepthSource != 0) ? uPrimDepth :							\n"
								"         clamp((vZCoord - uPolygonOffset) * uDepthScale.s + uDepthScale.t, 0.0, 1.0);	\n"
							;
						} else if (config.generalEmulation.enableClipping != 0) {
							m_part +=
							"  highp float FragDepth = (uDepthSource != 0) ? uPrimDepth :								\n"
							"      clamp(8.0 * (gl_FragCoord.z * 2.0 - 1.0) * uDepthScale.s + uDepthScale.t, 0.0, 1.0);	\n"
							;
						} else {
							m_part +=
							"  highp float FragDepth = (uDepthSource != 0) ? uPrimDepth :								\n"
							"            clamp((gl_FragCoord.z * 2.0 - 1.0) * uDepthScale.s + uDepthScale.t, 0.0, 1.0);	\n"
							;
						}
					} else {
						m_part +=
							"  highp float FragDepth = (uDepthSource != 0) ? uPrimDepth :								\n"
							"            clamp((gl_FragCoord.z * 2.0 - 1.0) * uDepthScale.s + uDepthScale.t, 0.0, 1.0);	\n"
						;
					}
					m_part +=
						"  highp int iZ = FragDepth > 0.999 ? 262143 : int(floor(FragDepth * 262143.0));					\n"
						"  mediump int y0 = clamp(iZ/512, 0, 511);															\n"
						"  mediump int x0 = iZ - 512*y0;																	\n"
						"  highp uint iN64z = texelFetch(uZlutImage,ivec2(x0,y0), 0).r;										\n"
						"  return clamp(float(iN64z)/65532.0, 0.0, 1.0);													\n"
						"}																									\n"
						;
				} else {
					if (_glinfo.isGLESX) {
						if (_glinfo.noPerspective) {
							m_part =
								"highp float writeDepth()																	\n"
								"{																							\n"
								"  if (uClampMode == 1 && (vZCoord > 1.0)) discard;											\n"
								"  if (uDepthSource != 0) return uPrimDepth;												\n"
								"  return clamp((vZCoord - uPolygonOffset) * uDepthScale.s + uDepthScale.t, 0.0, 1.0);		\n"
								"}																							\n"
							;
						} else if (config.generalEmulation.enableClipping != 0) {
							m_part =
								"highp float writeDepth()						        										\n"
								"{																								\n"
								"  if (uDepthSource != 0) return uPrimDepth;													\n"
								"  return clamp(8.0 * (gl_FragCoord.z * 2.0 - 1.0) * uDepthScale.s + uDepthScale.t, 0.0, 1.0);	\n"
								"}																								\n"
							;
						} else {
							m_part =
								"highp float writeDepth()						        									\n"
								"{																							\n"
								"  if (uDepthSource != 0) return uPrimDepth;												\n"
								"  return clamp((gl_FragCoord.z * 2.0 - 1.0) * uDepthScale.s + uDepthScale.t, 0.0, 1.0);	\n"
								"}																							\n"
								;
						}
					} else {
						m_part =
							"highp float writeDepth()						        									\n"
							"{																							\n"
							"  if (uDepthSource != 0) return uPrimDepth;												\n"
							"  return clamp((gl_FragCoord.z * 2.0 - 1.0) * uDepthScale.s + uDepthScale.t, 0.0, 1.0);	\n"
							"}																							\n"
						;
					}
				}
			}
		}
	}
};

class ShaderCalcLight : public ShaderPart
{
public:
	ShaderCalcLight(const opengl::GLInfo & /*_glinfo*/)
	{
		m_part =
			"uniform mediump vec3 uLightDirection[8];	\n"
			"uniform lowp vec3 uLightColor[8];			\n"
			"void calc_light(in lowp float fLights, in lowp vec3 input_color, out lowp vec3 output_color) {\n"
			"  output_color = input_color;									\n"
			"  lowp int nLights = int(floor(fLights + 0.5));				\n"
			"  if (nLights == 0)											\n"
			"     return;													\n"
			"  output_color = uLightColor[nLights];							\n"
			"  mediump float intensity;										\n"
			"  for (int i = 0; i < nLights; i++)	{						\n"
			"    intensity = max(dot(input_color, uLightDirection[i]), 0.0);\n"
			"    output_color += intensity*uLightColor[i];					\n"
			"  };															\n"
			"  output_color = clamp(output_color, 0.0, 1.0);				\n"
			"}																\n"
		;
	}
};

class ShaderN64DepthCompare : public ShaderPart
{
public:
	ShaderN64DepthCompare(const opengl::GLInfo & _glinfo)
	{
		if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
			m_part =
				"uniform lowp int uEnableDepth;							\n"
				"uniform lowp int uDepthMode;							\n"
				"uniform lowp int uEnableDepthUpdate;					\n"
				"uniform mediump float uDeltaZ;							\n"
				"bool depth_compare(highp float curZ)					\n"
				"{														\n"
				"  if (uEnableDepth == 0) return true;					\n"
				;
			if (_glinfo.imageTextures && !_glinfo.n64DepthWithFbFetch) {
				m_part +=
				"  ivec2 coord = ivec2(gl_FragCoord.xy);				\n"
				"  highp vec4 depthZ = imageLoad(uDepthImageZ,coord);	\n"
				"  highp vec4 depthDeltaZ = imageLoad(uDepthImageDeltaZ,coord);\n"
					;
			}
			m_part +=
				"  highp float bufZ = depthZ.r;							\n"
				"  highp float dz, dzMin;								\n"
				"  if (uDepthSource == 1) {								\n"
				"     dzMin = dz = uDeltaZ;								\n"
				"  } else {												\n"
				"    dz = 4.0*fwidth(curZ);								\n"
				"    dzMin = min(dz, depthDeltaZ.r);					\n"
				"  }													\n"
				"  bool bInfront = curZ < bufZ;							\n"
				"  bool bFarther = (curZ + dzMin) >= bufZ;				\n"
				"  bool bNearer = (curZ - dzMin) <= bufZ;				\n"
				"  bool bMax = bufZ == 1.0;								\n"
				"  bool bRes = false;									\n"
				"  switch (uDepthMode) {								\n"
				"     case 1:											\n"
				"       bRes = bMax || bNearer;							\n"
				"       break;											\n"
				"     case 0:											\n"
				"     case 2:											\n"
				"       bRes = bMax || bInfront;						\n"
				"       break;											\n"
				"     case 3:											\n"
				"       bRes = bFarther && bNearer && !bMax;			\n"
				"       break;											\n"
				"  }													\n"
				"  bRes = bRes || (uEnableDepthCompare == 0);			\n"
				"  if (uEnableDepthUpdate != 0 && bRes) {				\n"
				;
			if (_glinfo.n64DepthWithFbFetch) {
				m_part +=
					"    depthZ.r = curZ;									\n"
					"    depthDeltaZ.r = dz;								\n"
					;
			} else if (_glinfo.imageTextures) {
				m_part +=
				"    highp vec4 depthOutZ = vec4(curZ, 1.0, 1.0, 1.0);		\n"
				"    highp vec4 depthOutDeltaZ = vec4(dz, 1.0, 1.0, 1.0);	\n"
				"    imageStore(uDepthImageZ, coord, depthOutZ);			\n"
				"    imageStore(uDepthImageDeltaZ, coord, depthOutDeltaZ);	\n"
					;
			}

			m_part +=
				"  }													\n"
				"  return bRes;											\n"
				"}														\n"
			;
		}
	}
};

class ShaderN64DepthRender : public ShaderPart
{
public:
	ShaderN64DepthRender(const opengl::GLInfo & _glinfo)
	{
		if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
			m_part =
				"bool depth_render(highp float Z, highp float curZ)			\n"
				"{															\n"
				"  ivec2 coord = ivec2(gl_FragCoord.xy);					\n"
				;

			if ((config.generalEmulation.hacks & hack_ZeldaMM) != 0u) {
				// Zelda MM depth buffer copy
				if (_glinfo.imageTextures && !_glinfo.n64DepthWithFbFetch) {
					m_part +=
						"  if (uRenderTarget == 3) {							\n"
						"    highp vec4 copyZ = imageLoad(uDepthImageZ,coord);	\n"
						"    imageStore(uDepthImageZCopy, coord, copyZ);		\n"
						"    copyZ = imageLoad(uDepthImageDeltaZ, coord);		\n"
						"    imageStore(uDepthImageDeltaZCopy, coord, copyZ);	\n"
						"    return true;										\n"
						"  }													\n"
						"  if (uRenderTarget == 4) {							\n"
						"    highp vec4 testZ = imageLoad(uDepthImageZCopy,coord);	\n"
						"    if (testZ.r > 0.0) return false;					\n"
						"    highp vec4 copyZ = imageLoad(uDepthImageZ, coord);	\n"
						"    imageStore(uDepthImageZCopy, coord, copyZ);		\n"
						"    copyZ = imageLoad(uDepthImageDeltaZ, coord);		\n"
						"    imageStore(uDepthImageDeltaZCopy, coord, copyZ);	\n"
						"    return true;										\n"
						"  }													\n"
						;
				}
				else {
					m_part +=
						"  if (uRenderTarget == 3) {							\n"
						"    depthZCopy.r = depthZ.r;							\n"
						"    depthDeltaZCopy.r = depthDeltaZ.r;					\n"
						"    return true;										\n"
						"  }													\n"
						"  if (uRenderTarget == 4) {							\n"
						"    if (depthZCopy.r > 0.0) return false;				\n"
						"    depthZCopy.r = depthZ.r;							\n"
						"    depthDeltaZCopy.r = depthDeltaZ.r;					\n"
						"    return true;										\n"
						"  }													\n"
						;
				}
			}

			m_part +=
				"  if (uEnableDepthCompare != 0) {							\n"
				;
			if (_glinfo.imageTextures && !_glinfo.n64DepthWithFbFetch) {
				m_part +=
					"    highp vec4 depthZ = imageLoad(uDepthImageZ,coord);	\n"
					;
			}
			m_part +=
				"    highp float bufZ = depthZ.r;							\n"
				"    if (curZ >= bufZ) return false;						\n"
				"  }														\n"
				;
			if (_glinfo.n64DepthWithFbFetch) {
				m_part +=
					"  depthZ.r = Z;										\n"
					"  depthDeltaZ.r = 0.0;									\n"
					;
			} else if (_glinfo.imageTextures) {
				m_part +=
					"  highp vec4 depthOutZ = vec4(Z, 1.0, 1.0, 1.0);		\n"
					"  highp vec4 depthOutDeltaZ = vec4(0.0, 1.0, 1.0, 1.0);\n"
					"  imageStore(uDepthImageZ,coord, depthOutZ);			\n"
					"  imageStore(uDepthImageDeltaZ,coord, depthOutDeltaZ);	\n"
					;
			}

			m_part +=
				"  return true;												\n"
				"}															\n"
				;
		}
	}
};

class ShaderCoverage : public ShaderPart {
public:
	ShaderCoverage() {
		m_part =
			"const highp vec2 bias[8] = vec2[8] (vec2(-0.5,-0.5), vec2(0.0, -0.5), vec2(-0.25,-0.25), vec2(0.25, -0.25), \n"
			"                   vec2(-0.5, 0.0), vec2(0.0,0.0), vec2(-0.25,0.25), vec2(0.25,0.25)); \n"
			"highp vec4 dBCdx = dFdx(vBaryCoords);														\n"
			"highp vec4 dBCdy = dFdy(vBaryCoords);														\n"
			"cvg = 0.0;																					\n"
			"for (int i = 0; i<8; i++) {																\n"
			"  highp vec2 currentBias = bias[i];														\n"
			"  highp vec4 baryCoordsBiased = vBaryCoords + dBCdx*currentBias.x + dBCdy * currentBias.y; \n"
			"  lowp vec4 inside = step(0.0, baryCoordsBiased);											\n"
			"  cvg += 0.125 * inside[0] * inside[1] * inside[2] * inside[3];							\n"
			"}																							\n"
			;
	}
};

/*---------------ShaderPartsEnd-------------*/

static
GLuint _createVertexShader(const ShaderPart* _header, const ShaderPart* _body, const ShaderPart* _footer)
{
	std::stringstream ssShader;
	_header->write(ssShader);
	_body->write(ssShader);
	_footer->write(ssShader);
	const std::string strShader(ssShader.str());
	const GLchar * strShaderData = strShader.data();

	GLuint shader_object = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shader_object, 1, &strShaderData, nullptr);
	glCompileShader(shader_object);
	if (!Utils::checkShaderCompileStatus(shader_object))
		Utils::logErrorShader(GL_VERTEX_SHADER, strShaderData);
	return shader_object;
}

} // nameless namespace

namespace glsl {

CombinerProgramBuilderCommon::CombinerProgramBuilderCommon(const opengl::GLInfo & _glinfo, opengl::CachedUseProgram * _useProgram,
	std::unique_ptr<CombinerProgramUniformFactory> _uniformFactory)
: CombinerProgramBuilder(_glinfo, _useProgram, std::move(_uniformFactory))
, m_blender1(new ShaderBlender1(_glinfo))
, m_blender2(new ShaderBlender2(_glinfo))
, m_blenderAlpha (new ShaderBlenderAlpha(_glinfo))
, m_legacyBlender(new ShaderLegacyBlender)
, m_clamp(new ShaderClamp)
, m_signExtendColorC(new ShaderSignExtendColorC)
, m_signExtendAlphaC(new ShaderSignExtendAlphaC)
, m_signExtendColorABD(new ShaderSignExtendColorABD)
, m_signExtendAlphaABD(new ShaderSignExtendAlphaABD)
, m_alphaTest(new ShaderAlphaTest)
, m_callDither(new ShaderDithering(_glinfo))
, m_vertexHeader(new VertexShaderHeader(_glinfo))
, m_vertexEnd(new VertexShaderEnd(_glinfo))
, m_vertexRect(new VertexShaderRect(_glinfo))
, m_vertexTriangle(new VertexShaderTriangle(_glinfo))
, m_fragmentHeader(new FragmentShaderHeader(_glinfo))
, m_fragmentGlobalVariablesNotex(new ShaderFragmentGlobalVariablesNotex(_glinfo))
, m_fragmentHeaderNoise(new ShaderFragmentHeaderNoise(_glinfo))
, m_fragmentHeaderWriteDepth(new ShaderFragmentHeaderWriteDepth(_glinfo))
, m_fragmentHeaderCalcLight(new ShaderFragmentHeaderCalcLight(_glinfo))
, m_fragmentHeaderMipMap(new ShaderFragmentHeaderMipMap(_glinfo))
, m_fragmentHeaderDither(new ShaderFragmentHeaderDither(_glinfo))
, m_fragmentHeaderDepthCompare(new ShaderFragmentHeaderDepthCompare(_glinfo))
, m_fragmentMain(new ShaderFragmentMain(_glinfo))
, m_fragmentMain2Cycle(new ShaderFragmentMain2Cycle(_glinfo))
, m_fragmentBlendMux(new ShaderFragmentBlendMux(_glinfo))
, m_fragmentReadTexMipmap(new ShaderFragmentReadTexMipmap(_glinfo))
, m_fragmentCallN64Depth(new ShaderFragmentCallN64Depth(_glinfo))
, m_fragmentRenderTarget(new ShaderFragmentRenderTarget(_glinfo))
, m_shaderFragmentMainEnd(new ShaderFragmentMainEnd(_glinfo))
, m_shaderNoise(new ShaderNoise(_glinfo))
, m_shaderDither(new ShaderDither(_glinfo))
, m_shaderWriteDepth(new ShaderWriteDepth(_glinfo))
, m_shaderCalcLight(new ShaderCalcLight(_glinfo))
, m_shaderN64DepthCompare(new ShaderN64DepthCompare(_glinfo))
, m_shaderN64DepthRender(new ShaderN64DepthRender(_glinfo))
, m_shaderCoverage(new ShaderCoverage())
, m_combinerOptionsBits(graphics::CombinerProgram::getShaderCombinerOptionsBits())
{
}

CombinerProgramBuilderCommon::~CombinerProgramBuilderCommon()
{
	glDeleteShader(m_vertexShaderRect);
	glDeleteShader(m_vertexShaderTriangle);
	glDeleteShader(m_vertexShaderTexturedRect);
	glDeleteShader(m_vertexShaderTexturedTriangle);
}

const ShaderPart * CombinerProgramBuilderCommon::getVertexShaderHeader() const
{
	return m_vertexHeader.get();
}

const ShaderPart * CombinerProgramBuilderCommon::getFragmentShaderHeader() const
{
	return m_fragmentHeader.get();
}

const ShaderPart * CombinerProgramBuilderCommon::getFragmentShaderEnd() const
{
	return m_shaderFragmentMainEnd.get();
}

bool CombinerProgramBuilderCommon::isObsolete() const
{
	return m_combinerOptionsBits != graphics::CombinerProgram::getShaderCombinerOptionsBits();
}

GLuint CombinerProgramBuilderCommon::_getVertexShaderRect() const
{
	if (m_vertexShaderRect == 0)
		m_vertexShaderRect = _createVertexShader(m_vertexHeader.get(), m_vertexRect.get(), m_vertexEnd.get());
	return m_vertexShaderRect;
}

GLuint CombinerProgramBuilderCommon::_getVertexShaderTriangle() const
{
	if (m_vertexShaderTriangle == 0)
		m_vertexShaderTriangle = _createVertexShader(m_vertexHeader.get(), m_vertexTriangle.get(), m_vertexEnd.get());
	return m_vertexShaderTriangle;
}

GLuint CombinerProgramBuilderCommon::_getVertexShaderTexturedRect() const
{
	if (m_vertexShaderTexturedRect == 0)
		m_vertexShaderTexturedRect = _createVertexShader(m_vertexHeader.get(), getVertexShaderTexturedRect(), m_vertexEnd.get());
	return m_vertexShaderTexturedRect;
}

GLuint CombinerProgramBuilderCommon::_getVertexShaderTexturedTriangle() const
{
	if (m_vertexShaderTexturedTriangle == 0)
		m_vertexShaderTexturedTriangle = _createVertexShader(m_vertexHeader.get(), getVertexShaderTexturedTriangle(), m_vertexEnd.get());
	return m_vertexShaderTexturedTriangle;
}

void CombinerProgramBuilderCommon::_writeSignExtendAlphaC(std::stringstream& ssShader)const
{
	 m_signExtendAlphaC->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeSignExtendAlphaABD(std::stringstream& ssShader)const
{
	 m_signExtendAlphaABD->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeAlphaTest(std::stringstream& ssShader)const
{
	 m_alphaTest->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeSignExtendColorC(std::stringstream& ssShader)const
{
	 m_signExtendColorC->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeSignExtendColorABD(std::stringstream& ssShader)const
{
	 m_signExtendColorABD->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeClamp(std::stringstream& ssShader)const
{
	 m_clamp->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeCallDither(std::stringstream& ssShader)const
{
	 m_callDither->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeBlender1(std::stringstream& ssShader)const
{
	 m_blender1->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeBlender2(std::stringstream& ssShader)const
{
	m_blender2->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeBlenderAlpha(std::stringstream& ssShader)const
{
	 m_blenderAlpha->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeLegacyBlender(std::stringstream& ssShader)const
{
	 m_legacyBlender->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeFragmentHeader(std::stringstream& ssShader)const
{
	 m_fragmentHeader->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeFragmentHeaderDither(std::stringstream& ssShader)const
{
	 m_fragmentHeaderDither->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeFragmentHeaderNoise(std::stringstream& ssShader)const
{
	 m_fragmentHeaderNoise->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeFragmentHeaderWriteDepth(std::stringstream& ssShader)const
{
	 m_fragmentHeaderWriteDepth->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeFragmentHeaderDepthCompare(std::stringstream& ssShader)const
{
	 m_fragmentHeaderDepthCompare->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeFragmentHeaderMipMap(std::stringstream& ssShader)const
{
	 m_fragmentHeaderMipMap->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeFragmentGlobalVariablesNotex(std::stringstream& ssShader)const
{
	 m_fragmentGlobalVariablesNotex->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeFragmentHeaderCalcLight(std::stringstream& ssShader)const
{
	 m_fragmentHeaderCalcLight->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeFragmentMain2Cycle(std::stringstream& ssShader)const
{
	 m_fragmentMain2Cycle->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeFragmentMain(std::stringstream& ssShader)const
{
	 m_fragmentMain->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeFragmentBlendMux(std::stringstream& ssShader)const
{
	 m_fragmentBlendMux->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeShaderCoverage(std::stringstream& ssShader)const
{
	 m_shaderCoverage->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeFragmentReadTexMipmap(std::stringstream& ssShader)const
{
	 m_fragmentReadTexMipmap->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeFragmentCallN64Depth(std::stringstream& ssShader)const
{
	 m_fragmentCallN64Depth->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeFragmentRenderTarget(std::stringstream& ssShader)const
{
	 m_fragmentRenderTarget->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeShaderFragmentMainEnd(std::stringstream& ssShader)const
{
	 m_shaderFragmentMainEnd->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeShaderCalcLight(std::stringstream& ssShader)const
{
	 m_shaderCalcLight->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeShaderNoise(std::stringstream& ssShader)const
{
	 m_shaderNoise->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeShaderDither(std::stringstream& ssShader)const
{
	 m_shaderDither->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeShaderWriteDepth(std::stringstream& ssShader)const
{
	 m_shaderWriteDepth->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeShaderN64DepthCompare(std::stringstream& ssShader)const
{
	 m_shaderN64DepthCompare->write(ssShader);
}

void CombinerProgramBuilderCommon::_writeShaderN64DepthRender(std::stringstream& ssShader)const
{
	 m_shaderN64DepthRender->write(ssShader);
}

}
