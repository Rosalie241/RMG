#include "glsl_CombinerProgramUniformFactory.h"

#include <Config.h>
#include <Graphics/Parameters.h>
#include <Graphics/Context.h>

namespace glsl {

CombinerProgramUniformFactory::CombinerProgramUniformFactory(const opengl::GLInfo &_glInfo)
: m_glInfo(_glInfo) {
}

CombinerProgramUniformFactory::~CombinerProgramUniformFactory()
{
}

void CombinerProgramUniformFactory::buildUniforms(GLuint _program,
												  const CombinerInputs &_inputs,
												  const CombinerKey &_key,
												  UniformGroups &_uniforms) {

	_addNoiseTex(_program, _uniforms);
	_addScreenSpaceTriangleInfo(_program, _uniforms);
	_addRasterInfo(_program, _uniforms);
	_addViewportInfo(_program, _uniforms);

	if (!m_glInfo.isGLES2) {
		_addDepthTex(_program, _uniforms);
		_addDepthScale(_program, _uniforms);
	}

	if (_inputs.usesTexture()) {
		_addTextures(_program, _uniforms);

		if (config.video.multisampling != 0)
			_addMSAATextures(_program, _uniforms);

		_addFrameBufferInfo(_program, _uniforms);

		if (_inputs.usesLOD()) {
			_addMipmap(_program, _uniforms);
			if (config.generalEmulation.enableLOD != 0)
				_addMipmap2(_program, _uniforms);
		} else if (_key.getCycleType() < G_CYC_COPY) {
			_addTextureFetchMode(_program, _uniforms);
		}

		_addTexturePersp(_program, _uniforms);

		if (m_glInfo.isGLES2)
			_addTextureSize(_program, _uniforms, _inputs.usesTile(0), _inputs.usesTile(1));

		if (!_key.isRectKey())
			_addTextureParams(_program, _uniforms, _inputs.usesTile(0), _inputs.usesTile(1));

		_addClampWrapMirrorEngine(_program, _uniforms, _inputs.usesTile(0), _inputs.usesTile(1));
	}

	_addFog(_program, _uniforms);

	if (config.generalEmulation.enableLegacyBlending == 0) {
		switch (_key.getCycleType()) {
			case G_CYC_1CYCLE:
				_addBlendMode1Cycle(_program, _uniforms);
				break;
			case G_CYC_2CYCLE:
				_addBlendMode2Cycle(_program, _uniforms);
				break;
		}
	}

	_addBlendCvg(_program, _uniforms);

	_addDitherMode(_program, _uniforms, _inputs.usesNoise());

	_addScreenScale(_program, _uniforms);

	_addAlphaTestInfo(_program, _uniforms);

	if ((config.generalEmulation.hacks & hack_RE2) != 0 && config.generalEmulation.enableFragmentDepthWrite != 0)
		_addZLutTexture(_program, _uniforms);

	if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable)
		_addDepthInfo(_program, _uniforms);
	else
		_addDepthSource(_program, _uniforms);

	if (config.generalEmulation.enableFragmentDepthWrite != 0 ||
		config.frameBufferEmulation.N64DepthCompare != Config::dcDisable)
		_addRenderTarget(_program, _uniforms);

	if (m_glInfo.isGLESX && m_glInfo.noPerspective) {
		_addClampMode(_program, _uniforms);
		_addPolygonOffset(_program, _uniforms);
	}

	_addScreenCoordsScale(_program, _uniforms);

	_addColors(_program, _uniforms);

	if (_key.isRectKey())
		_addRectColor(_program, _uniforms);

	if (_inputs.usesHwLighting())
		_addLights(_program, _uniforms);
}

}
