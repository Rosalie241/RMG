#ifndef SR_GL_H
#define SR_GL_H

/* The slice of OpenGL the presenter uses, declared here instead of coming from
 * a system <GL/gl.h>.
 *
 * Why not just include the system header: it is a build-time dependency on a
 * development package (libgl-dev / mesa-common-dev) and a link-time dependency
 * on the libGL.so symlink that package installs, and neither is present on a
 * machine that only has the GL *runtime*. Since the presenter already resolves
 * every GL 3.3 entry point through the host's proc loader, the header buys
 * nothing but the GL 1.1 declarations - and putting those on the loader too
 * drops both dependencies. That is what the other mupen64plus video plugins do.
 *
 * The declarations below are ABI, not a reimplementation: the types, values and
 * signatures are fixed by the OpenGL specification, so they cannot drift from
 * whatever GL the host actually loads.
 *
 * On Win32, windows.h must be included before this header - APIENTRY is the
 * __stdcall calling convention there, and getting it wrong corrupts the stack
 * on every GL call. */

#include <stddef.h>
#include <stdint.h>

#ifndef APIENTRY
#  ifdef _WIN32
#    define APIENTRY __stdcall
#  else
#    define APIENTRY
#  endif
#endif

/* GL's types are spelled in terms of C types of a guaranteed width; these are
 * the definitions from the spec's khrplatform, minus the parts we never name. */
typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef int GLint;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef float GLfloat;
typedef char GLchar;

#define GL_FALSE 0
#define GL_TRUE 1

/* Buffer clears and capabilities */
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_CULL_FACE 0x0B44
#define GL_SCISSOR_TEST 0x0C11

/* Primitives */
#define GL_TRIANGLES 0x0004

/* Textures */
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_NEAREST 0x2600
#define GL_CLAMP_TO_EDGE 0x812F

/* Pixel transfer */
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#define GL_RGBA 0x1908
#define GL_UNSIGNED_BYTE 0x1401

/* Shaders */
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82

/* GL 1.1 - present in every context, but only reachable by name if the build
 * links libGL, which is exactly what this header exists to avoid. */
typedef void(APIENTRY *sr_gl_bind_texture)(GLenum target, GLuint texture);
typedef void(APIENTRY *sr_gl_clear)(GLbitfield mask);
typedef void(APIENTRY *sr_gl_clear_color)(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
typedef void(APIENTRY *sr_gl_delete_textures)(GLsizei n, const GLuint *textures);
typedef void(APIENTRY *sr_gl_disable)(GLenum cap);
typedef void(APIENTRY *sr_gl_draw_arrays)(GLenum mode, GLint first, GLsizei count);
typedef void(APIENTRY *sr_gl_enable)(GLenum cap);
typedef void(APIENTRY *sr_gl_gen_textures)(GLsizei n, GLuint *textures);
typedef GLboolean(APIENTRY *sr_gl_is_enabled)(GLenum cap);
typedef void(APIENTRY *sr_gl_pixel_store_i)(GLenum pname, GLint param);
typedef void(APIENTRY *sr_gl_tex_image_2d)(GLenum target, GLint level, GLint internalformat,
                                           GLsizei width, GLsizei height, GLint border,
                                           GLenum format, GLenum type, const GLvoid *pixels);
typedef void(APIENTRY *sr_gl_tex_parameter_i)(GLenum target, GLenum pname, GLint param);
typedef void(APIENTRY *sr_gl_tex_sub_image_2d)(GLenum target, GLint level, GLint xoffset,
                                               GLint yoffset, GLsizei width, GLsizei height,
                                               GLenum format, GLenum type, const GLvoid *pixels);
typedef void(APIENTRY *sr_gl_viewport)(GLint x, GLint y, GLsizei width, GLsizei height);

/* GL 2.0/3.0 - never available by name, always loaded. */
typedef void(APIENTRY *sr_gl_attach_shader)(GLuint program, GLuint shader);
typedef void(APIENTRY *sr_gl_bind_vertex_array)(GLuint array);
typedef void(APIENTRY *sr_gl_compile_shader)(GLuint shader);
typedef GLuint(APIENTRY *sr_gl_create_program)(void);
typedef GLuint(APIENTRY *sr_gl_create_shader)(GLenum type);
typedef void(APIENTRY *sr_gl_delete_program)(GLuint program);
typedef void(APIENTRY *sr_gl_delete_shader)(GLuint shader);
typedef void(APIENTRY *sr_gl_delete_vertex_arrays)(GLsizei n, const GLuint *arrays);
typedef void(APIENTRY *sr_gl_gen_vertex_arrays)(GLsizei n, GLuint *arrays);
typedef GLint(APIENTRY *sr_gl_get_uniform_location)(GLuint program, const GLchar *name);
typedef void(APIENTRY *sr_gl_get_program_iv)(GLuint program, GLenum pname, GLint *params);
typedef void(APIENTRY *sr_gl_get_shader_iv)(GLuint shader, GLenum pname, GLint *params);
typedef void(APIENTRY *sr_gl_link_program)(GLuint program);
typedef void(APIENTRY *sr_gl_shader_source)(GLuint shader, GLsizei count,
                                            const GLchar *const *string, const GLint *length);
typedef void(APIENTRY *sr_gl_uniform_1i)(GLint location, GLint v0);
typedef void(APIENTRY *sr_gl_use_program)(GLuint program);

#endif
