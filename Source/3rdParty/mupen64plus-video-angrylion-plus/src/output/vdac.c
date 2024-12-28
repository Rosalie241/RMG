#include "vdac.h"
#include "screen.h"
#include "core/msg.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef GLES
#include <GLES3/gl3.h>
#else
#include "gl_core_3_3.h"
#endif

#define GLES_SHADER_HEADER "#version 300 es\nprecision lowp float;\n"
#define GL_SHADER_HEADER "#version 330 core\n"

#define TEX_FORMAT GL_RGBA
#define TEX_TYPE GL_UNSIGNED_BYTE

static bool m_fbo_enabled;
static GLuint m_fbo;
static bool m_integer_scaling;

static GLuint m_fbtex;
static uint32_t m_fbtex_width;
static uint32_t m_fbtex_height;

static GLuint m_rawtex;
static uint32_t m_rawtex_width;
static uint32_t m_rawtex_height;
static bool m_rawtex_read;

static GLuint m_program;
static GLuint m_vao;

#ifdef _DEBUG
static void gl_check_errors(void)
{
    GLenum err;
    static int32_t invalid_op_count = 0;
    while ((err = glGetError()) != GL_NO_ERROR) {
        // if gl_check_errors is called from a thread with no valid
        // GL context, it would be stuck in an infinite loop here, since
        // glGetError itself causes GL_INVALID_OPERATION, so check for a few
        // cycles and abort if there are too many errors of that kind
        if (err == GL_INVALID_OPERATION) {
            if (++invalid_op_count >= 100) {
                msg_error("gl_check_errors: invalid OpenGL context!");
            }
        } else {
            invalid_op_count = 0;
        }

        char* err_str;
        switch (err) {
            case GL_INVALID_OPERATION:
                err_str = "INVALID_OPERATION";
                break;
            case GL_INVALID_ENUM:
                err_str = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                err_str = "INVALID_VALUE";
                break;
            case GL_OUT_OF_MEMORY:
                err_str = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                err_str = "INVALID_FRAMEBUFFER_OPERATION";
                break;
            default:
                err_str = "unknown";
        }
        msg_debug("gl_check_errors: %d (%s)", err, err_str);
    }
}
#else
#define gl_check_errors(...)
#endif

static void gl_fbo_create(uint32_t width, uint32_t height)
{
    // prepare FB texture
    glGenTextures(1, &m_fbtex);
    glBindTexture(GL_TEXTURE_2D, m_fbtex);

    // reallocate texture buffer on GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, TEX_FORMAT, TEX_TYPE, NULL);

    // prepare framebuffer object
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // attach texture
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbtex, 0);

    // bind raw texture again
    glBindTexture(GL_TEXTURE_2D, m_rawtex);

    // check framebuffer object
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        msg_error("FBO creation failed!");
    }
}

static void gl_fbo_delete(void)
{
    if (m_fbo) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }

    if (m_fbtex) {
        glDeleteTextures(1, &m_fbtex);
        m_fbtex = 0;
    }
}

static bool gl_shader_load_file(GLuint shader, const char* path)
{
    bool success = false;
    GLchar* source = NULL;
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        // fail quietly
        goto end;
    }

    // get file size
    fseek(fp, 0, SEEK_END);
    uint32_t source_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // allocate buffer for shader code
    source = malloc(source_size + 1);
    if (source == NULL) {
        msg_error("Can't allocate memory for shader file %s", path);
        goto end;
    }

    // read shader code
    if (fread(source, source_size, 1, fp) != 1) {
        msg_warning("Can't read shader file %s", path);
        goto end;
    }

    // terminate shader code string
    source[source_size] = 0;

    // send string to OpenGL
    const GLchar* source_ptr = source;
    glShaderSource(shader, 1, &source_ptr, NULL);

    success = true;

end:
    if (fp) {
        fclose(fp);
    }
    if (source) {
        free(source);
    }

    return success;
}

static GLuint gl_shader_compile(GLenum type, const GLchar* source, const char* path)
{
    GLuint shader = glCreateShader(type);

    // try to load external shader file first, otherwise use embedded fallback shader
    if (!gl_shader_load_file(shader, path)) {
        glShaderSource(shader, 1, &source, NULL);
    }

    glCompileShader(shader);

    GLint param;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &param);

    if (!param) {
        GLchar log[4096];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        msg_error("%s shader error: %s\n", type == GL_FRAGMENT_SHADER ? "Frag" : "Vert", log);
    }

    return shader;
}

static GLuint gl_shader_link(GLuint vert, GLuint frag)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint param;
    glGetProgramiv(program, GL_LINK_STATUS, &param);

    if (!param) {
        GLchar log[4096];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        msg_error("Shader link error: %s\n", log);
    }

    glDeleteShader(frag);
    glDeleteShader(vert);

    return program;
}

void vdac_init(struct n64video_config* config)
{
    screen_init(config);

#ifndef GLES
    // load OpenGL function pointers
    ogl_LoadFunctions();
#endif

    const char* gl_version = (const char*)glGetString(GL_VERSION);

#ifndef GLES
    const int is_gles = strstr(gl_version, "OpenGL ES") != NULL;
#else
    const int is_gles = 1;
#endif

    msg_debug("%s: GL_VERSION='%s'", __FUNCTION__, gl_version);
    msg_debug("%s: GL_VENDOR='%s'", __FUNCTION__, glGetString(GL_VENDOR));
    msg_debug("%s: GL_RENDERER='%s'", __FUNCTION__, glGetString(GL_RENDERER));
    msg_debug("%s: GL_SHADING_LANGUAGE_VERSION='%s'", __FUNCTION__, glGetString(GL_SHADING_LANGUAGE_VERSION));

    // shader sources for drawing a clipped full-screen triangle. the geometry
    // is defined by the vertex ID, so a VBO is not required.
    GLchar vert_shader[256];
    sprintf(vert_shader, "%s%s", (is_gles ? GLES_SHADER_HEADER : GL_SHADER_HEADER),
        "out vec2 uv;\n"
        "void main(void) {\n"
        "    uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n"
        "    gl_Position = vec4(uv * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.0, 1.0);\n"
        "}\n"
    );

    GLchar frag_shader[256];
    sprintf(frag_shader, "%s%s", (is_gles ? GLES_SHADER_HEADER : GL_SHADER_HEADER),
        "in vec2 uv;\n"
        "layout(location = 0) out vec4 color;\n"
        "uniform sampler2D tex0;\n"
        "void main(void) {\n"
        "    color = texture(tex0, uv);\n"
        "}\n"
    );

    // compile and link OpenGL program
    GLuint vert = gl_shader_compile(GL_VERTEX_SHADER, vert_shader, "alp_screen.vert");
    GLuint frag = gl_shader_compile(GL_FRAGMENT_SHADER, frag_shader, "alp_screen.frag");
    m_program = gl_shader_link(vert, frag);
    glUseProgram(m_program);

    // prepare dummy VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // prepare texture
    glGenTextures(1, &m_rawtex);
    glBindTexture(GL_TEXTURE_2D, m_rawtex);

    // select interpolation method
    GLint filter;
    switch (config->vi.interp) {
        case VI_INTERP_HYBRID:
            m_fbo_enabled = true;
            filter = GL_LINEAR;
            break;
        case VI_INTERP_LINEAR:
            m_fbo_enabled = false;
            filter = GL_LINEAR;
            break;
        case VI_INTERP_NEAREST:
        default:
            m_fbo_enabled = false;
            filter = GL_NEAREST;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    // read with exact FB size in non-filtered modes
    m_rawtex_read = config->vi.mode != VI_MODE_NORMAL;

    // save integer scaling flag, will be used later
    m_integer_scaling = config->vi.integer_scaling;

    // check if there was an error when using any of the commands above
    gl_check_errors();
}

void vdac_read(struct n64video_frame_buffer* fb, bool alpha)
{
    uint32_t width, height;

    if (m_rawtex_read) {
        width = m_rawtex_width;
        height = m_rawtex_height;
    } else {
        width = m_fbtex_width;
        height = m_fbtex_height;
    }

    // create temporary FBO if none has been created so far or if the existing
    // one has the wrong size
    bool temp_fbo = !m_fbo_enabled || width != m_fbtex_width || height != m_fbtex_height;

    fb->width = width;
    fb->height = fb->height_out = height;
    fb->pitch = fb->width;

    if (fb->pixels) {
        // create temporary FBO
        if (temp_fbo) {
            gl_fbo_delete(); // in case m_fbo_enabled is true
            gl_fbo_create(width, height);

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
            glViewport(0, 0, width, height);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
        glReadPixels(0, 0, width, height, alpha ? GL_RGBA : GL_RGB, TEX_TYPE, fb->pixels);

        // delete temporary FBO
        if (temp_fbo) {
            if (!m_fbo_enabled) {
                // bind default FB
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            }

            gl_fbo_delete();

            if (m_fbo_enabled) {
                // restore previous FBO
                gl_fbo_create(m_fbtex_width, m_fbtex_height);
            }
        }
    }
}

void vdac_write(struct n64video_frame_buffer* fb)
{
    bool raw_size_changed = m_rawtex_width != fb->width || m_rawtex_height != fb->height;
    bool fb_size_changed = m_fbtex_width != fb->width || m_fbtex_height != fb->height_out;

    // check if the framebuffer size has changed
    if (raw_size_changed) {
        m_rawtex_width = fb->width;
        m_rawtex_height = fb->height;

        // set pitch for all unpacking operations
        glPixelStorei(GL_UNPACK_ROW_LENGTH, fb->pitch);

        // reallocate texture buffer on GPU
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_rawtex_width,
            m_rawtex_height, 0, TEX_FORMAT, TEX_TYPE, fb->pixels);

        msg_debug("%s: resized framebuffer texture: %dx%d", __FUNCTION__, m_rawtex_width, m_rawtex_height);
    } else {
        // copy local buffer to GPU texture buffer
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_rawtex_width, m_rawtex_height,
            TEX_FORMAT, TEX_TYPE, fb->pixels);
    }

    if (fb_size_changed) {
        m_fbtex_width = fb->width;
        m_fbtex_height = fb->height_out;

        if (m_fbo_enabled) {
            // rebuild FBO
            gl_fbo_delete();
            gl_fbo_create(m_fbtex_width, m_fbtex_height);
        }
    }
}

void vdac_sync(bool valid)
{
    // clear old buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // get current window size and position
    int32_t win_width;
    int32_t win_height;
    int32_t out_width;
    int32_t out_height;
    int32_t out_x;
    int32_t out_y;

    screen_adjust(m_fbtex_width, m_fbtex_height, &win_width, &win_height, &out_x, &out_y);

    // if the screen is invalid or hidden, do nothing
    if (win_width <= 0 || win_height <= 0) {
        return;
    }

    // skip rendering and leave buffer blank if there's no valid input
    if (!valid) {
        screen_update();
        return;
    }

    if (m_integer_scaling) {
        out_width = (int32_t)m_fbtex_width;
        out_height = (int32_t)m_fbtex_height;

        // scale down if output is bigger than window
        while (out_width > win_width || out_height > win_height) {
            out_width /= 2;
            out_height /= 2;
        }

        // scale up if doubled size is smaller than window
        while (out_width * 2 <= win_width && out_height * 2 <= win_height) {
            out_width *= 2;
            out_height *= 2;
        }

        // apply new size and offset
        out_x += (win_width - out_width) / 2;
        out_y += (win_height - out_height) / 2;
    } else {
        int32_t hw = m_fbtex_height * win_width;
        int32_t wh = m_fbtex_width * win_height;

        // add letterboxes or pillarboxes if the window has a different aspect ratio
        // than the current display mode
        if (hw > wh) {
            int32_t w_max = wh / m_fbtex_height;
            out_x += (win_width - w_max) / 2;
            out_width = w_max;
            out_height = win_height;
        } else if (hw < wh) {
            int32_t h_max = hw / m_fbtex_width;
            out_y += (win_height - h_max) / 2;
            out_width = win_width;
            out_height = h_max;
        } else {
            out_width = win_width;
            out_height = win_height;
        }
    }

    if (m_fbo_enabled) {
        // framebuffer post-processing
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
        glViewport(0, 0, m_fbtex_width, m_fbtex_height);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // final upscale and output
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        GLint src_x0 = 0;
        GLint src_y0 = 0;
        GLint src_x1 = m_fbtex_width - 1;
        GLint src_y1 = m_fbtex_height - 1;
        GLint dst_x0 = out_x;
        GLint dst_y0 = out_y;
        GLint dst_x1 = dst_x0 + out_width - 1;
        GLint dst_y1 = dst_y0 + out_height - 1;

        glBlitFramebuffer(src_x0, src_y0, src_x1, src_y1, dst_x0, dst_y0, dst_x1, dst_y1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    } else {
        // configure viewport
        glViewport(out_x, out_y, out_width, out_height);

        // draw fullscreen triangle
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    // check if there was an error when using any of the commands above
    gl_check_errors();

    // refresh screen with new frame
    screen_update();
}

void vdac_close(void)
{
    m_rawtex_width = 0;
    m_rawtex_height = 0;

    m_fbtex_width = 0;
    m_fbtex_height = 0;

    if (m_rawtex) {
        glDeleteTextures(1, &m_rawtex);
        m_rawtex = 0;
    }

    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }

    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }

    gl_fbo_delete();

    screen_close();
}
