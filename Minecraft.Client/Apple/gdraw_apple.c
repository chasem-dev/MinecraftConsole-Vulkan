// gdraw_apple.c - macOS GDraw GL backend for Iggy
//
// Adapted from gdraw_wgl.c (Windows GL backend).
// Uses macOS OpenGL 2.1 Legacy Profile via CGL offscreen context.
// All GL functions are available directly (no wglGetProcAddress needed).

#define GDRAW_ASSERTS

#include "iggy.h"
#include "gdraw.h"
#include "gdraw_wgl.h"     // reuse the same public header (API is identical)

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define true 1
#define false 0

///////////////////////////////////////////////////////////////////////////////
//
//  On macOS, GL 2.0 functions are available directly — no extension loading
//  needed.  We just alias the ARB extension names to the core names.
//

// macOS GL 2.1 provides these as core functions.
// The gdraw_gl_shared.inl code references them via gl## names set up by the
// extension list.  We map directly to the core GL functions.
#define glActiveTexture          glActiveTexture
#define glCompressedTexImage2D   glCompressedTexImage2D
#define glGenBuffers             glGenBuffers
#define glDeleteBuffers          glDeleteBuffers
#define glBindBuffer             glBindBuffer
#define glBufferData             glBufferData
#define glMapBuffer              glMapBuffer
#define glUnmapBuffer            glUnmapBuffer
#define glVertexAttribPointer    glVertexAttribPointer
#define glEnableVertexAttribArray  glEnableVertexAttribArray
#define glDisableVertexAttribArray glDisableVertexAttribArray
#define glCreateShader           glCreateShader
#define glDeleteShader           glDeleteShader
#define glShaderSource           glShaderSource
#define glCompileShader          glCompileShader
#define glGetShaderiv            glGetShaderiv
#define glGetShaderInfoLog       glGetShaderInfoLog
#define glCreateProgram          glCreateProgram
#define glDeleteProgram          glDeleteProgram
#define glAttachShader           glAttachShader
#define glLinkProgram            glLinkProgram
#define glGetUniformLocation     glGetUniformLocation
#define glUseProgram             glUseProgram
#define glGetProgramiv           glGetProgramiv
#define glGetProgramInfoLog      glGetProgramInfoLog
#define glUniform1i              glUniform1i
#define glUniform4f              glUniform4f
#define glUniform4fv             glUniform4fv
#define glBindAttribLocation     glBindAttribLocation
#define glGenRenderbuffersEXT    glGenRenderbuffersEXT
#define glDeleteRenderbuffersEXT glDeleteRenderbuffersEXT
#define glBindRenderbufferEXT    glBindRenderbufferEXT
#define glRenderbufferStorageEXT glRenderbufferStorageEXT
#define glGenFramebuffersEXT     glGenFramebuffersEXT
#define glDeleteFramebuffersEXT  glDeleteFramebuffersEXT
#define glBindFramebufferEXT     glBindFramebufferEXT
#define glCheckFramebufferStatusEXT glCheckFramebufferStatusEXT
#define glFramebufferRenderbufferEXT glFramebufferRenderbufferEXT
#define glFramebufferTexture2DEXT   glFramebufferTexture2DEXT
#define glGenerateMipmapEXT      glGenerateMipmapEXT
#define glBlitFramebufferEXT     glBlitFramebufferEXT
#define glRenderbufferStorageMultisampleEXT glRenderbufferStorageMultisampleEXT

// Map the GL_EXT names that gdraw_gl_shared.inl uses to the function names
// macOS provides directly.  The shared code calls these via the gl## pointers
// set up by the extension list, but on macOS they're just regular functions.
// We use a different approach: since the shared .inl code references
// gl##id from the extension list, we define those identifiers as macros
// pointing to the real functions.
//
// NOTE: The extension list in gdraw_wgl.c defines function pointer variables
// named gl##id.  Since we're on macOS where all these are real functions,
// we skip the extension list entirely and instead map via the
// gdraw_gl_shared.inl's usage patterns.

// Framebuffer functions: the shared code uses GL_FRAMEBUFFER (not _EXT suffix)
// On macOS GL 2.1 Legacy, the EXT versions are available.
// We provide the non-EXT names as aliases.
#define glGenRenderbuffers          glGenRenderbuffersEXT
#define glDeleteRenderbuffers       glDeleteRenderbuffersEXT
#define glBindRenderbuffer          glBindRenderbufferEXT
#define glRenderbufferStorage       glRenderbufferStorageEXT
#define glGenFramebuffers           glGenFramebuffersEXT
#define glDeleteFramebuffers        glDeleteFramebuffersEXT
#define glBindFramebuffer           glBindFramebufferEXT
#define glCheckFramebufferStatus    glCheckFramebufferStatusEXT
#define glFramebufferRenderbuffer   glFramebufferRenderbufferEXT
#define glFramebufferTexture2D      glFramebufferTexture2DEXT
#define glGenerateMipmap            glGenerateMipmapEXT
#define glBlitFramebuffer           glBlitFramebufferEXT
#define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleEXT

// GL constants that may not be defined in macOS headers with non-EXT names
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER              GL_FRAMEBUFFER_EXT
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER             GL_RENDERBUFFER_EXT
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0        GL_COLOR_ATTACHMENT0_EXT
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT         GL_DEPTH_ATTACHMENT_EXT
#endif
#ifndef GL_STENCIL_ATTACHMENT
#define GL_STENCIL_ATTACHMENT       GL_STENCIL_ATTACHMENT_EXT
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE     GL_FRAMEBUFFER_COMPLETE_EXT
#endif
#ifndef GL_MAX_SAMPLES
#define GL_MAX_SAMPLES              GL_MAX_SAMPLES_EXT
#endif
#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER         GL_READ_FRAMEBUFFER_EXT
#endif
#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER         GL_DRAW_FRAMEBUFFER_EXT
#endif
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING      GL_FRAMEBUFFER_BINDING_EXT
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS              0x8B82
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS           0x8B81
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH          0x8B84
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER          0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER            0x8B31
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER             0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER     0x8893
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW              0x88E4
#endif
#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY               0x88B9
#endif
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL        0x813D
#endif

// On macOS, GLhandle = GLuint (not GLhandleARB)
// The shared code has a provision for this: define GDrawGLProgram before
// including gdraw_gl_shared.inl.
#define GDrawGLProgram GLuint

typedef GLuint GLhandle;
typedef gdraw_gl_resourcetype gdraw_resourcetype;

#define gdraw_GLx_(id)     gdraw_GL_##id
#define GDRAW_GLx_(id)     GDRAW_GL_##id
#define GDRAW_SHADERS      "gdraw_gl_shaders.inl"

// No extension loading needed on macOS — all GL 2.0 functions are core
static void load_extensions(void)
{
  // Nothing to do. All functions available directly.
}

static void clear_renderstate_platform_specific(void)
{
  glDisable(GL_ALPHA_TEST);
}

static void error_msg_platform_specific(const char *msg)
{
  fprintf(stderr, "[GDraw] %s\n", msg);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Shared code
//

#define GDRAW_MULTISAMPLING
#include "gdraw_gl_shared.inl"

///////////////////////////////////////////////////////////////////////////////
//
//  Initialization and platform-specific functionality
//

GDrawFunctions *gdraw_GL_CreateContext(S32 w, S32 h, S32 msaa_samples)
{
  static const TextureFormatDesc tex_formats[] = {
    { IFT_FORMAT_rgba_8888,    1, 1,  4,   GL_RGBA,                            GL_RGBA,               GL_UNSIGNED_BYTE },
    { IFT_FORMAT_rgba_4444_LE, 1, 1,  2,   GL_RGBA4,                           GL_RGBA,               GL_UNSIGNED_SHORT_4_4_4_4 },
    { IFT_FORMAT_rgba_5551_LE, 1, 1,  2,   GL_RGB5_A1,                         GL_RGBA,               GL_UNSIGNED_SHORT_5_5_5_1 },
    { IFT_FORMAT_la_88,        1, 1,  2,   GL_LUMINANCE8_ALPHA8,               GL_LUMINANCE_ALPHA,    GL_UNSIGNED_BYTE },
    { IFT_FORMAT_la_44,        1, 1,  1,   GL_LUMINANCE4_ALPHA4,               GL_LUMINANCE_ALPHA,    GL_UNSIGNED_BYTE },
    { IFT_FORMAT_i_8,          1, 1,  1,   GL_INTENSITY8,                      GL_ALPHA,              GL_UNSIGNED_BYTE },
    { IFT_FORMAT_i_4,          1, 1,  1,   GL_INTENSITY4,                      GL_ALPHA,              GL_UNSIGNED_BYTE },
    { IFT_FORMAT_l_8,          1, 1,  1,   GL_LUMINANCE8,                      GL_LUMINANCE,          GL_UNSIGNED_BYTE },
    { IFT_FORMAT_l_4,          1, 1,  1,   GL_LUMINANCE4,                      GL_LUMINANCE,          GL_UNSIGNED_BYTE },
    { IFT_FORMAT_DXT1,         4, 4,  8,   GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   0,                     GL_UNSIGNED_BYTE },
    { IFT_FORMAT_DXT3,         4, 4, 16,   GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,   0,                     GL_UNSIGNED_BYTE },
    { IFT_FORMAT_DXT5,         4, 4, 16,   GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,   0,                     GL_UNSIGNED_BYTE },
    { 0,                       0, 0,  0,   0,                                  0,                     0 },
  };

  GDrawFunctions *funcs;
  const char *s;
  GLint n;

  // The CGL context must already be current when this is called.
  s = (const char *)glGetString(GL_EXTENSIONS);
  if (s == NULL) {
    fprintf(stderr, "[GDraw] glGetString(GL_EXTENSIONS) returned NULL — "
                    "no GL context current?\n");
    return NULL;
  }

  // On macOS GL 2.1, all required extensions are part of core or guaranteed
  // available.  Skip the strict extension checks from gdraw_wgl.c —
  // macOS GL 2.1 Legacy Profile has:
  //   - Multitexture (core since GL 1.3)
  //   - Texture compression (core since GL 1.3)
  //   - Mirrored repeat (core since GL 1.4)
  //   - NPOT textures (core since GL 2.0)
  //   - VBOs (core since GL 1.5)
  //   - FBOs (available as EXT)
  //   - GLSL shaders (core since GL 2.0)

  load_extensions();
  funcs = create_context(w, h);
  if (!funcs)
    return NULL;

  gdraw->tex_formats = tex_formats;

  // macOS GL 2.1 capabilities
  gdraw->has_mapbuffer = true;
  gdraw->has_depth24 = true;
  gdraw->has_texture_max_level = true;

  if (hasext(s, "GL_EXT_packed_depth_stencil"))
    gdraw->has_packed_depth_stencil = true;

  // NPOT support: macOS GL 2.0+ has full NPOT
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &n);
  gdraw->has_conditional_non_power_of_two = n < 8192;

  // Multisampling
  if (msaa_samples > 1) {
    if (hasext(s, "GL_EXT_framebuffer_multisample")) {
      glGetIntegerv(GL_MAX_SAMPLES, &n);
      gdraw->multisampling = RR_MIN(msaa_samples, n);
    }
  }

  opengl_check();

  fprintf(stderr, "[GDraw] GL context created successfully (%dx%d)\n", w, h);
  return funcs;
}
