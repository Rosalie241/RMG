#pragma once
#include <memory>
#include <Combiner.h>
#include <Graphics/OpenGLContext/opengl_GLInfo.h>
#include "glsl_CombinerProgramUniformFactory.h"
#include "glsl_ShaderPart.h"

namespace graphics {
	class CombinerProgram;
}

namespace opengl {
	class CachedUseProgram;
}

namespace glsl {
	class CombinerInputs;
}

namespace glsl {

class TextureConvert {
public:
	void setMode(u32 _mode) {
		m_mode = _mode;
	}

	bool getBilerp1() const {
		return (m_mode & 1) != 0;
	}

	bool getBilerp0() const {
		return (m_mode & 2) != 0;
	}

	bool useYUVCoversion() const {
		return (m_mode & 3) != 3;
	}

	bool useTextureFiltering() const {
		return (m_mode & 3) != 0;
	}

private:
	u32 m_mode;
};

class CombinerProgramBuilder
{
public:
	using ShaderPartPtr = std::unique_ptr<ShaderPart>;

	CombinerProgramBuilder(const opengl::GLInfo & _glinfo, opengl::CachedUseProgram * _useProgram,
		std::unique_ptr<CombinerProgramUniformFactory> _uniformFactory);
	virtual ~CombinerProgramBuilder();

	graphics::CombinerProgram * buildCombinerProgram(Combiner & _color, Combiner & _alpha, const CombinerKey & _key);

	virtual const ShaderPart * getVertexShaderHeader() const = 0;

	virtual const ShaderPart * getFragmentShaderHeader() const = 0;

	virtual const ShaderPart * getFragmentShaderEnd() const = 0;

	virtual bool isObsolete() const = 0;

	static u32 s_cycleType;
	static TextureConvert s_textureConvert;

protected:
	virtual const ShaderPart * getVertexShaderTexturedRect() const = 0;
	virtual const ShaderPart * getVertexShaderTexturedTriangle() const = 0;

private:
	CombinerInputs compileCombiner(const CombinerKey & _key, Combiner & _color, Combiner & _alpha, std::string & _strShader);

	virtual void _writeSignExtendAlphaC(std::stringstream& ssShader) const = 0;
	virtual void _writeSignExtendAlphaABD(std::stringstream& ssShader) const = 0;
	virtual void _writeAlphaTest(std::stringstream& ssShader) const = 0;
	virtual void _writeSignExtendColorC(std::stringstream& ssShader) const = 0;
	virtual void _writeSignExtendColorABD(std::stringstream& ssShader) const = 0;
	virtual void _writeClamp(std::stringstream& ssShader) const = 0;
	virtual void _writeCallDither(std::stringstream& ssShader) const = 0;
	virtual void _writeBlender1(std::stringstream& ssShader) const = 0;
	virtual void _writeBlender2(std::stringstream& ssShader) const = 0;
	virtual void _writeBlenderAlpha(std::stringstream& ssShader) const = 0;
	virtual void _writeLegacyBlender(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentHeader(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentGlobalVariablesTex(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentHeaderDither(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentHeaderNoise(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentHeaderWriteDepth(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentHeaderDepthCompare(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentHeaderReadMSTex(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentHeaderClampWrapMirrorEngine(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentHeaderMipMap(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentHeaderReadTex(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentHeaderReadTexCopyMode(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentGlobalVariablesNotex(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentHeaderCalcLight(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentMain2Cycle(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentMain(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentBlendMux(std::stringstream& ssShader) const = 0;
	virtual void _writeShaderCoverage(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentCorrectTexCoords(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentClampWrapMirrorEngineTex0(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentClampWrapMirrorEngineTex1(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentReadTexMipmap(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentReadTex0(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentReadTex1(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentReadTexCopyMode(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentCallN64Depth(std::stringstream& ssShader) const = 0;
	virtual void _writeFragmentRenderTarget(std::stringstream& ssShader) const = 0;
	virtual void _writeShaderFragmentMainEnd(std::stringstream& ssShader) const = 0;
	virtual void _writeShaderCalcLight(std::stringstream& ssShader) const = 0;
	virtual void _writeShaderClampWrapMirrorEngine(std::stringstream& ssShader) const = 0;
	virtual void _writeShaderMipmap(std::stringstream& ssShader) const = 0;
	virtual void _writeShaderReadtex(std::stringstream& ssShader) const = 0;
	virtual void _writeShaderReadtexCopyMode(std::stringstream& ssShader) const = 0;
	virtual void _writeShaderNoise(std::stringstream& ssShader) const = 0;
	virtual void _writeShaderDither(std::stringstream& ssShader) const = 0;
	virtual void _writeShaderWriteDepth(std::stringstream& ssShader) const = 0;
	virtual void _writeShaderN64DepthCompare(std::stringstream& ssShader) const = 0;
	virtual void _writeShaderN64DepthRender(std::stringstream& ssShader) const = 0;

	virtual GLuint _getVertexShaderRect() const = 0;
	virtual GLuint _getVertexShaderTriangle() const = 0;
	virtual GLuint _getVertexShaderTexturedRect() const = 0;
	virtual GLuint _getVertexShaderTexturedTriangle() const = 0;

	std::unique_ptr<CombinerProgramUniformFactory> m_uniformFactory;
	opengl::CachedUseProgram * m_useProgram;
	bool m_useCoverage = false;
};

}
