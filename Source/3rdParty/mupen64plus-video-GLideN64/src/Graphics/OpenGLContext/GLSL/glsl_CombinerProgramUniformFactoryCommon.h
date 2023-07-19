#pragma once
#include "glsl_CombinerProgramUniformFactory.h"

namespace glsl {

/*---------------Uniform-------------*/

struct iUniform	{
	GLint loc = -1;
	int val = -999;
	void set(int _val, bool _force) {
		if (loc >= 0 && (_force || val != _val)) {
			val = _val;
			glUniform1i(loc, _val);
		}
	}
};

struct fUniform {
	GLint loc = -1;
	float val = -9999.9f;
	void set(float _val, bool _force) {
		if (loc >= 0 && (_force || val != _val)) {
			val = _val;
			glUniform1f(loc, _val);
		}
	}
};

struct fv2Uniform {
	GLint loc = -1;
	float val1 = -9999.9f, val2 = -9999.9f;
	void set(float _val1, float _val2, bool _force) {
		if (loc >= 0 && (_force || val1 != _val1 || val2 != _val2)) {
			val1 = _val1;
			val2 = _val2;
			glUniform2f(loc, _val1, _val2);
		}
	}
};

struct fv3Uniform {
	GLint loc = -1;
	float val[3];
	void set(float * _pVal, bool _force) {
		const size_t szData = sizeof(float)* 3;
		if (loc >= 0 && (_force || memcmp(val, _pVal, szData) != 0)) {
			memcpy(val, _pVal, szData);
			glUniform3fv(loc, 1, _pVal);
		}
	}
};

struct fv4Uniform {
	GLint loc = -1;
	float val[4];
	void set(float * _pVal, bool _force) {
		const size_t szData = sizeof(float)* 4;
		if (loc >= 0 && (_force || memcmp(val, _pVal, szData) != 0)) {
			memcpy(val, _pVal, szData);
			glUniform4fv(loc, 1, _pVal);
		}
	}
};

struct iv2Uniform {
	GLint loc = -1;
	int val1 = -999, val2 = -999;
	void set(int _val1, int _val2, bool _force) {
		if (loc >= 0 && (_force || val1 != _val1 || val2 != _val2)) {
			val1 = _val1;
			val2 = _val2;
			glUniform2i(loc, _val1, _val2);
		}
	}
};

struct i4Uniform {
	GLint loc = -1;
	int val0 = -999, val1 = -999, val2 = -999, val3 = -999;
	void set(int _val0, int _val1, int _val2, int _val3, bool _force) {
		if (loc < 0)
			return;
		if (_force || _val0 != val0 || _val1 != val1 || _val2 != val2 || _val3 != val3) {
			val0 = _val0;
			val1 = _val1;
			val2 = _val2;
			val3 = _val3;
			glUniform4i(loc, val0, val1, val2, val3);
		}
	}
};

#define LocateUniform(A) \
A.loc = glGetUniformLocation(_program, #A);

class CombinerProgramUniformFactoryCommon : public CombinerProgramUniformFactory
{
public:
	CombinerProgramUniformFactoryCommon(const opengl::GLInfo & _glInfo);

private:
	void _addNoiseTex(GLuint _program, UniformGroups &_uniforms) const override;

	void _addScreenSpaceTriangleInfo(GLuint _program, UniformGroups &_uniforms) const override;

	void _addRasterInfo(GLuint _program, UniformGroups &_uniforms) const override;

	void _addViewportInfo(GLuint _program, UniformGroups &_uniforms) const override;

	void _addDepthTex(GLuint _program, UniformGroups &_uniforms) const override;

	void _addDepthScale(GLuint _program, UniformGroups &_uniforms) const override;

	void _addTextures(GLuint _program, UniformGroups &_uniforms) const override;

	void _addMSAATextures(GLuint _program, UniformGroups &_uniforms) const override;

	void _addFrameBufferInfo(GLuint _program, UniformGroups &_uniforms) const override;

	void _addTextureFetchMode(GLuint _program, UniformGroups &_uniforms) const override;

	void _addTexturePersp(GLuint _program, UniformGroups &_uniforms) const override;

	void _addFog(GLuint _program, UniformGroups &_uniforms) const override;

	void _addBlendMode1Cycle(GLuint _program, UniformGroups &_uniforms) const override;

	void _addBlendMode2Cycle(GLuint _program, UniformGroups &_uniforms) const override;

	void _addBlendCvg(GLuint _program, UniformGroups &_uniforms) const override;

	void _addDitherMode(GLuint _program, UniformGroups &_uniforms, bool _usesNoise) const override;

	void _addScreenScale(GLuint _program, UniformGroups &_uniforms) const override;

	void _addAlphaTestInfo(GLuint _program, UniformGroups &_uniforms) const override;

	void _addZLutTexture(GLuint _program, UniformGroups &_uniforms) const override;

	void _addDepthInfo(GLuint _program, UniformGroups &_uniforms) const override;

	void _addDepthSource(GLuint _program, UniformGroups &_uniforms) const override;

	void _addRenderTarget(GLuint _program, UniformGroups &_uniforms) const override;

	void _addClampMode(GLuint _program, UniformGroups &_uniforms) const override;

	void _addPolygonOffset(GLuint _program, UniformGroups &_uniforms) const override;

	void _addScreenCoordsScale(GLuint _program, UniformGroups &_uniforms) const override;

	void _addColors(GLuint _program, UniformGroups &_uniforms) const override;

	void _addRectColor(GLuint _program, UniformGroups &_uniforms) const override;

	void _addLights(GLuint _program, UniformGroups &_uniforms) const override;
};

}
