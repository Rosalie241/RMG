#pragma once
#include <Graphics/OpenGLContext/opengl_GLInfo.h>
#include "glsl_CombinerProgramImpl.h"

namespace glsl {

class CombinerProgramUniformFactory {
public:
	CombinerProgramUniformFactory(const opengl::GLInfo & _glInfo);
	virtual ~CombinerProgramUniformFactory();

	void buildUniforms(GLuint _program,
			const CombinerInputs &_inputs,
			const CombinerKey &_key,
			UniformGroups &_uniforms);

private:
	virtual void _addNoiseTex(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addScreenSpaceTriangleInfo(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addRasterInfo(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addViewportInfo(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addDepthTex(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addDepthScale(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addTextures(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addMSAATextures(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addFrameBufferInfo(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addMipmap(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addMipmap2(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addTextureFetchMode(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addTexturePersp(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addTextureSize(GLuint _program, UniformGroups &_uniforms, bool _usesTile0, bool _usesTile1) const = 0;

	virtual void _addTextureParams(GLuint _program, UniformGroups &_uniforms, bool _usesTile0, bool _usesTile1) const = 0;

	virtual void _addClampWrapMirrorEngine(GLuint _program, UniformGroups &_uniforms, bool _usesTile0, bool _usesTile1) const = 0;

	virtual void _addFog(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addBlendMode1Cycle(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addBlendMode2Cycle(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addBlendCvg(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addDitherMode(GLuint _program, UniformGroups &_uniforms, bool _usesNoise) const = 0;

	virtual void _addScreenScale(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addAlphaTestInfo(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addZLutTexture(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addDepthInfo(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addDepthSource(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addRenderTarget(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addClampMode(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addPolygonOffset(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addScreenCoordsScale(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addColors(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addRectColor(GLuint _program, UniformGroups &_uniforms) const = 0;

	virtual void _addLights(GLuint _program, UniformGroups &_uniforms) const = 0;

protected:
	const opengl::GLInfo & m_glInfo;
};
}
