#include <Config.h>
#include "glsl_CombinerProgramBuilderFast.h"
#include "glsl_CombinerProgramUniformFactoryFast.h"

namespace {
using namespace glsl;

class VertexShaderTexturedTriangleFast : public ShaderPart
{
public:
	VertexShaderTexturedTriangleFast(const opengl::GLInfo & _glinfo)
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
			"uniform mediump vec2 uTexOffset[2];				\n"
			"uniform mediump vec2 uCacheScale[2];				\n"
			"uniform mediump vec2 uCacheOffset[2];				\n"
			"uniform mediump vec2 uCacheShiftScale[2];			\n"
			"uniform mediump vec2 uVTrans;						\n"
			"uniform mediump vec2 uVScale;						\n"
			"uniform mediump vec2 uAdjustTrans;					\n"
			"uniform mediump vec2 uAdjustScale;					\n"
			"uniform lowp ivec2 uCacheFrameBuffer;				\n"
			"OUT highp vec2 vTexCoord0;							\n"
			"OUT highp vec2 vTexCoord1;							\n"
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
			"mediump vec2 calcTexCoord(in vec2 texCoord, in int idx)		\n"
			"{																\n"
			"    vec2 texCoordOut = texCoord*uCacheShiftScale[idx];			\n"
			"    texCoordOut -= uTexOffset[idx];							\n"
			"    texCoordOut += uCacheOffset[idx];							\n"
			"    if (uTextureFilterMode != 0 && uCacheFrameBuffer[idx] != 0) \n"	/* Workaround for framebuffer textures. */
			"      texCoordOut -= vec2(0.0,1.0);							\n"		/* They contain garbage at the bottom.  */
			"    return texCoordOut * uCacheScale[idx];						\n"
			"}																\n"
			"																\n"
			"void main()													\n"
			"{																\n"
			"  gl_Position = aPosition;										\n"
			"  vShadeColor = aColor;										\n"
			"  vec2 texCoord = aTexCoord;									\n"
			"  texCoord *= uTexScale;										\n"
			"  if (uTexturePersp == 0 && aModify[2] == 0.0) texCoord *= 0.5;\n"
			"  vTexCoord0 = calcTexCoord(texCoord, 0);						\n"
			"  vTexCoord1 = calcTexCoord(texCoord, 1);						\n"
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

class VertexShaderTexturedRectFast : public ShaderPart
{
public:
	VertexShaderTexturedRectFast(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"IN highp vec4 aRectPosition;						\n"
			"IN highp vec2 aTexCoord0;							\n"
			"IN highp vec2 aTexCoord1;							\n"
			"IN highp vec2 aBaryCoords;							\n"
			"													\n"
			"OUT highp vec2 vTexCoord0;							\n"
			"OUT highp vec2 vTexCoord1;							\n"
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
			"  vTexCoord0 = aTexCoord0;							\n"
			"  vTexCoord1 = aTexCoord1;							\n"
			"  vBaryCoords = vec4(aBaryCoords, vec2(1.0) - aBaryCoords);	\n"
			;
	}
};

class ShaderFragmentGlobalVariablesTexFast : public ShaderPart
{
public:
	ShaderFragmentGlobalVariablesTexFast(const opengl::GLInfo & _glinfo)
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
			"uniform highp vec4 uTexClamp0;			\n"
			"uniform highp vec4 uTexClamp1;			\n"
			"uniform highp vec2 uTexWrap0;			\n"
			"uniform highp vec2 uTexWrap1;			\n"
			"uniform lowp vec2 uTexMirror0;			\n"
			"uniform lowp vec2 uTexMirror1;			\n"
			"uniform highp vec2 uTexScale0;			\n"
			"uniform highp vec2 uTexScale1;			\n"
			"uniform highp vec2 uTexCoordOffset[2];	\n"
			"uniform lowp int uUseTexCoordBounds;	\n"
			"uniform highp vec4 uTexCoordBounds0;	\n"
			"uniform highp vec4 uTexCoordBounds1;	\n"
			"uniform lowp int uScreenSpaceTriangle;	\n"
			"highp vec2 texCoord0;					\n"
			"highp vec2 texCoord1;					\n"
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
			"IN highp vec2 vTexCoord0;		\n"
			"IN highp vec2 vTexCoord1;		\n"
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
	}
};

class ShaderFragmentHeaderClampWrapMirror : public ShaderPart
{
public:
	ShaderFragmentHeaderClampWrapMirror(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"highp vec2 clampWrapMirror(in highp vec2 vTexCoord,	\n"
			"	in highp vec4 vClamp, in highp vec2 vWrap,			\n"
			"	in lowp vec2 vMirror, in highp vec2 vOffset);		\n"
		;
	}
};

class ShaderFragmentHeaderReadMSTexFast : public ShaderPart
{
public:
	ShaderFragmentHeaderReadMSTexFast(const opengl::GLInfo & _glinfo) : m_glinfo(_glinfo)
	{
	}

	void write(std::stringstream & shader) const override
	{
		if (!m_glinfo.isGLES2 &&
			config.video.multisampling > 0 &&
			(CombinerProgramBuilder::s_cycleType == G_CYC_COPY || CombinerProgramBuilder::s_textureConvert.useTextureFiltering()))
		{
			shader <<
				"lowp vec4 readTexMS(in lowp sampler2DMS mstex, in highp vec2 texCoord, in lowp int fbMonochrome, in lowp int fbFixedAlpha);\n";
		}
	}

private:
	const opengl::GLInfo& m_glinfo;
};

class ShaderFragmentHeaderReadTexFast : public ShaderPart
{
public:
	ShaderFragmentHeaderReadTexFast(const opengl::GLInfo & _glinfo) : m_glinfo(_glinfo)
	{
	}

	void write(std::stringstream & shader) const override
	{
		std::string shaderPart;
		if (!m_glinfo.isGLES2) {

			if (CombinerProgramBuilder::s_textureConvert.useTextureFiltering()) {
				shaderPart += "uniform lowp int uTextureFilterMode;								\n";
				switch (config.texture.bilinearMode + config.texture.enableHalosRemoval * 2) {
				case BILINEAR_3POINT:
					// 3 point texture filtering.
					// Original author: ArthurCarvalho
					// GLSL implementation: twinaphex, mupen64plus-libretro project.
					shaderPart +=
						"#define TEX_OFFSET(off, tex, texCoord) texture(tex, texCoord - (off)/texSize)			\n"
						"#define TEX_FILTER(name, tex, texCoord)												\\\n"
						"  {																					\\\n"
						"  mediump vec2 texSize = vec2(textureSize(tex,0));										\\\n"
						"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));							\\\n"
						"  offset -= step(1.0, offset.x + offset.y);											\\\n"
						"  lowp vec4 c0 = TEX_OFFSET(offset, tex, texCoord);									\\\n"
						"  lowp vec4 c1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y), tex, texCoord);	\\\n"
						"  lowp vec4 c2 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)), tex, texCoord);	\\\n"
						"  name = c0 + abs(offset.x)*(c1-c0) + abs(offset.y)*(c2-c0); 							\\\n"
						"  }																					\n"
						;
				break;
				case BILINEAR_STANDARD:
					shaderPart +=
						"#define TEX_OFFSET(off, tex, texCoord) texture(tex, texCoord - (off)/texSize)									\n"
						"#define TEX_FILTER(name, tex, texCoord)																		\\\n"
						"{																												\\\n"
						"  mediump vec2 texSize = vec2(textureSize(tex,0));																\\\n"
						"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));													\\\n"
						"  offset -= step(1.0, offset.x + offset.y);																	\\\n"
						"  lowp vec4 zero = vec4(0.0);																					\\\n"
						"																												\\\n"
						"  lowp vec4 p0q0 = TEX_OFFSET(offset, tex, texCoord);															\\\n"
						"  lowp vec4 p1q0 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y), tex, texCoord);						\\\n"
						"																												\\\n"
						"  lowp vec4 p0q1 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)), tex, texCoord);						\\\n"
						"  lowp vec4 p1q1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y - sign(offset.y)), tex, texCoord);		\\\n"
						"																												\\\n"
						"  mediump vec2 interpolationFactor = abs(offset);																\\\n"
						"  lowp vec4 pInterp_q0 = mix( p0q0, p1q0, interpolationFactor.x ); 											\\\n" // Interpolates top row in X direction.
						"  lowp vec4 pInterp_q1 = mix( p0q1, p1q1, interpolationFactor.x ); 											\\\n" // Interpolates bottom row in X direction.
						"  name = mix( pInterp_q0, pInterp_q1, interpolationFactor.y ); 												\\\n" // Interpolate in Y direction.
						"}																												\n"
						;
				break;
				case BILINEAR_3POINT_WITH_COLOR_BLEEDING:
					// 3 point texture filtering.
					// Original author: ArthurCarvalho
					// GLSL implementation: twinaphex, mupen64plus-libretro project.
					shaderPart +=
						"#define TEX_OFFSET(off, tex, texCoord) texture(tex, texCoord - (off)/texSize)									\n"
						"#define TEX_FILTER(name, tex, texCoord)												\\\n"
						"{																						\\\n"
						"  mediump vec2 texSize = vec2(textureSize(tex,0));										\\\n"
						"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));							\\\n"
						"  offset -= step(1.0, offset.x + offset.y);											\\\n"
						"  lowp vec4 c0 = TEX_OFFSET(offset, tex, texCoord);									\\\n"
						"  lowp vec4 c1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y), tex, texCoord);	\\\n"
						"  lowp vec4 c2 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)), tex, texCoord);	\\\n"
						"																						\\\n"
						"  if(uEnableAlphaTest == 1 ){															\\\n" // Calculate premultiplied color values
						"    c0.rgb *= c0.a;																	\\\n"
						"    c1.rgb *= c1.a;																	\\\n"
						"    c2.rgb *= c2.a;																	\\\n"
						"    name = c0 + abs(offset.x)*(c1-c0) + abs(offset.y)*(c2-c0); 						\\\n"
						"    name.rgb /= name.a;																\\\n" // Divide alpha to get actual color value
						"  }																					\\\n"
						"  else name = c0 + abs(offset.x)*(c1-c0) + abs(offset.y)*(c2-c0); 						\\\n"
						"}																						\n"
						;
				break;
				case BILINEAR_STANDARD_WITH_COLOR_BLEEDING_AND_PREMULTIPLIED_ALPHA:
					shaderPart +=
						"#define TEX_OFFSET(off, tex, texCoord) texture(tex, texCoord - (off)/texSize)									\n"
						"#define TEX_FILTER(name, tex, texCoord)																		\\\n"
						"{																												\\\n"
						"  mediump vec2 texSize = vec2(textureSize(tex,0));																\\\n"
						"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));													\\\n"
						"  offset -= step(1.0, offset.x + offset.y);																	\\\n"
						"  lowp vec4 zero = vec4(0.0);																					\\\n"
						"																												\\\n"
						"  lowp vec4 p0q0 = TEX_OFFSET(offset, tex, texCoord);															\\\n"
						"  lowp vec4 p1q0 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y), tex, texCoord);						\\\n"
						"																												\\\n"
						"  lowp vec4 p0q1 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)), tex, texCoord);						\\\n"
						"  lowp vec4 p1q1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y - sign(offset.y)), tex, texCoord);		\\\n"
						"																												\\\n"
						"  if(uEnableAlphaTest == 1){																					\\\n" // Calculate premultiplied color values
						"    p0q0.rgb *= p0q0.a;																						\\\n"
						"    p1q0.rgb *= p1q0.a;																						\\\n"
						"    p0q1.rgb *= p0q1.a;																						\\\n"
						"    p1q1.rgb *= p1q1.a;																						\\\n"
						"																												\\\n"
						"    mediump vec2 interpolationFactor = abs(offset);															\\\n"
						"    lowp vec4 pInterp_q0 = mix( p0q0, p1q0, interpolationFactor.x ); 											\\\n" // Interpolates top row in X direction.
						"    lowp vec4 pInterp_q1 = mix( p0q1, p1q1, interpolationFactor.x ); 											\\\n" // Interpolates bottom row in X direction.
						"    name = mix( pInterp_q0, pInterp_q1, interpolationFactor.y ); 												\\\n" // Interpolate in Y direction.
						"    name.rgb /= name.a;																						\\\n" // Divide alpha to get actual color value
						"  }																											\\\n"
						"  else if(uCvgXAlpha == 1){																					\\\n" // Use texture bleeding for mk64
						"    if(p0q0.a > p1q0.a) p1q0.rgb = p0q0.rgb;																	\\\n"
						"    if(p1q0.a > p0q0.a) p0q0.rgb = p1q0.rgb;																	\\\n"
						"    if(p0q1.a > p1q1.a) p1q1.rgb = p0q1.rgb;																	\\\n"
						"    if(p1q1.a > p0q1.a) p0q1.rgb = p1q1.rgb;																	\\\n"
						"    if(p0q0.a > p0q1.a) p0q1.rgb = p0q0.rgb;																	\\\n"
						"    if(p0q1.a > p0q0.a) p0q0.rgb = p0q1.rgb;																	\\\n"
						"    if(p1q0.a > p1q1.a) p1q1.rgb = p1q0.rgb;																	\\\n"
						"    if(p1q1.a > p1q0.a) p1q0.rgb = p1q1.rgb;																	\\\n"
						"																												\\\n"
						"    mediump vec2 interpolationFactor = abs(offset);															\\\n"
						"    lowp vec4 pInterp_q0 = mix( p0q0, p1q0, interpolationFactor.x );											\\\n" // Interpolates top row in X direction.
						"    lowp vec4 pInterp_q1 = mix( p0q1, p1q1, interpolationFactor.x );											\\\n" // Interpolates bottom row in X direction.
						"    name = mix( pInterp_q0, pInterp_q1, interpolationFactor.y );												\\\n"
						"  }																											\\\n"
						"  else{																										\\\n"
						"    mediump vec2 interpolationFactor = abs(offset);															\\\n"
						"    lowp vec4 pInterp_q0 = mix( p0q0, p1q0, interpolationFactor.x ); 											\\\n" // Interpolates top row in X direction.
						"    lowp vec4 pInterp_q1 = mix( p0q1, p1q1, interpolationFactor.x ); 											\\\n" // Interpolates bottom row in X direction.
						"    name = mix( pInterp_q0, pInterp_q1, interpolationFactor.y ); 												\\\n" // Interpolate in Y direction.
						"  }																											\\\n"
						"}																												\n"
						;
				break;
				}
				shaderPart +=
					"#define READ_TEX(name, tex, texCoord, fbMonochrome, fbFixedAlpha)	\\\n"
					"  {																\\\n"
					"  if (fbMonochrome == 3) {											\\\n"
					"    mediump ivec2 coord = ivec2(gl_FragCoord.xy);					\\\n"
					"    name = texelFetch(tex, coord, 0);								\\\n"
					"  } else {															\\\n"
					"    if (uTextureFilterMode == 0) name = texture(tex, texCoord);	\\\n"
					"    else TEX_FILTER(name, tex, texCoord);			 				\\\n"
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
					"#define YUVCONVERT_TEX0(name, tex, texCoord, format)				\\\n"
					"  {																\\\n"
					"  name = texture(tex, texCoord);									\\\n"
					"  YUVCONVERT(name, format)											\\\n"
					"  }																\n"
					"#define YUVCONVERT_TEX1(name, tex, texCoord, format, prev)			\\\n"
					"  {																\\\n"
					"  if (uTextureConvert != 0) name = prev;							\\\n"
					"  else name = texture(tex, texCoord);								\\\n"
					"  YUVCONVERT(name, format)											\\\n"
					"  }																\n"
					;
			}

		} else {
			if (CombinerProgramBuilder::s_textureConvert.useTextureFiltering()) {
				shaderPart +=
					"uniform lowp int uTextureFilterMode;								\n"
					"lowp vec4 readTex(in sampler2D tex, in highp vec2 texCoord, in lowp int fbMonochrome, in lowp int fbFixedAlpha);	\n"
					;
			}
			if (CombinerProgramBuilder::s_textureConvert.useYUVCoversion()) {
				shaderPart +=
					"uniform lowp ivec2 uTextureFormat;									\n"
					"uniform lowp int uTextureConvert;									\n"
					"uniform mediump ivec4 uConvertParams;								\n"
					"lowp vec4 YUV_Convert(in sampler2D tex, in highp vec2 texCoord, in lowp int convert, in lowp int format, in lowp vec4 prev);	\n"
					;
			}
		}

		shader << shaderPart;
	}

private:
	const opengl::GLInfo& m_glinfo;
};

class ShaderFragmentHeaderReadTexCopyModeFast : public ShaderPart
{
public:
	ShaderFragmentHeaderReadTexCopyModeFast (const opengl::GLInfo & _glinfo)
	{
		if (!_glinfo.isGLES2) {
			m_part =
				"#define READ_TEX(name, tex, texCoord, fbMonochrome, fbFixedAlpha)	\\\n"
				"  {																\\\n"
				"  if (fbMonochrome == 3) {											\\\n"
				"    mediump ivec2 coord = ivec2(gl_FragCoord.xy);					\\\n"
				"    name = texelFetch(tex, coord, 0);								\\\n"
				"  } else {															\\\n"
				"    name = texture(tex, texCoord);									\\\n"
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
				"lowp vec4 readTex(in sampler2D tex, in highp vec2 texCoord, in lowp int fbMonochrome, in lowp int fbFixedAlpha);	\n"
			;
		}
	}
};

class ShaderFragmentClampWrapMirrorTex0 : public ShaderPart
{
public:
	ShaderFragmentClampWrapMirrorTex0(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"  texCoord0 = clampWrapMirror(vTexCoord0, uTexClamp0, uTexWrap0, uTexMirror0, uTexScale0);	\n"
			;
	}
};

class ShaderFragmentClampWrapMirrorTex1 : public ShaderPart
{
public:
	ShaderFragmentClampWrapMirrorTex1(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"  texCoord1 = clampWrapMirror(vTexCoord1, uTexClamp1, uTexWrap1, uTexMirror1, uTexScale1);	\n"
			;
	}
};

class ShaderFragmentCorrectTexCoords : public ShaderPart {
public:
	ShaderFragmentCorrectTexCoords() {
		m_part +=
			" highp vec2 mTexCoord0 = vTexCoord0 + vec2(0.0001);						\n"
			" highp vec2 mTexCoord1 = vTexCoord1 + vec2(0.0001);						\n"
			" mTexCoord0 += uTexCoordOffset[0];											\n"
			" mTexCoord1 += uTexCoordOffset[1];											\n"
			" if (uUseTexCoordBounds != 0) {											\n"
			" mTexCoord0 = clamp(mTexCoord0, uTexCoordBounds0.xy, uTexCoordBounds0.zw); \n"
			" mTexCoord1 = clamp(mTexCoord1, uTexCoordBounds1.xy, uTexCoordBounds1.zw); \n"
			" }																			\n"
			;
	}
};

class ShaderFragmentReadTexCopyModeFast : public ShaderPart
{
public:
	ShaderFragmentReadTexCopyModeFast(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			m_part =
				"  nCurrentTile = 0; \n"
				"  lowp vec4 readtex0 = readTex(uTex0, texCoord0, uFbMonochrome[0], uFbFixedAlpha[0]);		\n"
				;
		} else {
			if (config.video.multisampling > 0) {
				m_part =
					"  lowp vec4 readtex0;																	\n"
					"  if (uMSTexEnabled[0] == 0) {															\n"
					"      READ_TEX(readtex0, uTex0, texCoord0, uFbMonochrome[0], uFbFixedAlpha[0])		\n"
					"  } else readtex0 = readTexMS(uMSTex0, texCoord0, uFbMonochrome[0], uFbFixedAlpha[0]);\n"
					;
			} else {
				m_part =
					"  lowp vec4 readtex0;																	\n"
					"  READ_TEX(readtex0, uTex0, texCoord0, uFbMonochrome[0], uFbFixedAlpha[0])			\n"
					;
			}
		}
	}
};

class ShaderFragmentReadTex0Fast : public ShaderPart
{
public:
	ShaderFragmentReadTex0Fast(const opengl::GLInfo & _glinfo) : m_glinfo(_glinfo)
	{
	}

	void write(std::stringstream & shader) const override
	{
		std::string shaderPart;

		if (m_glinfo.isGLES2) {

			shaderPart = "  nCurrentTile = 0; \n";
			if (CombinerProgramBuilder::s_textureConvert.getBilerp0()) {
				shaderPart += "  lowp vec4 readtex0 = readTex(uTex0, texCoord0, uFbMonochrome[0], uFbFixedAlpha[0]);		\n";
			} else {
				shaderPart += "  lowp vec4 tmpTex = vec4(0.0);																\n"
							  "  lowp vec4 readtex0 = YUV_Convert(uTex0, texCoord0, 0, uTextureFormat[0], tmpTex);			\n";
			}

		} else {

			if (!CombinerProgramBuilder::s_textureConvert.getBilerp0()) {
				shaderPart = "  lowp vec4 readtex0;																			\n"
							 "  YUVCONVERT_TEX0(readtex0, uTex0, texCoord0, uTextureFormat[0])								\n";
			} else {
				if (config.video.multisampling > 0) {
					shaderPart =
						"  lowp vec4 readtex0;																				\n"
						"  if (uMSTexEnabled[0] == 0) {																		\n"
						"    READ_TEX(readtex0, uTex0, texCoord0, uFbMonochrome[0], uFbFixedAlpha[0])						\n"
						"  } else readtex0 = readTexMS(uMSTex0, texCoord0, uFbMonochrome[0], uFbFixedAlpha[0]);			\n";
				} else {
					shaderPart = "  lowp vec4 readtex0;																		\n"
								 "  READ_TEX(readtex0, uTex0, texCoord0, uFbMonochrome[0], uFbFixedAlpha[0])				\n";
				}
			}

		}

		shader << shaderPart;
	}

private:
	const opengl::GLInfo& m_glinfo;
};

class ShaderFragmentReadTex1Fast : public ShaderPart
{
public:
	ShaderFragmentReadTex1Fast(const opengl::GLInfo & _glinfo) : m_glinfo(_glinfo)
	{
	}

	void write(std::stringstream & shader) const override
	{
		std::string shaderPart;

		if (m_glinfo.isGLES2) {

			shaderPart = "  nCurrentTile = 1; \n";

			if (CombinerProgramBuilder::s_textureConvert.getBilerp1()) {
				shaderPart += "  lowp vec4 readtex1 = readTex(uTex1, texCoord1, uFbMonochrome[1], uFbFixedAlpha[1]);				\n";
			} else {
				shaderPart += "  lowp vec4 readtex1 = YUV_Convert(uTex1, texCoord1, uTextureConvert, uTextureFormat[1], readtex0);	\n";
			}

		} else {

			if (!CombinerProgramBuilder::s_textureConvert.getBilerp1()) {
				shaderPart =
					"  lowp vec4 readtex1;																							\n"
					"    YUVCONVERT_TEX1(readtex1, uTex1, texCoord1, uTextureFormat[1], readtex0)					\n";
			} else {
				if (config.video.multisampling > 0) {
					shaderPart =
						"  lowp vec4 readtex1;																						\n"
						"  if (uMSTexEnabled[1] == 0) {																				\n"
						"    READ_TEX(readtex1, uTex1, texCoord1, uFbMonochrome[1], uFbFixedAlpha[1])								\n"
						"  } else readtex1 = readTexMS(uMSTex1, texCoord1, uFbMonochrome[1], uFbFixedAlpha[1]);					\n";
				} else {
					shaderPart = "  lowp vec4 readtex1;																				\n"
								 "  READ_TEX(readtex1, uTex1, texCoord1, uFbMonochrome[1], uFbFixedAlpha[1])						\n";
				}
			}

		}

		shader << shaderPart;
	}

private:
	const opengl::GLInfo& m_glinfo;
};

class ShaderMipmapFast : public ShaderPart
{
public:
	ShaderMipmapFast(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			if (config.generalEmulation.enableLOD == 0) {
				// Fake mipmap
				m_part =
					"uniform lowp int uMaxTile;			\n"
					"uniform mediump float uMinLod;		\n"
					"														\n"
					"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1) {	\n"
					"  readtex0 = texture2D(uTex0, texCoord0);				\n"
					"  readtex1 = texture2D(uTex1, texCoord1);				\n"
					"  if (uMaxTile == 0) return 1.0;						\n"
					"  return uMinLod;										\n"
					"}														\n"
				;
			} else {
				m_part =
					"uniform lowp int uEnableLod;		\n"
					"uniform mediump float uMinLod;		\n"
					"uniform lowp int uMaxTile;			\n"
					"uniform lowp int uTextureDetail;	\n"
					"														\n"
					"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1) {	\n"
					"  readtex0 = texture2D(uTex0, texCoord0);				\n"
					"  readtex1 = texture2DLodEXT(uTex1, texCoord1, 0.0);		\n"
					"														\n"
					"  mediump float fMaxTile = float(uMaxTile);			\n"
					"  mediump vec2 dx = abs(dFdx(vLodTexCoord)) * uScreenScale;	\n"
					"  mediump vec2 dy = abs(dFdy(vLodTexCoord)) * uScreenScale;	\n"
					"  mediump float lod = max(dx.x + dx.y, dy.x + dy.y);	\n" /*LINEAR*/
					"  bool magnify = lod < 1.0;							\n"
					"  mediump float lod_tile = magnify ? 0.0 : floor(log2(floor(lod))); \n"
					"  bool distant = lod > 128.0 || lod_tile >= fMaxTile;	\n"
					"  mediump float lod_frac = fract(lod/pow(2.0, lod_tile));	\n"
					"  if (magnify) lod_frac = max(lod_frac, uMinLod);		\n"
					"  if (uTextureDetail == 0)	{							\n"
					"    if (distant) lod_frac = 1.0;						\n"
					"    else if (magnify) lod_frac = 0.0;					\n"
					"  }													\n"
					"  if (magnify && (uTextureDetail == 1 || uTextureDetail == 3))			\n"
					"      lod_frac = 1.0 - lod_frac;						\n"
					"  if (uMaxTile == 0) {									\n"
					"    if (uEnableLod != 0) {								\n"
					"      if (uTextureDetail < 2)	readtex1 = readtex0;	\n"
					"      else if (!magnify) readtex0 = readtex1;			\n"
					"    }													\n"
					"    return lod_frac;									\n"
					"  }													\n"
					"  if (uEnableLod == 0) return lod_frac;				\n"
					"														\n"
					"  lod_tile = min(lod_tile, fMaxTile);					\n"
					"  lowp float lod_tile_m1 = max(0.0, lod_tile - 1.0);	\n"
					"  lowp float lod_tile_p1 = min(fMaxTile - 1.0, lod_tile + 1.0);	\n"
					"  lowp vec4 lodT = texture2DLodEXT(uTex1, texCoord1, lod_tile);	\n"
					"  lowp vec4 lodT_m1 = texture2DLodEXT(uTex1, texCoord1, lod_tile_m1);	\n"
					"  lowp vec4 lodT_p1 = texture2DLodEXT(uTex1, texCoord1, lod_tile_p1);	\n"
					"  if (lod_tile < 1.0) {								\n"
					"    if (magnify) {									\n"
					//     !sharpen && !detail
					"      if (uTextureDetail == 0) readtex1 = readtex0;	\n"
					"    } else {											\n"
					//     detail
					"      if (uTextureDetail > 1) {						\n"
					"        readtex0 = lodT;								\n"
					"        readtex1 = lodT_p1;							\n"
					"      }												\n"
					"    }													\n"
					"  } else {												\n"
					"    if (uTextureDetail > 1) {							\n"
					"      readtex0 = lodT;									\n"
					"      readtex1 = lodT_p1;								\n"
					"    } else {											\n"
					"      readtex0 = lodT_m1;								\n"
					"      readtex1 = lodT;									\n"
					"    }													\n"
					"  }													\n"
					"  return lod_frac;										\n"
					"}														\n"
				;
			}
		} else {
			if (config.texture.bilinearMode == BILINEAR_3POINT) {
				m_part =
					"#define TEX_OFFSET_NORMAL(off, tex, texCoord, lod) texture(tex, texCoord - (off)/texSize)			\n"
					"#define READ_TEX_NORMAL(name, tex, texCoord, lod)													\\\n"
					"  {																								\\\n"
					"  mediump vec2 texSize = vec2(textureSize(tex, int(lod)));											\\\n"
					"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));										\\\n"
					"  offset -= step(1.0, offset.x + offset.y);														\\\n"
					"  lowp vec4 c0 = TEX_OFFSET_NORMAL(offset, tex, texCoord, lod);									\\\n"
					"  lowp vec4 c1 = TEX_OFFSET_NORMAL(vec2(offset.x - sign(offset.x), offset.y), tex, texCoord, lod);	\\\n"
					"  lowp vec4 c2 = TEX_OFFSET_NORMAL(vec2(offset.x, offset.y - sign(offset.y)), tex, texCoord, lod);	\\\n"
					"  name = c0 + abs(offset.x)*(c1-c0) + abs(offset.y)*(c2-c0); 										\\\n"
					"  }																								\n"
					;
				if (config.generalEmulation.enableLOD != 0) {
					m_part +=
					"#define TEX_OFFSET_MIPMAP(off, tex, texCoord, lod) textureLod(tex, texCoord - (off)/texSize, lod)	\n"
					"#define READ_TEX_MIPMAP(name, tex, texCoord, lod)													\\\n"
					"  {																								\\\n"
					"  mediump vec2 texSize = vec2(textureSize(tex, int(lod)));											\\\n"
					"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));										\\\n"
					"  offset -= step(1.0, offset.x + offset.y);														\\\n"
					"  lowp vec4 c0 = TEX_OFFSET_MIPMAP(offset, tex, texCoord, lod);									\\\n"
					"  lowp vec4 c1 = TEX_OFFSET_MIPMAP(vec2(offset.x - sign(offset.x), offset.y), tex, texCoord, lod);	\\\n"
					"  lowp vec4 c2 = TEX_OFFSET_MIPMAP(vec2(offset.x, offset.y - sign(offset.y)), tex, texCoord, lod);	\\\n"
					"  name = c0 + abs(offset.x)*(c1-c0) + abs(offset.y)*(c2-c0); 										\\\n"
					"  }																								\n"
					;
				}
			} else {
					m_part =
						"#define TEX_OFFSET_NORMAL(off, tex, texCoord, lod) texture(tex, texCoord - (off)/texSize)						\n"
						"#define READ_TEX_NORMAL(name, tex, texCoord, lod)																\\\n"
						"  {																											\\\n"
						"  mediump vec2 texSize = vec2(textureSize(tex, int(lod)));														\\\n"
						"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));													\\\n"
						"  offset -= step(1.0, offset.x + offset.y);																	\\\n"
						"  lowp vec4 zero = vec4(0.0);																					\\\n"
						"																												\\\n"
						"  lowp vec4 p0q0 = TEX_OFFSET_NORMAL(offset, tex, texCoord, lod);												\\\n"
						"  lowp vec4 p1q0 = TEX_OFFSET_NORMAL(vec2(offset.x - sign(offset.x), offset.y), tex, texCoord, lod);			\\\n"
						"																												\\\n"
						"  lowp vec4 p0q1 = TEX_OFFSET_NORMAL(vec2(offset.x, offset.y - sign(offset.y)), tex, texCoord, lod);			\\\n"
						"  lowp vec4 p1q1 = TEX_OFFSET_NORMAL(vec2(offset.x - sign(offset.x), offset.y - sign(offset.y)), tex, texCoord, lod);	\\\n"
						"																												\\\n"
						"  mediump vec2 interpolationFactor = abs(offset);																\\\n"
						"  lowp vec4 pInterp_q0 = mix( p0q0, p1q0, interpolationFactor.x ); 											\\\n"
						"  lowp vec4 pInterp_q1 = mix( p0q1, p1q1, interpolationFactor.x ); 											\\\n"
						"  name = mix( pInterp_q0, pInterp_q1, interpolationFactor.y ); 												\\\n"
						"}																												\n"
						;
					if (config.generalEmulation.enableLOD != 0) {
						m_part +=
						"#define TEX_OFFSET_MIPMAP(off, tex, texCoord, lod) textureLod(tex, texCoord - (off)/texSize, lod)				\n"
						"#define READ_TEX_MIPMAP(name, tex, texCoord, lod)																\\\n"
						"  {																											\\\n"
						"  mediump vec2 texSize = vec2(textureSize(tex, int(lod)));														\\\n"
						"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));													\\\n"
						"  offset -= step(1.0, offset.x + offset.y);																	\\\n"
						"  lowp vec4 zero = vec4(0.0);																					\\\n"
						"																												\\\n"
						"  lowp vec4 p0q0 = TEX_OFFSET_MIPMAP(offset, tex, texCoord, lod);												\\\n"
						"  lowp vec4 p1q0 = TEX_OFFSET_MIPMAP(vec2(offset.x - sign(offset.x), offset.y), tex, texCoord, lod);			\\\n"
						"																												\\\n"
						"  lowp vec4 p0q1 = TEX_OFFSET_MIPMAP(vec2(offset.x, offset.y - sign(offset.y)), tex, texCoord, lod);			\\\n"
						"  lowp vec4 p1q1 = TEX_OFFSET_MIPMAP(vec2(offset.x - sign(offset.x), offset.y - sign(offset.y)), tex, texCoord, lod);	\\\n"
						"																												\\\n"
						"  mediump vec2 interpolationFactor = abs(offset);																\\\n"
						"  lowp vec4 pInterp_q0 = mix( p0q0, p1q0, interpolationFactor.x ); 											\\\n"
						"  lowp vec4 pInterp_q1 = mix( p0q1, p1q1, interpolationFactor.x ); 											\\\n"
						"  name = mix( pInterp_q0, pInterp_q1, interpolationFactor.y ); 												\\\n"
						"}																												\n"
						;
					}
			}

			if (config.generalEmulation.enableLOD == 0) {
				// Fake mipmap
				m_part +=
					"uniform lowp int uMaxTile;			\n"
					"uniform mediump float uMinLod;		\n"
					"														\n"
					"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1) {	\n"
					"  READ_TEX_NORMAL(readtex0, uTex0, texCoord0, 0.0);	\n"
					"  READ_TEX_NORMAL(readtex1, uTex1, texCoord1, 0.0);	\n"
					"  if (uMaxTile == 0) return 1.0;						\n"
					"  return uMinLod;										\n"
					"}														\n"
				;
			} else {
				m_part +=
					"uniform lowp int uEnableLod;		\n"
					"uniform mediump float uMinLod;		\n"
					"uniform lowp int uMaxTile;			\n"
					"uniform lowp int uTextureDetail;	\n"
					"																		\n"
					"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1) {	\n"
					"  READ_TEX_NORMAL(readtex0, uTex0, texCoord0, 0.0);					\n"
					"  READ_TEX_MIPMAP(readtex1, uTex1, texCoord1, 0.0);					\n"
					"																		\n"
					"  mediump float fMaxTile = float(uMaxTile);							\n"
					"  mediump vec2 dx = abs(dFdx(vLodTexCoord));							\n"
					"  dx *= uScreenScale;													\n"
					"  mediump float lod = max(dx.x, dx.y);									\n"
					"  bool magnify = lod < 1.0;											\n"
					"  mediump float lod_tile = magnify ? 0.0 : floor(log2(floor(lod)));	\n"
					"  bool distant = lod > 128.0 || lod_tile >= fMaxTile;					\n"
					"  mediump float lod_frac = fract(lod/pow(2.0, lod_tile));				\n"
					"  if (magnify) lod_frac = max(lod_frac, uMinLod);						\n"
					"  if (uTextureDetail == 0)	{											\n"
					"    if (distant) lod_frac = 1.0;										\n"
					"    else if (magnify) lod_frac = 0.0;									\n"
					"  }																	\n"
					"  if (magnify && ((uTextureDetail & 1) != 0))							\n"
					"      lod_frac = 1.0 - lod_frac;										\n"
					"  if (uMaxTile == 0) {													\n"
					"    if (uEnableLod != 0) {												\n"
					"      if ((uTextureDetail & 2) == 0) readtex1 = readtex0;				\n"
					"      else if (!magnify) readtex0 = readtex1;							\n"
					"    }																	\n"
					"    return lod_frac;													\n"
					"  }																	\n"
					"  if (uEnableLod == 0) return lod_frac;								\n"
					"																		\n"
					"  lod_tile = min(lod_tile, fMaxTile - 1.0);							\n"
					"  lowp float lod_tile_m1 = max(0.0, lod_tile - 1.0);					\n"
					"  lowp float lod_tile_p1 = min(fMaxTile - 1.0, lod_tile + 1.0);		\n"
					"  lowp vec4 lodT, lodT_m1, lodT_p1;									\n"
					"  READ_TEX_MIPMAP(lodT, uTex1, texCoord1, lod_tile);					\n"
					"  READ_TEX_MIPMAP(lodT_m1, uTex1, texCoord1, lod_tile_m1);				\n"
					"  READ_TEX_MIPMAP(lodT_p1, uTex1, texCoord1, lod_tile_p1);				\n"
					"  if (lod_tile < 1.0) {												\n"
					"    if (magnify) {														\n"
					//     !sharpen && !detail
					"      if (uTextureDetail == 0) readtex1 = readtex0;					\n"
					"    } else {															\n"
					//     detail
					"      if ((uTextureDetail & 2) != 0 ) {								\n"
					"        readtex0 = lodT;												\n"
					"        readtex1 = lodT_p1;											\n"
					"      }																\n"
					"    }																	\n"
					"  } else {																\n"
					"    if ((uTextureDetail & 2) != 0 ) {									\n"
					"      readtex0 = lodT;													\n"
					"      readtex1 = lodT_p1;												\n"
					"    } else {															\n"
					"      readtex0 = lodT_m1;												\n"
					"      readtex1 = lodT;													\n"
					"    }																	\n"
					"  }																	\n"
					"  return lod_frac;														\n"
					"}																		\n"
					;
			}
		}
	}
};

class ShaderReadtexFast : public ShaderPart
{
public:
	ShaderReadtexFast(const opengl::GLInfo & _glinfo) : m_glinfo(_glinfo)
	{
	}

	void write(std::stringstream & shader) const override
	{
		std::string shaderPart;

		if (m_glinfo.isGLES2) {
			if (CombinerProgramBuilder::s_textureConvert.useYUVCoversion())
				shaderPart +=
				"lowp vec4 YUV_Convert(in sampler2D tex, in highp vec2 texCoord, in lowp int convert, in lowp int format, in lowp vec4 prev)	\n"
				"{																	\n"
				"  lowp vec4 texColor;												\n"
				"  if (convert != 0) texColor = prev;								\n"
				"  else texColor = texture2D(tex, texCoord);						\n"
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
						"uniform mediump vec2 uTextureSize[2];										\n"
						// 3 point texture filtering.
						// Original author: ArthurCarvalho
						// GLSL implementation: twinaphex, mupen64plus-libretro project.
						"#define TEX_OFFSET(off) texture2D(tex, texCoord - (off)/texSize)			\n"
						"lowp vec4 TextureFilter(in sampler2D tex, in highp vec2 texCoord)		\n"
						"{																			\n"
						"  mediump vec2 texSize;													\n"
						"  if (nCurrentTile == 0)													\n"
						"    texSize = uTextureSize[0];												\n"
						"  else																		\n"
						"    texSize = uTextureSize[1];												\n"
						"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));				\n"
						"  offset -= step(1.0, offset.x + offset.y);								\n"
						"  lowp vec4 c0 = TEX_OFFSET(offset);										\n"
						"  lowp vec4 c1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y));	\n"
						"  lowp vec4 c2 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)));	\n"
						"  return c0 + abs(offset.x)*(c1-c0) + abs(offset.y)*(c2-c0);				\n"
						"}																			\n"
						;
				} else {
					shaderPart +=
						// bilinear filtering.
						"uniform mediump vec2 uTextureSize[2];										\n"
						"#define TEX_OFFSET(off) texture2D(tex, texCoord - (off)/texSize)			\n"
						"lowp vec4 TextureFilter(in sampler2D tex, in highp vec2 texCoord)		\n"
						"{																			\n"
						"  mediump vec2 texSize;													\n"
						"  if (nCurrentTile == 0)													\n"
						"    texSize = uTextureSize[0];												\n"
						"  else																		\n"
						"    texSize = uTextureSize[1];												\n"
						"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));				\n"
						"  offset -= step(1.0, offset.x + offset.y);								\n"
						"  lowp vec4 zero = vec4(0.0);												\n"
						"																			\n"
						"  lowp vec4 p0q0 = TEX_OFFSET(offset);										\n"
						"  lowp vec4 p1q0 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y));	\n"
						"																			\n"
						"  lowp vec4 p0q1 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)));	\n"
						"  lowp vec4 p1q1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y - sign(offset.y)));\n"
						"																			\n"
						"  mediump vec2 interpolationFactor = abs(offset);							\n"
						"  lowp vec4 pInterp_q0 = mix( p0q0, p1q0, interpolationFactor.x ); 		\n" // Interpolates top row in X direction.
						"  lowp vec4 pInterp_q1 = mix( p0q1, p1q1, interpolationFactor.x ); 		\n" // Interpolates bottom row in X direction.
						"  return mix( pInterp_q0, pInterp_q1, interpolationFactor.y ); 			\n" // Interpolate in Y direction.
						"}																			\n"
						;
				}
				shaderPart +=
					"lowp vec4 readTex(in sampler2D tex, in highp vec2 texCoord, in lowp int fbMonochrome, in lowp int fbFixedAlpha)	\n"
					"{																			\n"
					"  lowp vec4 texColor;														\n"
					"  if (uTextureFilterMode == 0) texColor = texture2D(tex, texCoord);		\n"
					"  else texColor = TextureFilter(tex, texCoord);							\n"
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
					"lowp vec4 readTexMS(in lowp sampler2DMS mstex, in highp vec2 texCoord, in lowp int fbMonochrome, in lowp int fbFixedAlpha)	\n"
					"{																			\n"
					"  mediump ivec2 itexCoord;													\n"
					"  if (fbMonochrome == 3) {													\n"
					"    itexCoord = ivec2(gl_FragCoord.xy);									\n"
					"  } else {																	\n"
					"    mediump vec2 msTexSize = vec2(textureSize(mstex));						\n"
					"    itexCoord = ivec2(msTexSize * texCoord);								\n"
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

class ShaderReadtexCopyModeFast : public ShaderPart
{
public:
	ShaderReadtexCopyModeFast(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			m_part =
					"lowp vec4 readTex(in sampler2D tex, in highp vec2 texCoord, in lowp int fbMonochrome, in lowp int fbFixedAlpha)	\n"
					"{																			\n"
					"  lowp vec4 texColor = texture2D(tex, texCoord);							\n"
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
					"lowp vec4 readTexMS(in lowp sampler2DMS mstex, in highp vec2 texCoord, in lowp int fbMonochrome, in lowp int fbFixedAlpha)	\n"
					"{																			\n"
					"  mediump ivec2 itexCoord;													\n"
					"  if (fbMonochrome == 3) {													\n"
					"    itexCoord = ivec2(gl_FragCoord.xy);									\n"
					"  } else {																	\n"
					"    mediump vec2 msTexSize = vec2(textureSize(mstex));						\n"
					"    itexCoord = ivec2(msTexSize * texCoord);								\n"
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

class ShaderClampWrapMirror : public ShaderPart
{
public:
	ShaderClampWrapMirror(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"highp vec2 clampWrapMirror(in highp vec2 vTexCoord, in highp vec4 vClamp,		\n"
			"	in highp vec2 vWrap, in lowp vec2 vMirror, in highp vec2 vScale)			\n"
			"{																				\n"
			"  highp vec2 texCoord = clamp(vTexCoord, vClamp.xy, vClamp.zw);				\n"
			"  lowp vec2 one = vec2(1.0);													\n"
			"  lowp vec2 clamped = step(vClamp.zw, texCoord);								\n"
			"  lowp vec2 notClamped = one - clamped;										\n"
			"  lowp vec2 wrapped = step(vWrap , texCoord);									\n"
			"  lowp vec2 notWrapped = one - wrapped;										\n"
			"  texCoord = clamped * texCoord + notClamped * (wrapped*mod(texCoord, vWrap) + notWrapped*texCoord);			\n"
			"  highp vec2 intPart = floor(texCoord);										\n"
			"  highp vec2 fractPart = fract(texCoord);										\n"
			"  lowp vec2 needMirror = step(vec2(0.5), mod(intPart, vWrap)) * vMirror;	\n"
			"  texCoord = clamped * texCoord + notClamped * fractPart;						\n"
			"  texCoord = (one - vMirror) * texCoord + vMirror * fractPart;					\n"
			"  texCoord = (one - texCoord) * needMirror + texCoord * (one - needMirror);	\n"
			"  texCoord *= vScale;															\n"
			"  return texCoord;																\n"
			"}																				\n"
			;
	}
};

} // nameless namespace

namespace glsl {

CombinerProgramBuilderFast::CombinerProgramBuilderFast(const opengl::GLInfo & _glinfo, opengl::CachedUseProgram * _useProgram)
: CombinerProgramBuilderCommon(_glinfo, _useProgram, std::make_unique<CombinerProgramUniformFactoryFast>(_glinfo))
, m_vertexTexturedTriangle(new VertexShaderTexturedTriangleFast(_glinfo))
, m_vertexTexturedRect(new VertexShaderTexturedRectFast(_glinfo))
, m_fragmentGlobalVariablesTex(new ShaderFragmentGlobalVariablesTexFast(_glinfo))
, m_fragmentHeaderClampWrapMirror(new ShaderFragmentHeaderClampWrapMirror(_glinfo))
, m_fragmentHeaderReadMSTex(new ShaderFragmentHeaderReadMSTexFast(_glinfo))
, m_fragmentHeaderReadTex(new ShaderFragmentHeaderReadTexFast(_glinfo))
, m_fragmentHeaderReadTexCopyMode(new ShaderFragmentHeaderReadTexCopyModeFast(_glinfo))
, m_fragmentReadTex0(new ShaderFragmentReadTex0Fast(_glinfo))
, m_fragmentReadTex1(new ShaderFragmentReadTex1Fast(_glinfo))
, m_fragmentClampWrapMirrorTex0(new ShaderFragmentClampWrapMirrorTex0(_glinfo))
, m_fragmentClampWrapMirrorTex1(new ShaderFragmentClampWrapMirrorTex1(_glinfo))
, m_fragmentCorrectTexCoords(new ShaderFragmentCorrectTexCoords())
, m_fragmentReadTexCopyMode(new ShaderFragmentReadTexCopyModeFast(_glinfo))
, m_shaderMipmap(new ShaderMipmapFast(_glinfo))
, m_shaderReadtex(new ShaderReadtexFast(_glinfo))
, m_shaderReadtexCopyMode(new ShaderReadtexCopyModeFast(_glinfo))
, m_shaderClampWrapMirror(new ShaderClampWrapMirror(_glinfo))
{
}

const ShaderPart * CombinerProgramBuilderFast::getVertexShaderTexturedRect() const
{
	return m_vertexTexturedRect.get();
}

const ShaderPart * CombinerProgramBuilderFast::getVertexShaderTexturedTriangle() const
{
	return m_vertexTexturedTriangle.get();
}

void CombinerProgramBuilderFast::_writeFragmentGlobalVariablesTex(std::stringstream& ssShader) const
{
	 m_fragmentGlobalVariablesTex->write(ssShader);
}

void CombinerProgramBuilderFast::_writeFragmentHeaderReadMSTex(std::stringstream& ssShader) const
{
	 m_fragmentHeaderReadMSTex->write(ssShader);
}

void CombinerProgramBuilderFast::_writeFragmentHeaderReadTex(std::stringstream& ssShader) const
{
	 m_fragmentHeaderReadTex->write(ssShader);
}

void CombinerProgramBuilderFast::_writeFragmentHeaderReadTexCopyMode(std::stringstream& ssShader) const
{
	 m_fragmentHeaderReadTexCopyMode->write(ssShader);
}

void CombinerProgramBuilderFast::_writeFragmentHeaderClampWrapMirrorEngine(std::stringstream& ssShader) const
{
	m_fragmentHeaderClampWrapMirror->write(ssShader);
}

void CombinerProgramBuilderFast::_writeFragmentClampWrapMirrorEngineTex0(std::stringstream& ssShader) const
{
	m_fragmentClampWrapMirrorTex0->write(ssShader);
}

void CombinerProgramBuilderFast::_writeFragmentClampWrapMirrorEngineTex1(std::stringstream& ssShader) const
{
	m_fragmentClampWrapMirrorTex1->write(ssShader);
}

void CombinerProgramBuilderFast::_writeFragmentCorrectTexCoords(std::stringstream& ssShader)const
{
	m_fragmentCorrectTexCoords->write(ssShader);
}
void CombinerProgramBuilderFast::_writeFragmentReadTex0(std::stringstream& ssShader) const
{
	m_fragmentReadTex0->write(ssShader);
}

void CombinerProgramBuilderFast::_writeFragmentReadTex1(std::stringstream& ssShader) const
{
	m_fragmentReadTex1->write(ssShader);
}

void CombinerProgramBuilderFast::_writeFragmentReadTexCopyMode(std::stringstream& ssShader) const
{
	 m_fragmentReadTexCopyMode->write(ssShader);
}

void CombinerProgramBuilderFast::_writeShaderClampWrapMirrorEngine(std::stringstream& ssShader) const
{
	m_shaderClampWrapMirror->write(ssShader);
}

void CombinerProgramBuilderFast::_writeShaderMipmap(std::stringstream& ssShader) const
{
	 m_shaderMipmap->write(ssShader);
}

void CombinerProgramBuilderFast::_writeShaderReadtex(std::stringstream& ssShader) const
{
	 m_shaderReadtex->write(ssShader);
}

void CombinerProgramBuilderFast::_writeShaderReadtexCopyMode(std::stringstream& ssShader) const
{
	 m_shaderReadtexCopyMode->write(ssShader);
}

}
