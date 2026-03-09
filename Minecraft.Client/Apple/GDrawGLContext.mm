// GDrawGLContext.mm - macOS CGL offscreen OpenGL context for GDraw/Iggy
//
// Creates a headless OpenGL 2.1 Legacy Profile context via CGL so the
// GDraw GL backend can compile shaders, create textures, and render
// Iggy SWF content.  The result is read back via glReadPixels for
// compositing into the Vulkan framebuffer.

#include "GDrawGLContext.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <stdio.h>
#include <string.h>

static CGLContextObj g_cglContext = NULL;
static GLuint g_fbo = 0;
static GLuint g_colorTex = 0;
static GLuint g_depthStencilRB = 0;
static int g_width = 0;
static int g_height = 0;

int GDrawGLContext_Create(int width, int height)
{
  // Request a Legacy Profile (OpenGL 2.1) — supports all the GLSL 1.10
  // shaders and legacy features (GL_ALPHA, GL_LUMINANCE, etc.) that
  // GDraw requires.  macOS supports this up to 2.1.
  CGLPixelFormatAttribute attrs[] = {
    kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute)kCGLOGLPVersion_Legacy,
    kCGLPFAColorSize, (CGLPixelFormatAttribute)32,
    kCGLPFADepthSize, (CGLPixelFormatAttribute)24,
    kCGLPFAStencilSize, (CGLPixelFormatAttribute)8,
    kCGLPFAAccelerated,
    kCGLPFAAllowOfflineRenderers,
    (CGLPixelFormatAttribute)0
  };

  CGLPixelFormatObj pixelFormat = NULL;
  GLint numPixelFormats = 0;
  CGLError err = CGLChoosePixelFormat(attrs, &pixelFormat, &numPixelFormats);
  if (err != kCGLNoError || pixelFormat == NULL) {
    fprintf(stderr, "[GDrawGL] CGLChoosePixelFormat failed: %s\n",
            CGLErrorString(err));
    return -1;
  }

  err = CGLCreateContext(pixelFormat, NULL, &g_cglContext);
  CGLDestroyPixelFormat(pixelFormat);
  if (err != kCGLNoError || g_cglContext == NULL) {
    fprintf(stderr, "[GDrawGL] CGLCreateContext failed: %s\n",
            CGLErrorString(err));
    return -1;
  }

  CGLSetCurrentContext(g_cglContext);

  g_width = width;
  g_height = height;

  fprintf(stderr, "[GDrawGL] GL Vendor:   %s\n", glGetString(GL_VENDOR));
  fprintf(stderr, "[GDrawGL] GL Renderer: %s\n", glGetString(GL_RENDERER));
  fprintf(stderr, "[GDrawGL] GL Version:  %s\n", glGetString(GL_VERSION));

  // Create an FBO so GDraw renders to an offscreen texture
  glGenFramebuffersEXT(1, &g_fbo);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_fbo);

  // Color attachment — RGBA texture
  glGenTextures(1, &g_colorTex);
  glBindTexture(GL_TEXTURE_2D, g_colorTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                            GL_TEXTURE_2D, g_colorTex, 0);

  // Depth+stencil attachment — packed renderbuffer
  glGenRenderbuffersEXT(1, &g_depthStencilRB);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, g_depthStencilRB);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT,
                           width, height);
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                               GL_RENDERBUFFER_EXT, g_depthStencilRB);
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT,
                               GL_RENDERBUFFER_EXT, g_depthStencilRB);

  GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
    fprintf(stderr, "[GDrawGL] FBO incomplete: 0x%x\n", status);
    GDrawGLContext_Destroy();
    return -1;
  }

  // Clear to transparent
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  fprintf(stderr, "[GDrawGL] Offscreen GL context ready (%dx%d)\n",
          width, height);
  return 0;
}

void GDrawGLContext_MakeCurrent(void)
{
  if (g_cglContext != NULL) {
    CGLSetCurrentContext(g_cglContext);
  }
}

void GDrawGLContext_ClearCurrent(void)
{
  CGLSetCurrentContext(NULL);
}

void GDrawGLContext_Destroy(void)
{
  if (g_cglContext != NULL) {
    CGLSetCurrentContext(g_cglContext);

    if (g_fbo) {
      glDeleteFramebuffersEXT(1, &g_fbo);
      g_fbo = 0;
    }
    if (g_colorTex) {
      glDeleteTextures(1, &g_colorTex);
      g_colorTex = 0;
    }
    if (g_depthStencilRB) {
      glDeleteRenderbuffersEXT(1, &g_depthStencilRB);
      g_depthStencilRB = 0;
    }

    CGLSetCurrentContext(NULL);
    CGLDestroyContext(g_cglContext);
    g_cglContext = NULL;
  }
}

void GDrawGLContext_ReadPixels(int width, int height, unsigned char *rgbaOut)
{
  if (g_cglContext == NULL || rgbaOut == NULL) return;

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_fbo);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgbaOut);
}
