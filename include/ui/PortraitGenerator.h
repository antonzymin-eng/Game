// Created: November 18, 2025
// Location: include/ui/PortraitGenerator.h
// Procedural portrait generator for character portraits

#pragma once

#include <GL/gl.h>
#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>

namespace game {
namespace character {
    class CharacterComponent;
}
}

namespace ui {

// Portrait visual style data
struct PortraitStyle {
    // Face colors
    float skinHue;          // 0.0-1.0
    float skinSaturation;   // 0.0-1.0
    float skinBrightness;   // 0.0-1.0

    // Hair
    float hairHue;
    float hairSaturation;
    float hairBrightness;
    bool hasBeard;
    bool hasHair;

    // Features
    float faceShape;        // 0.0-1.0 (round to angular)
    float eyeSize;          // 0.0-1.0
    float noseSize;         // 0.0-1.0

    // Background
    float bgHue;
    float bgSaturation;
    float bgBrightness;

    // Accessories
    bool hasCrown;
    bool hasRobe;

    // Age markers
    float wrinkles;         // 0.0-1.0
    float grayHair;         // 0.0-1.0
};

// Generated portrait texture
struct Portrait {
    GLuint textureId;
    int width;
    int height;
    uint64_t characterHash;  // Hash of character data used to generate
};

// Procedural portrait generator
class PortraitGenerator {
public:
    PortraitGenerator();
    ~PortraitGenerator();

    // Delete copy constructor and assignment operator
    PortraitGenerator(const PortraitGenerator&) = delete;
    PortraitGenerator& operator=(const PortraitGenerator&) = delete;

    // Initialize OpenGL resources
    bool Initialize();
    void Shutdown();

    // Generate a portrait for a character
    // Returns texture ID that can be used with ImGui::Image
    GLuint GeneratePortrait(const game::character::CharacterComponent* character,
                           int width = 256,
                           int height = 256);

    // Get cached portrait if it exists
    GLuint GetCachedPortrait(const game::character::CharacterComponent* character);

    // Clear portrait cache
    void ClearCache();

    // Get portrait style from character attributes
    static PortraitStyle GetStyleFromCharacter(const game::character::CharacterComponent* character);

private:
    // Generate unique hash from character data
    uint64_t HashCharacter(const game::character::CharacterComponent* character) const;

    // Render portrait to texture
    GLuint RenderPortraitToTexture(const PortraitStyle& style, int width, int height);

    // Drawing functions
    void DrawBackground(const PortraitStyle& style, int width, int height);
    void DrawFace(const PortraitStyle& style, int width, int height);
    void DrawEyes(const PortraitStyle& style, int width, int height);
    void DrawNose(const PortraitStyle& style, int width, int height);
    void DrawMouth(const PortraitStyle& style, int width, int height);
    void DrawHair(const PortraitStyle& style, int width, int height);
    void DrawBeard(const PortraitStyle& style, int width, int height);
    void DrawCrown(const PortraitStyle& style, int width, int height);
    void DrawRobe(const PortraitStyle& style, int width, int height);
    void DrawBorder(int width, int height);

    // Helper functions
    void SetColor(float h, float s, float v, float a = 1.0f);
    void DrawFilledCircle(float x, float y, float radius, int segments = 32);
    void DrawFilledEllipse(float x, float y, float rx, float ry, int segments = 32);
    void DrawFilledRect(float x, float y, float width, float height);
    void DrawFilledTriangle(float x1, float y1, float x2, float y2, float x3, float y3);

    // OpenGL framebuffer for rendering
    GLuint m_framebuffer;
    GLuint m_depthbuffer;

    // Portrait cache: character hash -> texture ID
    std::unordered_map<uint64_t, Portrait> m_portraitCache;

    // Initialization state
    bool m_initialized;
};

} // namespace ui
