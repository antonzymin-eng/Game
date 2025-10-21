/**
 * @file example_platform_usage.cpp
 * @brief Example demonstrating cross-platform compatibility patterns
 * 
 * This file shows the recommended way to write platform-independent code
 * using the PlatformCompat.h header.
 * 
 * Created: October 21, 2025
 */

#include "utils/PlatformCompat.h"
#include <iostream>
#include <fstream>

/**
 * Example 1: JsonCpp usage (automatically handles header path differences)
 */
void LoadConfiguration(const std::string& filename) {
    // No need for #ifdef - PlatformCompat.h includes the right header
    std::ifstream file(filename);
    Json::Value root;
    Json::Reader reader;
    
    if (reader.parse(file, root)) {
        std::cout << "Config loaded: " << root["name"].asString() << std::endl;
    }
}

/**
 * Example 2: Path handling (cross-platform)
 */
void LoadGameData() {
    // Automatically uses correct path separator (\ on Windows, / on Linux)
    std::string configPath = PlatformUtils::JoinPath("data", "config.json");
    std::string savePath = PlatformUtils::JoinPath("saves", "game1.sav");
    
    // Normalize paths from mixed separators
    std::string mixedPath = "data\\config/settings.json";
    std::string normalized = PlatformUtils::NormalizePath(mixedPath);
    
    std::cout << "Config: " << configPath << std::endl;
    std::cout << "Save: " << savePath << std::endl;
    std::cout << "Normalized: " << normalized << std::endl;
}

/**
 * Example 3: OpenGL initialization (automatically uses GLAD on Windows, GL on Linux)
 */
void InitializeOpenGL() {
    #ifdef PLATFORM_WINDOWS
        // Windows: GLAD is already included via PlatformCompat.h
        if (!gladLoadGL()) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            return;
        }
    #endif
    
    // Now you can use OpenGL functions on both platforms
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "OpenGL Version: " << version << std::endl;
}

/**
 * Example 4: ImGui key handling (compatible with old and new ImGui versions)
 */
void HandleKeyInput() {
    // Works with both ImGui 1.89- (KeysDown[]) and 1.89+ (IsKeyDown())
    if (ImGuiCompat::IsKeyDown(SDLK_w)) {
        std::cout << "W key is held down" << std::endl;
    }
    
    if (ImGuiCompat::IsKeyPressed(SDLK_SPACE)) {
        std::cout << "Space key was just pressed" << std::endl;
    }
}

/**
 * Example 5: Debug utilities
 */
void DebugExample() {
    // Platform-agnostic debug output
    DEBUG_OUTPUT("This message goes to debugger on Windows, stderr on Linux\n");
    
    // Platform-agnostic assertions (only in debug builds)
    int* ptr = nullptr;
    PLATFORM_ASSERT(ptr != nullptr, "Pointer is null!");
    
    // Platform-agnostic breakpoint
    // DEBUG_BREAK();  // Uncomment to trigger debugger
}

/**
 * Example 6: Platform-specific code (when really needed)
 */
void PlatformSpecificFeature() {
    #ifdef PLATFORM_WINDOWS
        std::cout << "Running on Windows" << std::endl;
        // Windows-specific code here
        // e.g., registry access, COM APIs, etc.
    #endif
    
    #ifdef PLATFORM_LINUX
        std::cout << "Running on Linux" << std::endl;
        // Linux-specific code here
        // e.g., sysfs access, D-Bus, etc.
    #endif
}

/**
 * Example 7: OpenSSL usage (same on both platforms)
 */
void ComputeHash(const std::string& data) {
    // PlatformCompat.h includes <openssl/sha.h>
    unsigned char hash[SHA256_DIGEST_LENGTH];
    
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.c_str(), data.length());
    SHA256_Final(hash, &sha256);
    #pragma GCC diagnostic pop
    
    std::cout << "SHA256 hash computed" << std::endl;
}

/**
 * Main function - demonstrates all examples
 */
int main() {
    std::cout << "=== Cross-Platform Compatibility Examples ===" << std::endl;
    std::cout << std::endl;
    
    // Platform detection
    #ifdef PLATFORM_WINDOWS
        std::cout << "Platform: Windows" << std::endl;
    #elif defined(PLATFORM_LINUX)
        std::cout << "Platform: Linux" << std::endl;
    #else
        std::cout << "Platform: Unknown" << std::endl;
    #endif
    
    std::cout << "Path separator: '" << PATH_SEPARATOR << "'" << std::endl;
    std::cout << "Line ending: " << (LINE_ENDING[1] == '\n' ? "LF" : "CRLF") << std::endl;
    std::cout << std::endl;
    
    // Run examples
    LoadGameData();
    std::cout << std::endl;
    
    PlatformSpecificFeature();
    std::cout << std::endl;
    
    std::cout << "=== All examples completed ===" << std::endl;
    
    return 0;
}
