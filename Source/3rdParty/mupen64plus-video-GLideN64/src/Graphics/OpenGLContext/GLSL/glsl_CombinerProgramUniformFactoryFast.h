#pragma once
#include "glsl_CombinerProgramImpl.h"
#include "glsl_CombinerProgramUniformFactoryCommon.h"

namespace glsl {

class CombinerProgramUniformFactoryFast : public CombinerProgramUniformFactoryCommon
{
public:
	CombinerProgramUniformFactoryFast(const opengl::GLInfo & _glInfo);
private:

	void _addRasterInfo(GLuint _program, UniformGroups &_uniforms) const override;

	void _addMipmap(GLuint _program, UniformGroups &_uniforms) const override;

	void _addMipmap2(GLuint _program, UniformGroups &_uniforms) const override;

	void _addTextureSize(GLuint _program, UniformGroups &_uniforms, bool _usesTile0, bool _usesTile1) const override;

	void _addTextureParams(GLuint _program, UniformGroups &_uniforms, bool _usesTile0, bool _usesTile1) const override;

	void _addClampWrapMirrorEngine(GLuint _program, UniformGroups &_uniforms, bool _usesTile0, bool _usesTile1) const override;
};

}
