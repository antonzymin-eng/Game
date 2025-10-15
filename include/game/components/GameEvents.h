#pragma once

// Minimal stubs for event types used by AI systems
struct MilitaryEvent {
    int eventId = 1;
    float severity = 0.5f;
    uint32_t sourceProvinceId = 0;
    int GetEventId() const { return eventId; }
    float GetSeverity() const { return severity; }
    uint32_t GetSourceProvinceId() const { return sourceProvinceId; }
};

struct DiplomaticEvent {
    int eventId = 2;
    uint32_t nationId = 0;
    int GetEventId() const { return eventId; }
    uint32_t GetNationId() const { return nationId; }
};

struct EconomicEvent {
    int eventId = 3;
    float severity = 0.3f;
    float impact = 0.0f;
    uint32_t sourceProvinceId = 0;
    int GetEventId() const { return eventId; }
    float GetSeverity() const { return severity; }
    float GetImpact() const { return impact; }
    uint32_t GetSourceProvinceId() const { return sourceProvinceId; }
    uint32_t GetProvinceId() const { return sourceProvinceId; }
};
