// Created: December 19, 2025
// Location: src/utils/PlatformCompat.cpp
// Platform compatibility implementation - OpenGL extension loading for Linux

#include "utils/PlatformCompat.h"
#include <iostream>

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
    // Track which extensions failed to load
    bool all_loaded = true;

    // Helper macro to load and check each function
    #define LOAD_GL_FUNC(name, type) \
        name = (type)SDL_GL_GetProcAddress(#name); \
        if (!name) { \
            std::cerr << "Failed to load OpenGL extension: " #name << std::endl; \
            all_loaded = false; \
        }

    // Load OpenGL extension functions using SDL
    LOAD_GL_FUNC(glGenFramebuffers, PFNGLGENFRAMEBUFFERSPROC)
    LOAD_GL_FUNC(glDeleteFramebuffers, PFNGLDELETEFRAMEBUFFERSPROC)
    LOAD_GL_FUNC(glBindFramebuffer, PFNGLBINDFRAMEBUFFERPROC)
    LOAD_GL_FUNC(glFramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2DPROC)
    LOAD_GL_FUNC(glGenRenderbuffers, PFNGLGENRENDERBUFFERSPROC)
    LOAD_GL_FUNC(glDeleteRenderbuffers, PFNGLDELETERENDERBUFFERSPROC)
    LOAD_GL_FUNC(glBindRenderbuffer, PFNGLBINDRENDERBUFFERPROC)
    LOAD_GL_FUNC(glRenderbufferStorage, PFNGLRENDERBUFFERSTORAGEPROC)
    LOAD_GL_FUNC(glFramebufferRenderbuffer, PFNGLFRAMEBUFFERRENDERBUFFERPROC)
    LOAD_GL_FUNC(glCheckFramebufferStatus, PFNGLCHECKFRAMEBUFFERSTATUSPROC)

    #undef LOAD_GL_FUNC

    if (all_loaded) {
        std::cout << "Successfully loaded all OpenGL framebuffer extensions" << std::endl;
    }

    return all_loaded;
}

bool AreOpenGLExtensionsLoaded() {
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
