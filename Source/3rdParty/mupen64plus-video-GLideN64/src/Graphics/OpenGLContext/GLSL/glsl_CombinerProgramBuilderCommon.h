#pragma once
#include "glsl_CombinerProgramBuilder.h"

namespace glsl {

class CombinerProgramBuilderCommon : public glsl::CombinerProgramBuilder
{
public:
	CombinerProgramBuilderCommon(const opengl::GLInfo & _glinfo, opengl::CachedUseProgram * _useProgram,
		std::unique_ptr<CombinerProgramUniformFactory> _uniformFactory);
	~CombinerProgramBuilderCommon();

	const ShaderPart * getVertexShaderHeader() const override;

	const ShaderPart * getFragmentShaderHeader() const override;

	const ShaderPart * getFragmentShaderEnd() const override;

	bool isObsolete() const override;

private:

	GLuint _getVertexShaderRect() const override;
	GLuint _getVertexShaderTriangle() const override;
	GLuint _getVertexShaderTexturedRect() const override;
	GLuint _getVertexShaderTexturedTriangle() const override;

	void _writeSignExtendAlphaC(std::stringstream& ssShader) const override;
	void _writeSignExtendAlphaABD(std::stringstream& ssShader) const override;
	void _writeAlphaTest(std::stringstream& ssShader) const override;
	void _writeSignExtendColorC(std::stringstream& ssShader) const override;
	void _writeSignExtendColorABD(std::stringstream& ssShader) const override;
	void _writeClamp(std::stringstream& ssShader) const override;
	void _writeCallDither(std::stringstream& ssShader) const override;
	void _writeBlender1(std::stringstream& ssShader) const override;
	void _writeBlender2(std::stringstream& ssShader) const override;
	void _writeBlenderAlpha(std::stringstream& ssShader) const override;
	void _writeLegacyBlender(std::stringstream& ssShader) const override;
	void _writeFragmentHeader(std::stringstream& ssShader) const override;
	void _writeFragmentHeaderDither(std::stringstream& ssShader) const override;
	void _writeFragmentHeaderNoise(std::stringstream& ssShader) const override;
	void _writeFragmentHeaderWriteDepth(std::stringstream& ssShader) const override;
	void _writeFragmentHeaderDepthCompare(std::stringstream& ssShader) const override;
	void _writeFragmentHeaderMipMap(std::stringstream& ssShader) const override;
	void _writeFragmentGlobalVariablesNotex(std::stringstream& ssShader) const override;
	void _writeFragmentHeaderCalcLight(std::stringstream& ssShader) const override;
	void _writeFragmentMain2Cycle(std::stringstream& ssShader) const override;
	void _writeFragmentMain(std::stringstream& ssShader) const override;
	void _writeFragmentBlendMux(std::stringstream& ssShader) const override;
	void _writeShaderCoverage(std::stringstream& ssShader) const override;
	void _writeFragmentReadTexMipmap(std::stringstream& ssShader) const override;
	void _writeFragmentCallN64Depth(std::stringstream& ssShader) const override;
	void _writeFragmentRenderTarget(std::stringstream& ssShader) const override;
	void _writeShaderFragmentMainEnd(std::stringstream& ssShader) const override;
	void _writeShaderCalcLight(std::stringstream& ssShader) const override;
	void _writeShaderNoise(std::stringstream& ssShader) const override;
	void _writeShaderDither(std::stringstream& ssShader) const override;
	void _writeShaderWriteDepth(std::stringstream& ssShader) const override;
	void _writeShaderN64DepthCompare(std::stringstream& ssShader) const override;
	void _writeShaderN64DepthRender(std::stringstream& ssShader) const override;

	ShaderPartPtr m_blender1;
	ShaderPartPtr m_blender2;
	ShaderPartPtr m_blenderAlpha;
	ShaderPartPtr m_legacyBlender;
	ShaderPartPtr m_clamp;
	ShaderPartPtr m_signExtendColorC;
	ShaderPartPtr m_signExtendAlphaC;
	ShaderPartPtr m_signExtendColorABD;
	ShaderPartPtr m_signExtendAlphaABD;
	ShaderPartPtr m_alphaTest;
	ShaderPartPtr m_callDither;

	ShaderPartPtr m_vertexHeader;
	ShaderPartPtr m_vertexEnd;
	ShaderPartPtr m_vertexRect;
	ShaderPartPtr m_vertexTriangle;

	ShaderPartPtr m_fragmentHeader;
	ShaderPartPtr m_fragmentGlobalVariablesNotex;
	ShaderPartPtr m_fragmentHeaderNoise;
	ShaderPartPtr m_fragmentHeaderWriteDepth;
	ShaderPartPtr m_fragmentHeaderCalcLight;
	ShaderPartPtr m_fragmentHeaderMipMap;
	ShaderPartPtr m_fragmentHeaderDither;
	ShaderPartPtr m_fragmentHeaderDepthCompare;
	ShaderPartPtr m_fragmentMain;
	ShaderPartPtr m_fragmentMain2Cycle;
	ShaderPartPtr m_fragmentBlendMux;
	ShaderPartPtr m_fragmentReadTexMipmap;
	ShaderPartPtr m_fragmentCallN64Depth;
	ShaderPartPtr m_fragmentRenderTarget;
	ShaderPartPtr m_shaderFragmentMainEnd;

	ShaderPartPtr m_shaderNoise;
	ShaderPartPtr m_shaderDither;
	ShaderPartPtr m_shaderWriteDepth;
	ShaderPartPtr m_shaderCalcLight;
	ShaderPartPtr m_shaderN64DepthCompare;
	ShaderPartPtr m_shaderN64DepthRender;
	ShaderPartPtr m_shaderCoverage;

	u32 m_combinerOptionsBits;

	mutable GLuint m_vertexShaderRect = 0u;
	mutable GLuint m_vertexShaderTriangle = 0u;
	mutable GLuint m_vertexShaderTexturedRect = 0u;
	mutable GLuint m_vertexShaderTexturedTriangle = 0u;
};

}
