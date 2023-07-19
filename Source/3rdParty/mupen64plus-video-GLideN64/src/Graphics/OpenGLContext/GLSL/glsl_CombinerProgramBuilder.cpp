#include <iomanip> // for setprecision
#include <assert.h>
#include <Log.h>
#include <Config.h>
#include "glsl_Utils.h"
#include "glsl_CombinerInputs.h"
#include "glsl_CombinerProgramImpl.h"
#include "glsl_CombinerProgramBuilderAccurate.h"
#include "glsl_CombinerProgramUniformFactoryAccurate.h"
#include "GraphicsDrawer.h"

namespace glsl {

u32 CombinerProgramBuilder::s_cycleType = G_CYC_1CYCLE;
TextureConvert CombinerProgramBuilder::s_textureConvert;

/*---------------_compileCombiner-------------*/

static
const char *ColorInput[] = {
	"combined_color.rgb",
	"readtex0.rgb",
	"readtex1.rgb",
	"uPrimColor.rgb",
	"vec_color.rgb",
	"uEnvColor.rgb",
	"uCenterColor.rgb",
	"uScaleColor.rgb",
	"combined_color.a",
	"vec3(readtex0.a)",
	"vec3(readtex1.a)",
	"vec3(uPrimColor.a)",
	"vec3(vec_color.a)",
	"vec3(uEnvColor.a)",
	"vec3(lod_frac)",
	"vec3(uPrimLod)",
	"vec3(0.5 + 0.5*snoise())",
	"vec3(uK4)",
	"vec3(uK5)",
	"vec3(1.0)",
	"vec3(0.0)",
	"vec3(0.5)"
};

static
const char *AlphaInput[] = {
	"combined_color.a",
	"readtex0.a",
	"readtex1.a",
	"uPrimColor.a",
	"vec_color.a",
	"uEnvColor.a",
	"uCenterColor.a",
	"uScaleColor.a",
	"combined_color.a",
	"readtex0.a",
	"readtex1.a",
	"uPrimColor.a",
	"vec_color.a",
	"uEnvColor.a",
	"lod_frac",
	"uPrimLod",
	"0.5 + 0.5*snoise()",
	"uK4",
	"uK5",
	"1.0",
	"0.0",
	"0.5"
};

inline
u32 correctFirstStageParam(u32 _param)
{
	switch (_param) {
	case G_GCI_TEXEL1:
		return G_GCI_TEXEL0;
	case G_GCI_TEXEL1_ALPHA:
		return G_GCI_TEXEL0_ALPHA;
	}
	return _param;
}

static
void _correctFirstStageParams(CombinerStage & _stage)
{
	for (u32 i = 0; i < _stage.numOps; ++i) {
		_stage.op[i].param1 = correctFirstStageParam(_stage.op[i].param1);
		_stage.op[i].param2 = correctFirstStageParam(_stage.op[i].param2);
		_stage.op[i].param3 = correctFirstStageParam(_stage.op[i].param3);
	}
}

inline
u32 correctFirstStageParam2Cyc(u32 _param)
{
	switch (_param) {
	case G_GCI_COMBINED:
		return G_GCI_HALF;
	}
	return _param;
}

static
void _correctFirstStageParams2Cyc(CombinerStage & _stage)
{
	for (u32 i = 0; i < _stage.numOps; ++i) {
		_stage.op[i].param1 = correctFirstStageParam2Cyc(_stage.op[i].param1);
		_stage.op[i].param2 = correctFirstStageParam2Cyc(_stage.op[i].param2);
		_stage.op[i].param3 = correctFirstStageParam2Cyc(_stage.op[i].param3);
	}
}

inline
u32 correctSecondStageParam(u32 _param)
{
	switch (_param) {
	case G_GCI_TEXEL0:
		return G_GCI_TEXEL1;
	case G_GCI_TEXEL1:
		return G_GCI_TEXEL0;
	case G_GCI_TEXEL0_ALPHA:
		return G_GCI_TEXEL1_ALPHA;
	case G_GCI_TEXEL1_ALPHA:
		return G_GCI_TEXEL0_ALPHA;
	}
	return _param;
}

static
void _correctSecondStageParams(CombinerStage & _stage) {
	for (u32 i = 0; i < _stage.numOps; ++i) {
		_stage.op[i].param1 = correctSecondStageParam(_stage.op[i].param1);
		_stage.op[i].param2 = correctSecondStageParam(_stage.op[i].param2);
		_stage.op[i].param3 = correctSecondStageParam(_stage.op[i].param3);
	}
}

static
CombinerInputs _compileCombiner(const CombinerStage & _stage, const char** _Input, std::stringstream & _strShader) {
	bool bBracketOpen = false;
	CombinerInputs inputs;
	for (u32 i = 0; i < _stage.numOps; ++i) {
		switch (_stage.op[i].op) {
		case LOAD:
			//			sprintf(buf, "(%s ", _Input[_stage.op[i].param1]);
			_strShader << "(" << _Input[_stage.op[i].param1] << " ";
			bBracketOpen = true;
			inputs.addInput(_stage.op[i].param1);
			break;
		case SUB:
			if (bBracketOpen) {
				//				sprintf(buf, "- %s)", _Input[_stage.op[i].param1]);
				_strShader << "- " << _Input[_stage.op[i].param1] << ")";
				bBracketOpen = false;
			}
			else
				//				sprintf(buf, "- %s", _Input[_stage.op[i].param1]);
				_strShader << "- " << _Input[_stage.op[i].param1];
			//			_strShader += buf;
			inputs.addInput(_stage.op[i].param1);
			break;
		case ADD:
			if (bBracketOpen) {
				//				sprintf(buf, "+ %s)", _Input[_stage.op[i].param1]);
				_strShader << "+ " << _Input[_stage.op[i].param1] << ")";
				bBracketOpen = false;
			}
			else
				//				sprintf(buf, "+ %s", _Input[_stage.op[i].param1]);
				_strShader << "+ " << _Input[_stage.op[i].param1];
			inputs.addInput(_stage.op[i].param1);
			break;
		case MUL:
			if (bBracketOpen) {
				//				sprintf(buf, ")*%s", _Input[_stage.op[i].param1]);
				_strShader << ")*" << _Input[_stage.op[i].param1];
				bBracketOpen = false;
			}
			else
				//				sprintf(buf, "*%s", _Input[_stage.op[i].param1]);
				_strShader << "*" << _Input[_stage.op[i].param1];
			inputs.addInput(_stage.op[i].param1);
			break;
		case INTER:
			//			sprintf(buf, "mix(%s, %s, %s)", _Input[_stage.op[0].param2], _Input[_stage.op[0].param1], _Input[_stage.op[0].param3]);
			_strShader << "mix(" <<
				_Input[_stage.op[0].param2] << "," <<
				_Input[_stage.op[0].param1] << "," <<
				_Input[_stage.op[0].param3] << ")";
			inputs.addInput(_stage.op[i].param1);
			inputs.addInput(_stage.op[i].param2);
			inputs.addInput(_stage.op[i].param3);
			break;

			//			default:
			//				assert(false);
		}
	}
	if (bBracketOpen)
		_strShader << ")";
	_strShader << ";" << std::endl;
	return inputs;
}

static
bool needClampColor() {
	return CombinerProgramBuilder::s_cycleType <= G_CYC_2CYCLE;
}

static
bool combinedColorC(const gDPCombine & _combine) {
	if (CombinerProgramBuilder::s_cycleType != G_CYC_2CYCLE)
		return false;
	return _combine.mRGB1 == G_CCMUX_COMBINED;
}

static
bool combinedAlphaC(const gDPCombine & _combine) {
	if (CombinerProgramBuilder::s_cycleType != G_CYC_2CYCLE)
		return false;
	return _combine.mA1 == G_ACMUX_COMBINED;
}

static
bool combinedColorABD(const gDPCombine & _combine) {
	if (CombinerProgramBuilder::s_cycleType != G_CYC_2CYCLE)
		return false;
	if (_combine.aRGB1 == G_CCMUX_COMBINED)
		return true;
	if (_combine.saRGB1 == G_CCMUX_COMBINED || _combine.sbRGB1 == G_CCMUX_COMBINED)
		return _combine.mRGB1 != G_CCMUX_0;
	return false;
}

static
bool combinedAlphaABD(const gDPCombine & _combine) {
	if (CombinerProgramBuilder::s_cycleType != G_CYC_2CYCLE)
		return false;
	if (_combine.aA1 == G_ACMUX_COMBINED)
		return true;
	if (_combine.saA1 == G_ACMUX_COMBINED || _combine.sbA1 == G_ACMUX_COMBINED)
		return _combine.mA1 != G_ACMUX_0;
	return false;
}

CombinerInputs CombinerProgramBuilder::compileCombiner(const CombinerKey & _key, Combiner & _color, Combiner & _alpha, std::string & _strShader)
{
	gDPCombine combine;
	combine.mux = _key.getMux();

	std::stringstream ssShader;

	if (CombinerProgramBuilder::s_cycleType != G_CYC_2CYCLE) {
		_correctFirstStageParams(_alpha.stage[0]);
		_correctFirstStageParams(_color.stage[0]);
	} else {
		_correctFirstStageParams2Cyc(_alpha.stage[0]);
		_correctFirstStageParams2Cyc(_color.stage[0]);
	}
	ssShader << "  alpha1 = ";
	CombinerInputs inputs = _compileCombiner(_alpha.stage[0], AlphaInput, ssShader);
	// Simulate N64 color sign-extend.
	if (combinedAlphaC(combine))
		_writeSignExtendAlphaC(ssShader);
	else if (combinedAlphaABD(combine))
		_writeSignExtendAlphaABD(ssShader);

	if (CombinerProgramBuilder::s_cycleType < G_CYC_FILL)
		_writeAlphaTest(ssShader);

	ssShader << "  color1 = ";
	inputs += _compileCombiner(_color.stage[0], ColorInput, ssShader);
	// Simulate N64 color sign-extend.
	if (combinedColorC(combine))
		_writeSignExtendColorC(ssShader);
	else if (combinedColorABD(combine))
		_writeSignExtendColorABD(ssShader);

	if (CombinerProgramBuilder::s_cycleType == G_CYC_2CYCLE) {

		ssShader << "  combined_color = vec4(color1, alpha1);" << std::endl;
		if (_alpha.numStages == 2) {
			ssShader << "  alpha2 = ";
			_correctSecondStageParams(_alpha.stage[1]);
			inputs += _compileCombiner(_alpha.stage[1], AlphaInput, ssShader);
		}
		else
			ssShader << "  alpha2 = alpha1;" << std::endl;

		ssShader << "  if (uCvgXAlpha != 0 && alpha2 < 0.125) discard;" << std::endl;

		if (_color.numStages == 2) {
			ssShader << "  color2 = ";
			_correctSecondStageParams(_color.stage[1]);
			inputs += _compileCombiner(_color.stage[1], ColorInput, ssShader);
		}
		else
			ssShader << "  color2 = color1;" << std::endl;

		ssShader << "  lowp vec4 cmbRes = vec4(color2, alpha2);" << std::endl;
	}
	else {
		if (CombinerProgramBuilder::s_cycleType < G_CYC_FILL)
			ssShader << "  if (uCvgXAlpha != 0 && alpha1 < 0.125) discard;" << std::endl;
		ssShader << "  lowp vec4 cmbRes = vec4(color1, alpha1);" << std::endl;
	}

	// Simulate N64 color clamp.
	if (needClampColor())
		_writeClamp(ssShader);
	else
		ssShader << "  lowp vec4 clampedColor = clamp(cmbRes, 0.0, 1.0);" << std::endl;

	if (CombinerProgramBuilder::s_cycleType <= G_CYC_2CYCLE) {
		_writeCallDither(ssShader);

		ssShader << "if (uCvgXAlpha != 0) cvg *= clampedColor.a;" << std::endl;
		ssShader << "if (uAlphaCvgSel != 0) clampedColor.a = cvg; " << std::endl;
	}


	if (config.generalEmulation.enableLegacyBlending == 0) {
		if (CombinerProgramBuilder::s_cycleType <= G_CYC_2CYCLE) {
			_writeBlender1(ssShader);
			if (CombinerProgramBuilder::s_cycleType == G_CYC_2CYCLE)
				_writeBlender2(ssShader);
			_writeBlenderAlpha(ssShader);
		} else
			ssShader << "  fragColor = clampedColor;" << std::endl;

	}
	else {
		ssShader << "  fragColor = clampedColor;" << std::endl;
		_writeLegacyBlender(ssShader);
	}


	// SHOW COVERAGE HACK
	//	ssShader << "fragColor.rgb = vec3(cvg);" << std::endl;

	_strShader = ssShader.str();
	return inputs;
}

graphics::CombinerProgram * CombinerProgramBuilder::buildCombinerProgram(Combiner & _color,
																		Combiner & _alpha,
																		const CombinerKey & _key)
{
	CombinerProgramBuilder::s_cycleType = _key.getCycleType();
	CombinerProgramBuilder::s_textureConvert.setMode(_key.getBilerp());

	std::string strCombiner;
	CombinerInputs combinerInputs(compileCombiner(_key, _color, _alpha, strCombiner));

	const bool bUseLod = combinerInputs.usesLOD();
	const bool bUseTextures = combinerInputs.usesTexture();
	const bool bIsRect = _key.isRectKey();
	const bool bUseHWLight = !bIsRect && // Rects not use lighting
							 isHWLightingAllowed() &&
							 combinerInputs.usesShadeColor();

	if (bUseHWLight)
		combinerInputs.addInput(G_GCI_HW_LIGHT);

	std::stringstream ssShader;

	/* Write headers */
	_writeFragmentHeader(ssShader);

	if (bUseTextures) {
		_writeFragmentGlobalVariablesTex(ssShader);

		if (CombinerProgramBuilder::s_cycleType == G_CYC_2CYCLE && config.generalEmulation.enableLegacyBlending == 0)
			ssShader << "uniform lowp ivec4 uBlendMux2;" << std::endl << "uniform lowp int uForceBlendCycle2;" << std::endl;

		if (CombinerProgramBuilder::s_cycleType <= G_CYC_2CYCLE)
			_writeFragmentHeaderDither(ssShader);
		_writeFragmentHeaderNoise(ssShader);
		_writeFragmentHeaderWriteDepth(ssShader);
		_writeFragmentHeaderDepthCompare(ssShader);
		_writeFragmentHeaderReadMSTex(ssShader);
		_writeFragmentHeaderClampWrapMirrorEngine(ssShader);
		if (bUseLod)
			_writeFragmentHeaderMipMap(ssShader);
		else if (CombinerProgramBuilder::s_cycleType < G_CYC_COPY)
			_writeFragmentHeaderReadTex(ssShader);
		else
			_writeFragmentHeaderReadTexCopyMode(ssShader);
	} else {
		_writeFragmentGlobalVariablesNotex(ssShader);

		if (CombinerProgramBuilder::s_cycleType == G_CYC_2CYCLE && config.generalEmulation.enableLegacyBlending == 0)
			ssShader << "uniform lowp ivec4 uBlendMux2;" << std::endl << "uniform lowp int uForceBlendCycle2;" << std::endl;

		if (CombinerProgramBuilder::s_cycleType <= G_CYC_2CYCLE)
			_writeFragmentHeaderDither(ssShader);
		_writeFragmentHeaderNoise(ssShader);
		_writeFragmentHeaderWriteDepth(ssShader);
		_writeFragmentHeaderDepthCompare(ssShader);
	}

	if (bUseHWLight)
		_writeFragmentHeaderCalcLight(ssShader);

	/* Write body */
	if (CombinerProgramBuilder::s_cycleType == G_CYC_2CYCLE)
		_writeFragmentMain2Cycle(ssShader);
	else
		_writeFragmentMain(ssShader);


	if (CombinerProgramBuilder::s_cycleType <= G_CYC_2CYCLE)
		_writeFragmentBlendMux(ssShader);

	if (CombinerProgramBuilder::s_cycleType <= G_CYC_2CYCLE && m_useCoverage)
		_writeShaderCoverage(ssShader);
	else
		ssShader << "cvg = 1.0; \n" << std::endl;


	if (bUseTextures) {
		_writeFragmentCorrectTexCoords(ssShader);
		if (combinerInputs.usesTile(0))
		{
			_writeFragmentClampWrapMirrorEngineTex0(ssShader);
		}
		if (combinerInputs.usesTile(1))
		{
			_writeFragmentClampWrapMirrorEngineTex1(ssShader);
		}

		if (bUseLod) {
			_writeFragmentReadTexMipmap(ssShader);
		} else {
			if (CombinerProgramBuilder::s_cycleType < G_CYC_COPY) {
				if (combinerInputs.usesTile(0))
					_writeFragmentReadTex0(ssShader);
				else
					ssShader << "  lowp vec4 readtex0;" << std::endl;

				if (combinerInputs.usesTile(1))
					_writeFragmentReadTex1(ssShader);
			} else
				_writeFragmentReadTexCopyMode(ssShader);
		}
	}

	if (bUseHWLight)
		ssShader << "  calc_light(vNumLights, shadeColor.rgb, input_color);" << std::endl;
	else
		ssShader << "  input_color = shadeColor.rgb;" << std::endl;

	ssShader << "  vec_color = vec4(input_color, shadeColor.a);" << std::endl;
	ssShader << strCombiner << std::endl;

	if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable)
		_writeFragmentCallN64Depth(ssShader);
	else
		_writeFragmentRenderTarget(ssShader);

	// End of Main() function
	_writeShaderFragmentMainEnd(ssShader);

	/* Write other functions */
	if (bUseHWLight)
		_writeShaderCalcLight(ssShader);

	if (bUseTextures) {
		_writeShaderClampWrapMirrorEngine(ssShader);
		if (bUseLod)
			_writeShaderMipmap(ssShader);
		else {
			if (CombinerProgramBuilder::s_cycleType < G_CYC_COPY)
				_writeShaderReadtex(ssShader);
			else
				_writeShaderReadtexCopyMode(ssShader);
		}
	}

	_writeShaderNoise(ssShader);

	if (CombinerProgramBuilder::s_cycleType <= G_CYC_2CYCLE)
		_writeShaderDither(ssShader);

	_writeShaderWriteDepth(ssShader);

	_writeShaderN64DepthCompare(ssShader);

	_writeShaderN64DepthRender(ssShader);

	const std::string strFragmentShader(ssShader.str());

	/* Create shader program */

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar * strShaderData = strFragmentShader.data();
	glShaderSource(fragmentShader, 1, &strShaderData, nullptr);
	glCompileShader(fragmentShader);
	if (!Utils::checkShaderCompileStatus(fragmentShader))
		Utils::logErrorShader(GL_FRAGMENT_SHADER, strFragmentShader);

	GLuint program = glCreateProgram();
	Utils::locateAttributes(program, bIsRect, bUseTextures);
	if (bIsRect)
		glAttachShader(program, bUseTextures ? _getVertexShaderTexturedRect() : _getVertexShaderRect());
	else
		glAttachShader(program, bUseTextures ? _getVertexShaderTexturedTriangle() : _getVertexShaderTriangle());
	glAttachShader(program, fragmentShader);
	if (CombinerInfo::get().isShaderCacheSupported()) {
		if (IS_GL_FUNCTION_VALID(ProgramParameteri))
			glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
	}
	glLinkProgram(program);
	assert(Utils::checkProgramLinkStatus(program));
	glDeleteShader(fragmentShader);

	UniformGroups uniforms;
	m_uniformFactory->buildUniforms(program, combinerInputs, _key, uniforms);

	return new CombinerProgramImpl(_key, program, m_useProgram, combinerInputs, std::move(uniforms));
}

CombinerProgramBuilder::CombinerProgramBuilder(const opengl::GLInfo & _glinfo, opengl::CachedUseProgram * _useProgram,
	std::unique_ptr<CombinerProgramUniformFactory> _uniformFactory)
: m_uniformFactory(std::move(_uniformFactory))
, m_useProgram(_useProgram)
, m_useCoverage(_glinfo.coverage && config.generalEmulation.enableCoverage != 0)
{
}

CombinerProgramBuilder::~CombinerProgramBuilder()
{
}

}
