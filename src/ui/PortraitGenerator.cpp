// Created: November 18, 2025
// Location: src/ui/PortraitGenerator.cpp
// Procedural portrait generator implementation

#include "ui/PortraitGenerator.h"
#include "game/components/CharacterComponent.h"
#include <GL/gl.h>
#include <cmath>
#include <functional>

namespace ui {

namespace {
    // HSV to RGB conversion
    void HSVtoRGB(float h, float s, float v, float& r, float& g, float& b) {
        if (s == 0.0f) {
            r = g = b = v;
            return;
        }

        h = fmod(h, 1.0f) * 6.0f;
        int i = static_cast<int>(h);
        float f = h - i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        switch (i) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            default: r = v; g = p; b = q; break;
        }
    }

    // Simple hash function for strings
    uint64_t HashString(const std::string& str) {
        uint64_t hash = 5381;
        for (char c : str) {
            hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
        }
        return hash;
    }
}

PortraitGenerator::PortraitGenerator()
    : m_framebuffer(0)
    , m_depthbuffer(0)
    , m_initialized(false) {
}

PortraitGenerator::~PortraitGenerator() {
    Shutdown();
}

bool PortraitGenerator::Initialize() {
    if (m_initialized) {
        return true;
    }

    // Create framebuffer for offscreen rendering
    glGenFramebuffers(1, &m_framebuffer);
    glGenRenderbuffers(1, &m_depthbuffer);

    m_initialized = true;
    return true;
}

void PortraitGenerator::Shutdown() {
    if (!m_initialized) {
        return;
    }

    // Delete all cached portraits
    for (auto& pair : m_portraitCache) {
        glDeleteTextures(1, &pair.second.textureId);
    }
    m_portraitCache.clear();

    // Delete framebuffer
    if (m_framebuffer != 0) {
        glDeleteFramebuffers(1, &m_framebuffer);
        m_framebuffer = 0;
    }

    if (m_depthbuffer != 0) {
        glDeleteRenderbuffers(1, &m_depthbuffer);
        m_depthbuffer = 0;
    }

    m_initialized = false;
}

uint64_t PortraitGenerator::HashCharacter(const game::character::CharacterComponent* character) const {
    if (!character) {
        return 0;
    }

    // Hash based on name and key attributes
    uint64_t hash = HashString(character->GetName());
    hash ^= static_cast<uint64_t>(character->GetAge()) << 16;
    hash ^= static_cast<uint64_t>(character->GetDiplomacy()) << 24;
    hash ^= static_cast<uint64_t>(character->GetMartial()) << 32;
    hash ^= static_cast<uint64_t>(character->GetLearning()) << 40;

    return hash;
}

GLuint PortraitGenerator::GetCachedPortrait(const game::character::CharacterComponent* character) {
    if (!character) {
        return 0;
    }

    uint64_t hash = HashCharacter(character);
    auto it = m_portraitCache.find(hash);
    if (it != m_portraitCache.end()) {
        return it->second.textureId;
    }

    return 0;
}

GLuint PortraitGenerator::GeneratePortrait(const game::character::CharacterComponent* character,
                                          int width,
                                          int height) {
    if (!character || !m_initialized) {
        return 0;
    }

    // Check cache first
    uint64_t hash = HashCharacter(character);
    auto it = m_portraitCache.find(hash);
    if (it != m_portraitCache.end()) {
        return it->second.textureId;
    }

    // Generate portrait style from character
    PortraitStyle style = GetStyleFromCharacter(character);

    // Render portrait to texture
    GLuint textureId = RenderPortraitToTexture(style, width, height);

    // Cache the portrait
    Portrait portrait;
    portrait.textureId = textureId;
    portrait.width = width;
    portrait.height = height;
    portrait.characterHash = hash;
    m_portraitCache[hash] = portrait;

    return textureId;
}

PortraitStyle PortraitGenerator::GetStyleFromCharacter(const game::character::CharacterComponent* character) {
    PortraitStyle style;

    if (!character) {
        // Default style
        style.skinHue = 0.08f;
        style.skinSaturation = 0.3f;
        style.skinBrightness = 0.7f;
        style.hairHue = 0.1f;
        style.hairSaturation = 0.4f;
        style.hairBrightness = 0.3f;
        style.hasBeard = true;
        style.hasHair = true;
        style.faceShape = 0.5f;
        style.eyeSize = 0.5f;
        style.noseSize = 0.5f;
        style.bgHue = 0.6f;
        style.bgSaturation = 0.5f;
        style.bgBrightness = 0.4f;
        style.hasCrown = false;
        style.hasRobe = false;
        style.wrinkles = 0.0f;
        style.grayHair = 0.0f;
        return style;
    }

    // Use character name as seed for consistent randomization
    uint64_t seed = HashString(character->GetName());
    auto seededRand = [&seed]() -> float {
        seed = (seed * 1103515245 + 12345) & 0x7fffffff;
        return static_cast<float>(seed % 1000) / 1000.0f;
    };

    // Skin tone variation
    style.skinHue = 0.05f + seededRand() * 0.1f;  // Slight variation
    style.skinSaturation = 0.2f + seededRand() * 0.3f;
    style.skinBrightness = 0.6f + seededRand() * 0.2f;

    // Hair color
    style.hairHue = seededRand();  // Wide variation
    style.hairSaturation = 0.3f + seededRand() * 0.5f;
    style.hairBrightness = 0.2f + seededRand() * 0.5f;

    // Age-based features
    uint32_t age = character->GetAge();
    style.wrinkles = (age > 40) ? (age - 40) / 60.0f : 0.0f;
    style.grayHair = (age > 50) ? (age - 50) / 50.0f : 0.0f;

    // Hair and beard (less likely for older characters to have full hair)
    style.hasHair = seededRand() > (age / 150.0f);
    style.hasBeard = seededRand() > 0.3f;

    // Face features
    style.faceShape = seededRand();
    style.eyeSize = 0.3f + seededRand() * 0.4f;
    style.noseSize = 0.3f + seededRand() * 0.4f;

    // Background based on prestige
    float prestige = character->GetPrestige();
    style.bgHue = seededRand();  // Random hue
    style.bgSaturation = 0.4f + (prestige / 2000.0f) * 0.4f;  // More prestigious = more saturated
    style.bgBrightness = 0.3f + seededRand() * 0.2f;

    // Crown and robe for high prestige characters
    style.hasCrown = prestige > 500.0f;
    style.hasRobe = prestige > 200.0f || character->GetDiplomacy() > 12;

    return style;
}

GLuint PortraitGenerator::RenderPortraitToTexture(const PortraitStyle& style, int width, int height) {
    // Create texture
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Setup framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);

    // Save current viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    // Set viewport to texture size
    glViewport(0, 0, width, height);

    // Setup orthographic projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Clear to transparent
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw portrait elements
    DrawBackground(style, width, height);

    if (style.hasRobe) {
        DrawRobe(style, width, height);
    }

    DrawFace(style, width, height);
    DrawEyes(style, width, height);
    DrawNose(style, width, height);
    DrawMouth(style, width, height);

    if (style.hasHair) {
        DrawHair(style, width, height);
    }

    if (style.hasBeard) {
        DrawBeard(style, width, height);
    }

    if (style.hasCrown) {
        DrawCrown(style, width, height);
    }

    DrawBorder(width, height);

    // Restore matrices
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // Restore viewport
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return textureId;
}

void PortraitGenerator::SetColor(float h, float s, float v, float a) {
    float r, g, b;
    HSVtoRGB(h, s, v, r, g, b);
    glColor4f(r, g, b, a);
}

void PortraitGenerator::DrawFilledCircle(float x, float y, float radius, int segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex2f(x + cos(angle) * radius, y + sin(angle) * radius);
    }
    glEnd();
}

void PortraitGenerator::DrawFilledEllipse(float x, float y, float rx, float ry, int segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex2f(x + cos(angle) * rx, y + sin(angle) * ry);
    }
    glEnd();
}

void PortraitGenerator::DrawFilledRect(float x, float y, float width, float height) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void PortraitGenerator::DrawFilledTriangle(float x1, float y1, float x2, float y2, float x3, float y3) {
    glBegin(GL_TRIANGLES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glVertex2f(x3, y3);
    glEnd();
}

void PortraitGenerator::DrawBackground(const PortraitStyle& style, int width, int height) {
    // Draw background
    SetColor(style.bgHue, style.bgSaturation, style.bgBrightness);
    DrawFilledRect(0, 0, width, height);

    // Add subtle pattern
    SetColor(style.bgHue, style.bgSaturation * 0.8f, style.bgBrightness * 1.1f, 0.3f);

    // Diagonal stripes pattern
    for (int i = -height; i < width + height; i += 20) {
        glBegin(GL_QUADS);
        glVertex2f(i, 0);
        glVertex2f(i + 10, 0);
        glVertex2f(i + 10 - height, height);
        glVertex2f(i - height, height);
        glEnd();
    }
}

void PortraitGenerator::DrawFace(const PortraitStyle& style, int width, int height) {
    float centerX = width * 0.5f;
    float centerY = height * 0.55f;

    // Face shape varies from round (0.0) to oval (1.0)
    float faceWidth = width * 0.35f;
    float faceHeight = height * 0.4f + style.faceShape * height * 0.1f;

    // Draw face
    SetColor(style.skinHue, style.skinSaturation, style.skinBrightness);
    DrawFilledEllipse(centerX, centerY, faceWidth, faceHeight);

    // Add shading
    SetColor(style.skinHue, style.skinSaturation * 1.2f, style.skinBrightness * 0.8f, 0.3f);
    DrawFilledEllipse(centerX - faceWidth * 0.4f, centerY, faceWidth * 0.2f, faceHeight * 0.8f);
    DrawFilledEllipse(centerX + faceWidth * 0.4f, centerY, faceWidth * 0.2f, faceHeight * 0.8f);

    // Add wrinkles for older characters
    if (style.wrinkles > 0.1f) {
        SetColor(style.skinHue, style.skinSaturation * 1.3f, style.skinBrightness * 0.6f, style.wrinkles * 0.4f);
        // Forehead wrinkles
        for (int i = 0; i < 3; i++) {
            float y = centerY - faceHeight * 0.4f + i * 8;
            glLineWidth(1.0f);
            glBegin(GL_LINES);
            glVertex2f(centerX - faceWidth * 0.3f, y);
            glVertex2f(centerX + faceWidth * 0.3f, y);
            glEnd();
        }
    }
}

void PortraitGenerator::DrawEyes(const PortraitStyle& style, int width, int height) {
    float centerX = width * 0.5f;
    float centerY = height * 0.5f;
    float eyeSpacing = width * 0.15f;
    float eyeSize = width * 0.04f * style.eyeSize;

    // Left eye
    SetColor(0.0f, 0.0f, 1.0f);  // White
    DrawFilledEllipse(centerX - eyeSpacing, centerY, eyeSize * 1.5f, eyeSize);

    // Right eye
    DrawFilledEllipse(centerX + eyeSpacing, centerY, eyeSize * 1.5f, eyeSize);

    // Pupils
    SetColor(0.6f, 0.5f, 0.2f);  // Brown/varied
    DrawFilledCircle(centerX - eyeSpacing, centerY, eyeSize * 0.5f);
    DrawFilledCircle(centerX + eyeSpacing, centerY, eyeSize * 0.5f);

    // Highlights
    SetColor(0.0f, 0.0f, 1.0f, 0.7f);  // White highlight
    DrawFilledCircle(centerX - eyeSpacing - eyeSize * 0.2f, centerY - eyeSize * 0.2f, eyeSize * 0.2f);
    DrawFilledCircle(centerX + eyeSpacing - eyeSize * 0.2f, centerY - eyeSize * 0.2f, eyeSize * 0.2f);
}

void PortraitGenerator::DrawNose(const PortraitStyle& style, int width, int height) {
    float centerX = width * 0.5f;
    float centerY = height * 0.55f;
    float noseWidth = width * 0.05f * style.noseSize;
    float noseHeight = height * 0.12f * style.noseSize;

    // Nose shadow
    SetColor(style.skinHue, style.skinSaturation * 1.2f, style.skinBrightness * 0.7f, 0.5f);
    DrawFilledTriangle(
        centerX, centerY - noseHeight * 0.5f,
        centerX - noseWidth, centerY + noseHeight * 0.5f,
        centerX + noseWidth, centerY + noseHeight * 0.5f
    );

    // Nostrils
    SetColor(style.skinHue, style.skinSaturation * 1.3f, style.skinBrightness * 0.5f, 0.6f);
    DrawFilledCircle(centerX - noseWidth * 0.5f, centerY + noseHeight * 0.3f, noseWidth * 0.3f);
    DrawFilledCircle(centerX + noseWidth * 0.5f, centerY + noseHeight * 0.3f, noseWidth * 0.3f);
}

void PortraitGenerator::DrawMouth(const PortraitStyle& style, int width, int height) {
    float centerX = width * 0.5f;
    float centerY = height * 0.65f;
    float mouthWidth = width * 0.12f;

    // Mouth
    SetColor(0.0f, 0.5f, 0.3f, 0.8f);  // Reddish
    DrawFilledEllipse(centerX, centerY, mouthWidth, height * 0.015f);

    // Smile shadow
    SetColor(style.skinHue, style.skinSaturation * 1.2f, style.skinBrightness * 0.7f, 0.3f);
    DrawFilledEllipse(centerX, centerY - height * 0.01f, mouthWidth * 1.1f, height * 0.02f);
}

void PortraitGenerator::DrawHair(const PortraitStyle& style, int width, int height) {
    float centerX = width * 0.5f;
    float centerY = height * 0.55f;
    float faceWidth = width * 0.35f;
    float faceHeight = height * 0.4f + style.faceShape * height * 0.1f;

    // Apply gray for older characters
    float hairBrightness = style.hairBrightness + style.grayHair * (0.8f - style.hairBrightness);
    float hairSaturation = style.hairSaturation * (1.0f - style.grayHair);

    // Hair crown
    SetColor(style.hairHue, hairSaturation, hairBrightness);
    DrawFilledEllipse(centerX, centerY - faceHeight * 0.7f, faceWidth * 1.1f, faceHeight * 0.5f);

    // Side hair
    DrawFilledEllipse(centerX - faceWidth * 0.8f, centerY - faceHeight * 0.2f, faceWidth * 0.3f, faceHeight * 0.6f);
    DrawFilledEllipse(centerX + faceWidth * 0.8f, centerY - faceHeight * 0.2f, faceWidth * 0.3f, faceHeight * 0.6f);
}

void PortraitGenerator::DrawBeard(const PortraitStyle& style, int width, int height) {
    float centerX = width * 0.5f;
    float centerY = height * 0.55f;
    float faceWidth = width * 0.35f;
    float faceHeight = height * 0.4f + style.faceShape * height * 0.1f;

    // Apply gray for older characters
    float beardBrightness = style.hairBrightness + style.grayHair * (0.8f - style.hairBrightness);
    float beardSaturation = style.hairSaturation * (1.0f - style.grayHair);

    // Beard
    SetColor(style.hairHue, beardSaturation, beardBrightness);
    DrawFilledEllipse(centerX, centerY + faceHeight * 0.6f, faceWidth * 0.7f, faceHeight * 0.4f);

    // Mustache
    DrawFilledEllipse(centerX - faceWidth * 0.25f, centerY + faceHeight * 0.1f, faceWidth * 0.25f, faceHeight * 0.1f);
    DrawFilledEllipse(centerX + faceWidth * 0.25f, centerY + faceHeight * 0.1f, faceWidth * 0.25f, faceHeight * 0.1f);
}

void PortraitGenerator::DrawCrown(const PortraitStyle& style, int width, int height) {
    float centerX = width * 0.5f;
    float centerY = height * 0.2f;
    float crownWidth = width * 0.4f;
    float crownHeight = height * 0.1f;

    // Crown base
    SetColor(0.15f, 0.8f, 0.9f);  // Gold
    DrawFilledRect(centerX - crownWidth * 0.5f, centerY, crownWidth, crownHeight * 0.3f);

    // Crown points
    for (int i = 0; i < 5; i++) {
        float x = centerX - crownWidth * 0.4f + i * crownWidth * 0.2f;
        DrawFilledTriangle(
            x, centerY,
            x - crownWidth * 0.08f, centerY + crownHeight * 0.3f,
            x + crownWidth * 0.08f, centerY + crownHeight * 0.3f
        );
    }

    // Jewels
    SetColor(0.0f, 0.8f, 0.7f);  // Red jewel
    for (int i = 0; i < 5; i++) {
        float x = centerX - crownWidth * 0.4f + i * crownWidth * 0.2f;
        DrawFilledCircle(x, centerY + crownHeight * 0.15f, width * 0.015f);
    }
}

void PortraitGenerator::DrawRobe(const PortraitStyle& style, int width, int height) {
    float centerX = width * 0.5f;
    float centerY = height * 0.85f;
    float robeWidth = width * 0.7f;
    float robeHeight = height * 0.4f;

    // Robe body
    SetColor(style.bgHue + 0.5f, 0.6f, 0.4f);  // Complementary color to background
    DrawFilledRect(centerX - robeWidth * 0.5f, centerY - robeHeight, robeWidth, robeHeight);

    // Collar
    SetColor(style.bgHue + 0.5f, 0.7f, 0.5f);
    DrawFilledRect(centerX - robeWidth * 0.3f, centerY - robeHeight, robeWidth * 0.6f, robeHeight * 0.1f);

    // Decorative trim
    SetColor(0.15f, 0.8f, 0.8f);  // Gold trim
    DrawFilledRect(centerX - robeWidth * 0.5f, centerY - robeHeight, robeWidth, robeHeight * 0.05f);
    DrawFilledRect(centerX - robeWidth * 0.5f, centerY - robeHeight * 0.05f, robeWidth * 0.05f, robeHeight);
    DrawFilledRect(centerX + robeWidth * 0.45f, centerY - robeHeight * 0.05f, robeWidth * 0.05f, robeHeight);
}

void PortraitGenerator::DrawBorder(int width, int height) {
    // Outer border
    SetColor(0.15f, 0.7f, 0.5f);  // Gold border
    glLineWidth(4.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(2, 2);
    glVertex2f(width - 2, 2);
    glVertex2f(width - 2, height - 2);
    glVertex2f(2, height - 2);
    glEnd();

    // Inner border
    SetColor(0.15f, 0.5f, 0.3f);  // Darker gold
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(6, 6);
    glVertex2f(width - 6, 6);
    glVertex2f(width - 6, height - 6);
    glVertex2f(6, height - 6);
    glEnd();
}

void PortraitGenerator::ClearCache() {
    for (auto& pair : m_portraitCache) {
        glDeleteTextures(1, &pair.second.textureId);
    }
    m_portraitCache.clear();
}

} // namespace ui
