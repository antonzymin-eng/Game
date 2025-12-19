// Created: December 19, 2025
// Location: src/utils/PlatformCompat.cpp
// Platform compatibility implementation - OpenGL extension loading for Linux

#include "utils/PlatformCompat.h"

#ifdef PLATFORM_LINUX

// Define OpenGL extension function pointers for Linux
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = nullptr;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = nullptr;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = nullptr;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = nullptr;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = nullptr;

namespace PlatformUtils {

bool InitializeOpenGLExtensions() {
    // Load OpenGL extension functions using SDL
    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glGenFramebuffers");
    glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteFramebuffers");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBindFramebuffer");
    glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)SDL_GL_GetProcAddress("glFramebufferTexture2D");
    glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)SDL_GL_GetProcAddress("glGenRenderbuffers");
    glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteRenderbuffers");
    glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)SDL_GL_GetProcAddress("glBindRenderbuffer");
    glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)SDL_GL_GetProcAddress("glRenderbufferStorage");
    glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)SDL_GL_GetProcAddress("glFramebufferRenderbuffer");
    glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)SDL_GL_GetProcAddress("glCheckFramebufferStatus");

    // Check if all functions were loaded successfully
    return glGenFramebuffers != nullptr &&
           glDeleteFramebuffers != nullptr &&
           glBindFramebuffer != nullptr &&
           glFramebufferTexture2D != nullptr &&
           glGenRenderbuffers != nullptr &&
           glDeleteRenderbuffers != nullptr &&
           glBindRenderbuffer != nullptr &&
           glRenderbufferStorage != nullptr &&
           glFramebufferRenderbuffer != nullptr &&
           glCheckFramebufferStatus != nullptr;
}

} // namespace PlatformUtils

#endif // PLATFORM_LINUX
