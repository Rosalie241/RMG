/* OpenGL 3.3 presenter.
 *
 * Two ways in, and which one a front-end uses decides how much of this file is
 * platform code:
 *
 *   sr_present_init          - the presenter owns the window's GL context and
 *                              creates it itself. That means WGL, so this path
 *                              exists on Win32 only (PJ64 is its only caller).
 *   sr_present_init_external - the host already has a current context and hands
 *                              over a proc loader. Nothing here touches the
 *                              windowing system, so this path is portable, and
 *                              it is the one mupen64plus uses on every OS.
 *
 * Everything after context creation - shader, texture upload, letterboxed draw
 * - is shared by both and is plain GL. The Win32-only pieces are kept behind
 * guards rather than in a separate file because they interleave with that
 * shared code at the statement level (a MakeCurrent around an upload), and
 * splitting them out would mean duplicating the upload.
 *
 * No GL function is called by name: every one is a pointer resolved at init
 * (see sr_gl.h for why). The p_ prefix is a reminder that a null pointer here
 * means the context could not supply that entry point, not that GL failed. */
#include "sr_present.h"
#include "sr_gl.h"

#include <stddef.h>
#include <string.h>

#ifndef _WIN32
#include <dlfcn.h>
#endif

#ifdef _WIN32
#ifndef WGL_CONTEXT_MAJOR_VERSION_ARB
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#endif

typedef HGLRC(WINAPI *wgl_create_context_attribs_arb_proc)(HDC hdc, HGLRC share, const int *attribs);
typedef BOOL(WINAPI *wgl_swap_interval_ext_proc)(int interval);

static wgl_create_context_attribs_arb_proc p_wglCreateContextAttribsARB;
static wgl_swap_interval_ext_proc p_wglSwapIntervalEXT;
#endif

static sr_gl_bind_texture p_glBindTexture;
static sr_gl_clear p_glClear;
static sr_gl_clear_color p_glClearColor;
static sr_gl_delete_textures p_glDeleteTextures;
static sr_gl_disable p_glDisable;
static sr_gl_draw_arrays p_glDrawArrays;
static sr_gl_enable p_glEnable;
static sr_gl_gen_textures p_glGenTextures;
static sr_gl_is_enabled p_glIsEnabled;
static sr_gl_pixel_store_i p_glPixelStorei;
static sr_gl_tex_image_2d p_glTexImage2D;
static sr_gl_tex_parameter_i p_glTexParameteri;
static sr_gl_tex_sub_image_2d p_glTexSubImage2D;
static sr_gl_viewport p_glViewport;

static sr_gl_attach_shader p_glAttachShader;
static sr_gl_bind_vertex_array p_glBindVertexArray;
static sr_gl_compile_shader p_glCompileShader;
static sr_gl_create_program p_glCreateProgram;
static sr_gl_create_shader p_glCreateShader;
static sr_gl_delete_program p_glDeleteProgram;
static sr_gl_delete_shader p_glDeleteShader;
static sr_gl_delete_vertex_arrays p_glDeleteVertexArrays;
static sr_gl_gen_vertex_arrays p_glGenVertexArrays;
static sr_gl_get_uniform_location p_glGetUniformLocation;
static sr_gl_get_program_iv p_glGetProgramiv;
static sr_gl_get_shader_iv p_glGetShaderiv;
static sr_gl_link_program p_glLinkProgram;
static sr_gl_shader_source p_glShaderSource;
static sr_gl_uniform_1i p_glUniform1i;
static sr_gl_use_program p_glUseProgram;

static void *(*g_gl_load_proc)(const char *name) = NULL;

/* Look a symbol up in the GL library itself, bypassing any host loader. */
static void *platform_gl_proc(const char *name)
{
#ifdef _WIN32
    void *proc = (void *)wglGetProcAddress(name);
    /* wglGetProcAddress reports failure as any of these, not just NULL. */
    if (proc == (void *)1 || proc == (void *)2 || proc == (void *)3 || proc == (void *)-1) {
        proc = NULL;
    }
    if (!proc) {
        proc = (void *)GetProcAddress(GetModuleHandleA("opengl32.dll"), name);
    }
    return proc;
#else
    /* The SONAME, not the libGL.so symlink: the symlink is installed by the
     * development package, while this one comes with the driver and is what
     * is already mapped into the process. Deliberately never dlclose'd - the
     * handle is refcounted and the context outlives any single presenter. */
    static void *libgl = NULL;
    if (!libgl) {
        libgl = dlopen("libGL.so.1", RTLD_LAZY);
        if (!libgl) {
            return NULL;
        }
    }
    return dlsym(libgl, name);
#endif
}

static void *load_gl_proc(const char *name)
{
    void *proc = NULL;

    if (g_gl_load_proc) {
        proc = g_gl_load_proc(name);
    }
    /* A host loader is wglGetProcAddress or glXGetProcAddress underneath, and
     * neither is required to return the GL 1.1 entry points - those predate
     * extension loading and are plain exports of the library. Falling back to
     * the library's own symbol table covers exactly that gap, so the 1.1 calls
     * need no link-time dependency on libGL. */
    if (!proc) {
        proc = platform_gl_proc(name);
    }
    return proc;
}

static bool load_gl_procs(void)
{
    p_glBindTexture = (sr_gl_bind_texture)load_gl_proc("glBindTexture");
    p_glClear = (sr_gl_clear)load_gl_proc("glClear");
    p_glClearColor = (sr_gl_clear_color)load_gl_proc("glClearColor");
    p_glDeleteTextures = (sr_gl_delete_textures)load_gl_proc("glDeleteTextures");
    p_glDisable = (sr_gl_disable)load_gl_proc("glDisable");
    p_glDrawArrays = (sr_gl_draw_arrays)load_gl_proc("glDrawArrays");
    p_glEnable = (sr_gl_enable)load_gl_proc("glEnable");
    p_glGenTextures = (sr_gl_gen_textures)load_gl_proc("glGenTextures");
    p_glIsEnabled = (sr_gl_is_enabled)load_gl_proc("glIsEnabled");
    p_glPixelStorei = (sr_gl_pixel_store_i)load_gl_proc("glPixelStorei");
    p_glTexImage2D = (sr_gl_tex_image_2d)load_gl_proc("glTexImage2D");
    p_glTexParameteri = (sr_gl_tex_parameter_i)load_gl_proc("glTexParameteri");
    p_glTexSubImage2D = (sr_gl_tex_sub_image_2d)load_gl_proc("glTexSubImage2D");
    p_glViewport = (sr_gl_viewport)load_gl_proc("glViewport");

    p_glAttachShader = (sr_gl_attach_shader)load_gl_proc("glAttachShader");
    p_glBindVertexArray = (sr_gl_bind_vertex_array)load_gl_proc("glBindVertexArray");
    p_glCompileShader = (sr_gl_compile_shader)load_gl_proc("glCompileShader");
    p_glCreateProgram = (sr_gl_create_program)load_gl_proc("glCreateProgram");
    p_glCreateShader = (sr_gl_create_shader)load_gl_proc("glCreateShader");
    p_glDeleteProgram = (sr_gl_delete_program)load_gl_proc("glDeleteProgram");
    p_glDeleteShader = (sr_gl_delete_shader)load_gl_proc("glDeleteShader");
    p_glDeleteVertexArrays = (sr_gl_delete_vertex_arrays)load_gl_proc("glDeleteVertexArrays");
    p_glGenVertexArrays = (sr_gl_gen_vertex_arrays)load_gl_proc("glGenVertexArrays");
    p_glGetUniformLocation = (sr_gl_get_uniform_location)load_gl_proc("glGetUniformLocation");
    p_glGetProgramiv = (sr_gl_get_program_iv)load_gl_proc("glGetProgramiv");
    p_glGetShaderiv = (sr_gl_get_shader_iv)load_gl_proc("glGetShaderiv");
    p_glLinkProgram = (sr_gl_link_program)load_gl_proc("glLinkProgram");
    p_glShaderSource = (sr_gl_shader_source)load_gl_proc("glShaderSource");
    p_glUniform1i = (sr_gl_uniform_1i)load_gl_proc("glUniform1i");
    p_glUseProgram = (sr_gl_use_program)load_gl_proc("glUseProgram");

    return p_glBindTexture && p_glClear && p_glClearColor && p_glDeleteTextures &&
           p_glDisable && p_glDrawArrays && p_glEnable && p_glGenTextures &&
           p_glIsEnabled && p_glPixelStorei && p_glTexImage2D &&
           p_glTexParameteri && p_glTexSubImage2D && p_glViewport &&
           p_glAttachShader && p_glBindVertexArray && p_glCompileShader &&
           p_glCreateProgram && p_glCreateShader && p_glDeleteProgram &&
           p_glDeleteShader && p_glDeleteVertexArrays && p_glGenVertexArrays &&
           p_glGetUniformLocation && p_glGetProgramiv &&
           p_glGetShaderiv && p_glLinkProgram &&
           p_glShaderSource && p_glUniform1i && p_glUseProgram;
}

/* ---------------------------------------------------------------------------
 * Owned-context platform layer.
 *
 * Every one of these is reached only when present->external_context is false,
 * which off Win32 never happens - sr_present_init is the only function that
 * clears the flag and it fails there. They still have to compile, so the
 * non-Win32 side is stubs.
 * ------------------------------------------------------------------------ */

static bool present_make_current(sr_present *present)
{
#ifdef _WIN32
    return wglMakeCurrent((HDC)present->hdc, (HGLRC)present->glrc) != FALSE;
#else
    (void)present;
    return false;
#endif
}

static void present_swap_and_release(sr_present *present)
{
#ifdef _WIN32
    SwapBuffers((HDC)present->hdc);
    wglMakeCurrent(NULL, NULL);
#else
    (void)present;
#endif
}

static void present_client_size(sr_present *present, int *width, int *height)
{
    *width = 0;
    *height = 0;
#ifdef _WIN32
    RECT rect;
    if (present->hwnd && GetClientRect(present->hwnd, &rect)) {
        *width = rect.right - rect.left;
        *height = rect.bottom - rect.top;
    }
#else
    (void)present;
#endif
}

#ifdef _WIN32
static bool choose_window_pixel_format(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd;
    int format;

    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 0;
    pfd.cStencilBits = 0;
    pfd.iLayerType = PFD_MAIN_PLANE;

    format = ChoosePixelFormat(hdc, &pfd);
    return format != 0 && SetPixelFormat(hdc, format, &pfd) != FALSE;
}

static bool make_gl_context(sr_present *present)
{
    HDC hdc = (HDC)present->hdc;
    HGLRC legacy;
    HGLRC modern;
    const int attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };

    if (!choose_window_pixel_format(hdc)) {
        return false;
    }

    legacy = wglCreateContext(hdc);
    if (!legacy || !wglMakeCurrent(hdc, legacy)) {
        if (legacy) {
            wglDeleteContext(legacy);
        }
        return false;
    }

    p_wglCreateContextAttribsARB =
        (wgl_create_context_attribs_arb_proc)wglGetProcAddress("wglCreateContextAttribsARB");
    if (!p_wglCreateContextAttribsARB) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(legacy);
        return false;
    }

    modern = p_wglCreateContextAttribsARB(hdc, NULL, attribs);
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(legacy);

    if (!modern || !wglMakeCurrent(hdc, modern)) {
        if (modern) {
            wglDeleteContext(modern);
        }
        return false;
    }

    present->glrc = modern;
    if (!load_gl_procs()) {
        return false;
    }

    /* PJ64's speed limiter is independent of the driver's swap interval.
     * Keep presentation uncapped when VI VSync is disabled, so SwapBuffers cannot impose the desktop refresh rate. */
    p_wglSwapIntervalEXT =
        (wgl_swap_interval_ext_proc)load_gl_proc("wglSwapIntervalEXT");
    if (p_wglSwapIntervalEXT) {
        (void)p_wglSwapIntervalEXT(0);
    }
    return true;
}
#endif  /* _WIN32 */

static GLuint compile_shader(GLenum type, const char *source)
{
    GLuint shader = p_glCreateShader(type);
    GLint ok = 0;

    p_glShaderSource(shader, 1, &source, NULL);
    p_glCompileShader(shader);
    p_glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        p_glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static GLuint create_program(void)
{
    static const char *vertex_source =
        "#version 330 core\n"
        "out vec2 v_uv;\n"
        "void main(void) {\n"
        "    v_uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n"
        "    gl_Position = vec4(v_uv * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.0, 1.0);\n"
        "}\n";
    static const char *fragment_source =
        "#version 330 core\n"
        "in vec2 v_uv;\n"
        "layout(location = 0) out vec4 out_color;\n"
        "uniform sampler2D u_frame;\n"
        "void main(void) {\n"
        "    out_color = texture(u_frame, v_uv);\n"
        "}\n";

    GLuint vertex = compile_shader(GL_VERTEX_SHADER, vertex_source);
    GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    GLuint program;
    GLint ok = 0;

    if (!vertex || !fragment) {
        if (vertex) {
            p_glDeleteShader(vertex);
        }
        if (fragment) {
            p_glDeleteShader(fragment);
        }
        return 0;
    }

    program = p_glCreateProgram();
    p_glAttachShader(program, vertex);
    p_glAttachShader(program, fragment);
    p_glLinkProgram(program);
    p_glGetProgramiv(program, GL_LINK_STATUS, &ok);
    p_glDeleteShader(fragment);
    p_glDeleteShader(vertex);

    if (!ok) {
        p_glDeleteProgram(program);
        return 0;
    }

    return program;
}

/* Shader, VAO and texture setup, identical for both entry points. The context
 * is expected to be current. */
static bool create_gl_objects(sr_present *present)
{
    GLint sampler;

    present->program = create_program();
    if (!present->program) {
        return false;
    }

    p_glUseProgram(present->program);
    sampler = p_glGetUniformLocation(present->program, "u_frame");
    if (sampler >= 0) {
        p_glUniform1i(sampler, 0);
    }

    p_glGenVertexArrays(1, &present->vao);
    p_glBindVertexArray(present->vao);

    p_glGenTextures(1, &present->texture);
    p_glBindTexture(GL_TEXTURE_2D, present->texture);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    p_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    return true;
}

/* Drop the GL objects. The context must be current; which one that is, and
 * whether it has to be made current first, is the caller's business. */
static void destroy_gl_objects(sr_present *present)
{
    if (present->texture && p_glDeleteTextures) {
        p_glDeleteTextures(1, &present->texture);
    }
    if (present->vao && p_glDeleteVertexArrays) {
        p_glDeleteVertexArrays(1, &present->vao);
    }
    if (present->program && p_glDeleteProgram) {
        p_glDeleteProgram(present->program);
    }
}

bool sr_present_init(sr_present *present, HWND hwnd)
{
#ifdef _WIN32
    if (!present || !hwnd) {
        return false;
    }

    sr_present_shutdown(present);
    memset(present, 0, sizeof(*present));
    present->hwnd = hwnd;
    present->hdc = GetDC(hwnd);
    if (!present->hdc) {
        return false;
    }

    if (!make_gl_context(present)) {
        sr_present_shutdown(present);
        return false;
    }

    if (!create_gl_objects(present)) {
        sr_present_shutdown(present);
        return false;
    }

    present->ready = true;
    sr_present_clear(present);
    return true;
#else
    /* Creating a context from a native window handle is WGL-only here. A
     * front-end on another OS supplies its own context and calls
     * sr_present_init_external instead. */
    (void)present;
    (void)hwnd;
    return false;
#endif
}

bool sr_present_init_external(sr_present *present, void *(*load_proc)(const char *name))
{
    if (!present || !load_proc) {
        return false;
    }

    sr_present_shutdown(present);
    memset(present, 0, sizeof(*present));
    present->external_context = true;
    g_gl_load_proc = load_proc;

    if (!load_gl_procs()) {
        sr_present_shutdown(present);
        return false;
    }

    if (!create_gl_objects(present)) {
        sr_present_shutdown(present);
        return false;
    }

    present->ready = true;
    return true;
}

void sr_present_shutdown(sr_present *present)
{
    if (!present) {
        return;
    }

    if (present->external_context) {
        destroy_gl_objects(present);
    }
#ifdef _WIN32
    else {
        if (present->glrc) {
            if (present_make_current(present)) {
                destroy_gl_objects(present);
                wglMakeCurrent(NULL, NULL);
            }
            wglDeleteContext((HGLRC)present->glrc);
        }

        if (present->hwnd && present->hdc) {
            ReleaseDC(present->hwnd, (HDC)present->hdc);
        }
    }
#endif

    memset(present, 0, sizeof(*present));
}

void sr_present_clear(sr_present *present)
{
    int width;
    int height;

    if (!present || !present->ready) {
        return;
    }

    if (!present->external_context) {
        if (!present_make_current(present)) {
            return;
        }
    }

    if (present->external_context) {
        width = present->window_width ? (int)present->window_width : 640;
        height = present->window_height ? (int)present->window_height : 480;
    } else {
        present_client_size(present, &width, &height);
    }

    GLboolean scissor_test = p_glIsEnabled(GL_SCISSOR_TEST);
    p_glDisable(GL_SCISSOR_TEST);

    p_glViewport(0, 0, width, height);
    p_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    p_glClear(GL_COLOR_BUFFER_BIT);

    if (scissor_test) {
        p_glEnable(GL_SCISSOR_TEST);
    }

    if (!present->external_context) {
        present_swap_and_release(present);
    }
}

void sr_present_set_display_size(sr_present *present, uint32_t width, uint32_t height)
{
    if (!present) return;
    present->display_width = width;
    present->display_height = height;
}

void sr_present_set_window_size(sr_present *present, uint32_t width, uint32_t height)
{
    if (!present) return;
    present->window_width = width;
    present->window_height = height;
}

/* Render the frame into the current framebuffer. Assumes the GL context is
 * already current and does not swap; callers own MakeCurrent/SwapBuffers so a
 * combined upload+draw only pays for a single context switch per frame. */
static void draw_locked(sr_present *present)
{
    int width;
    int height;

    if (present->external_context) {
        width = present->window_width ? (int)present->window_width : 640;
        height = present->window_height ? (int)present->window_height : 480;
    } else {
        present_client_size(present, &width, &height);
    }

    GLboolean depth_test = p_glIsEnabled(GL_DEPTH_TEST);
    GLboolean scissor_test = p_glIsEnabled(GL_SCISSOR_TEST);
    GLboolean blend = p_glIsEnabled(GL_BLEND);
    GLboolean cull_face = p_glIsEnabled(GL_CULL_FACE);

    p_glDisable(GL_DEPTH_TEST);
    p_glDisable(GL_SCISSOR_TEST);
    p_glDisable(GL_BLEND);
    p_glDisable(GL_CULL_FACE);

    p_glViewport(0, 0, width, height);
    p_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    p_glClear(GL_COLOR_BUFFER_BIT);

    if (present->has_frame) {
        /* VI dimensions describe pixels, not a request to stretch them to
         * the host window. Keep the image's aspect ratio and letterbox. */
        int viewport_width = width;
        int viewport_height = height;
        if (present->frame_width && present->frame_height && width > 0 && height > 0) {
            const double window_aspect = (double)width / (double)height;
            const uint32_t aspect_width = present->display_width ?
                                          present->display_width : present->frame_width;
            const uint32_t aspect_height = present->display_height ?
                                           present->display_height : present->frame_height;
            const double frame_aspect = (double)aspect_width / (double)aspect_height;
            if (window_aspect > frame_aspect) {
                viewport_width = (int)((double)height * frame_aspect);
            } else {
                viewport_height = (int)((double)width / frame_aspect);
            }
        }
        p_glViewport((width - viewport_width) / 2, (height - viewport_height) / 2,
                     viewport_width, viewport_height);
        p_glUseProgram(present->program);
        p_glBindVertexArray(present->vao);
        p_glBindTexture(GL_TEXTURE_2D, present->texture);
        p_glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    if (depth_test) p_glEnable(GL_DEPTH_TEST);
    if (scissor_test) p_glEnable(GL_SCISSOR_TEST);
    if (blend) p_glEnable(GL_BLEND);
    if (cull_face) p_glEnable(GL_CULL_FACE);
}

void sr_present_draw(sr_present *present)
{
    if (!present || !present->ready) {
        return;
    }

    if (!present->external_context) {
        if (!present_make_current(present)) {
            return;
        }
    }

    draw_locked(present);

    if (!present->external_context) {
        present_swap_and_release(present);
    }
}

bool sr_present_upload_rgba8(sr_present *present,
                             const sr_rgba8 *pixels,
                             uint32_t width,
                             uint32_t height,
                             uint32_t stride_pixels)
{
    if (!present || !present->ready || !pixels || width == 0 || height == 0 || stride_pixels < width) {
        return false;
    }
    if (!present->external_context) {
        if (!present_make_current(present)) {
            return false;
        }
    }

    p_glBindTexture(GL_TEXTURE_2D, present->texture);
    p_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    p_glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)stride_pixels);

    if (present->frame_width != width || present->frame_height != height) {
        p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height,
                       0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        present->frame_width = width;
        present->frame_height = height;
    } else {
        p_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)width, (GLsizei)height,
                          GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    }

    p_glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    present->has_frame = true;

    /* Context is already current from the upload above: draw without a second
     * MakeCurrent, then swap and release once. */
    draw_locked(present);
    if (!present->external_context) {
        present_swap_and_release(present);
    }
    return true;
}
