// GDrawGLContext.h - macOS CGL offscreen OpenGL context for GDraw/Iggy
//
// Creates a headless OpenGL 2.1 Legacy Profile context so that the GDraw GL
// backend can render Iggy Flash UI without a visible OpenGL window.
// The Vulkan window remains the primary display surface.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Create an offscreen CGL OpenGL context and make it current.
// Returns 0 on success, non-zero on failure.
int GDrawGLContext_Create(int width, int height);

// Make the GDraw GL context current on this thread.
void GDrawGLContext_MakeCurrent(void);

// Clear the current context (release from this thread).
void GDrawGLContext_ClearCurrent(void);

// Destroy the offscreen GL context.
void GDrawGLContext_Destroy(void);

// Read back the current GL framebuffer into an RGBA pixel buffer.
// Caller must provide a buffer of at least width*height*4 bytes.
void GDrawGLContext_ReadPixels(int width, int height, unsigned char *rgbaOut);

#ifdef __cplusplus
}
#endif
