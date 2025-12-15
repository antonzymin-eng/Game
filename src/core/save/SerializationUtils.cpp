// Created: December 5, 2025
// Location: src/core/save/SerializationUtils.cpp
// Purpose: Implementation of serialization utilities

#include "core/save/SerializationUtils.h"
#include "core/ECS/EntityManager.h"
#include <json/json.h>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <zlib.h>
#include <vector>
#include <algorithm>

namespace game::core::serialization {

// =============================================================================
// CRC32 Implementation (IEEE 802.3 polynomial)
// =============================================================================

// CRC32 lookup table
static uint32_t crc32_table[256];
static bool crc32_table_initialized = false;

static void InitCRC32Table() {
    if (crc32_table_initialized) return;

    const uint32_t polynomial = 0xEDB88320;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = true;
}

uint32_t CalculateCRC32(const std::string& data) {
    InitCRC32Table();

    uint32_t crc = 0xFFFFFFFF;
    for (unsigned char byte : data) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ byte) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

std::string WrapWithChecksum(const std::string& data) {
    // Calculate checksum
    uint32_t checksum = CalculateCRC32(data);

    // Create wrapper JSON
    Json::Value wrapper;
    wrapper["version"] = 1;
    wrapper["checksum"] = static_cast<Json::UInt>(checksum);
    wrapper["data"] = data;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, wrapper);
}

bool UnwrapAndValidate(const std::string& wrapped, std::string& out_data) {
    // Parse wrapper JSON
    Json::Value wrapper;
    Json::CharReaderBuilder builder;
    std::stringstream ss(wrapped);
    std::string errors;

    if (!Json::parseFromStream(builder, ss, &wrapper, &errors)) {
        return false;
    }

    // Validate structure
    if (!wrapper.isMember("checksum") || !wrapper.isMember("data")) {
        return false;
    }

    // Extract data
    if (!wrapper["data"].isString()) {
        return false;
    }
    out_data = wrapper["data"].asString();

    // Verify checksum
    uint32_t stored_checksum = wrapper["checksum"].asUInt();
    uint32_t calculated_checksum = CalculateCRC32(out_data);

    return (stored_checksum == calculated_checksum);
}

// =============================================================================
// Compression (zlib/deflate)
// =============================================================================

std::string Compress(const std::string& data) {
    if (data.empty()) return "";

    // Calculate maximum compressed size
    uLongf compressed_size = compressBound(static_cast<uLong>(data.size()));
    std::vector<Bytef> compressed_buffer(compressed_size);

    // Compress using zlib
    int result = compress2(
        compressed_buffer.data(),
        &compressed_size,
        reinterpret_cast<const Bytef*>(data.c_str()),
        static_cast<uLong>(data.size()),
        Z_DEFAULT_COMPRESSION  // Level 6, good balance of speed and compression
    );

    if (result != Z_OK) {
        // Compression failed, return uncompressed with header indicating no compression
        return "ZLIB:NONE:" + data;
    }

    // Resize buffer to actual compressed size
    compressed_buffer.resize(compressed_size);

    // Convert to string with header
    std::string compressed_str(
        reinterpret_cast<const char*>(compressed_buffer.data()),
        compressed_size
    );

    // Add header with format version and original size
    std::stringstream header;
    header << "ZLIB1:" << data.size() << ":";

    return header.str() + compressed_str;
}

std::string Decompress(const std::string& compressed) {
    // Check for zlib header
    if (compressed.size() < 8 || compressed.substr(0, 6) != "ZLIB1:") {
        // Try legacy RLE format for backwards compatibility
        if (compressed.size() >= 5 && compressed.substr(0, 5) == "RLE1:") {
            // Decompress using legacy RLE (for backwards compatibility)
            return DecompressLegacyRLE(compressed);
        }
        // Not compressed or unknown format
        return compressed;
    }

    // Check for uncompressed data
    if (compressed.substr(0, 10) == "ZLIB:NONE:") {
        return compressed.substr(10);
    }

    // Parse header to get original size
    size_t header_end = compressed.find(':', 6);
    if (header_end == std::string::npos) {
        return compressed;  // Invalid format
    }

    std::string size_str = compressed.substr(6, header_end - 6);
    uLongf original_size = std::stoull(size_str);

    // Extract compressed data
    std::string compressed_data = compressed.substr(header_end + 1);

    // Decompress
    std::vector<Bytef> decompressed_buffer(original_size);
    uLongf decompressed_size = original_size;

    int result = uncompress(
        decompressed_buffer.data(),
        &decompressed_size,
        reinterpret_cast<const Bytef*>(compressed_data.c_str()),
        static_cast<uLong>(compressed_data.size())
    );

    if (result != Z_OK || decompressed_size != original_size) {
        // Decompression failed, return as-is
        return compressed;
    }

    return std::string(
        reinterpret_cast<const char*>(decompressed_buffer.data()),
        decompressed_size
    );
}

// Legacy RLE decompression for backwards compatibility
static std::string DecompressLegacyRLE(const std::string& compressed) {
    if (compressed.size() < 5 || compressed.substr(0, 5) != "RLE1:") {
        return compressed;
    }

    std::string data = compressed.substr(5);
    std::string decompressed;
    decompressed.reserve(data.size() * 2);

    size_t i = 0;
    while (i < data.size()) {
        if (data[i] == '\xFF' && i + 2 < data.size()) {
            unsigned char count = static_cast<unsigned char>(data[i + 1]);
            char value = data[i + 2];
            decompressed.append(count, value);
            i += 3;
        } else {
            decompressed += data[i];
            i++;
        }
    }

    return decompressed;
}

bool ShouldCompress(const std::string& data) {
    return data.size() > COMPRESSION_THRESHOLD;
}

// =============================================================================
// EntityID Serialization
// =============================================================================

Json::Value SerializeEntityID(const core::ecs::EntityID& entity_id) {
    Json::Value data;

    // Serialize both id and version for version safety
    data["id"] = Json::UInt64(entity_id.id);
    data["version"] = entity_id.version;

    return data;
}

core::ecs::EntityID DeserializeEntityID(const Json::Value& data) {
    core::ecs::EntityID entity_id;

    // Handle versioned format: {"id": X, "version": Y}
    if (data.isObject() && data.isMember("id")) {
        entity_id.id = data["id"].asUInt64();
        entity_id.version = data.isMember("version") ? data["version"].asUInt() : 1;
    }
    // Handle legacy format: just a number
    else if (data.isUInt64()) {
        entity_id.id = data.asUInt64();
        entity_id.version = 1;  // Default version for legacy saves
    }
    else if (data.isUInt()) {
        entity_id.id = data.asUInt();
        entity_id.version = 1;  // Default version for legacy saves
    }
    // Invalid format
    else {
        entity_id.id = 0;
        entity_id.version = 0;
    }

    return entity_id;
}

Json::Value SerializeLegacyEntityID(uint32_t legacy_id) {
    // Simple passthrough for backwards compatibility
    return Json::Value(legacy_id);
}

uint32_t DeserializeLegacyEntityID(const Json::Value& data) {
    if (data.isUInt()) {
        return data.asUInt();
    }
    return 0;  // Invalid entity
}

// =============================================================================
// Streaming Serialization
// =============================================================================

StreamWriter::StreamWriter(const std::string& filepath)
    : filepath_(filepath)
    , file_handle_(nullptr)
    , chunk_count_(0)
    , finalized_(false)
{
    FILE* file = std::fopen(filepath.c_str(), "wb");
    file_handle_ = static_cast<void*>(file);
}

StreamWriter::~StreamWriter() {
    if (!finalized_) {
        Finalize();
    }
    if (file_handle_) {
        std::fclose(static_cast<FILE*>(file_handle_));
    }
}

bool StreamWriter::WriteHeader(int version) {
    if (!file_handle_) return false;

    FILE* file = static_cast<FILE*>(file_handle_);

    // Write magic number "GSAV" (Game Save)
    const char magic[4] = {'G', 'S', 'A', 'V'};
    if (std::fwrite(magic, 1, 4, file) != 4) return false;

    // Write version
    if (std::fwrite(&version, sizeof(int), 1, file) != 1) return false;

    // Write placeholder for chunk count (will update in Finalize)
    if (std::fwrite(&chunk_count_, sizeof(uint32_t), 1, file) != 1) return false;

    return true;
}

bool StreamWriter::WriteChunk(const std::string& component_name, const std::string& data) {
    if (!file_handle_) return false;

    FILE* file = static_cast<FILE*>(file_handle_);

    // Write component name length and name
    uint32_t name_len = static_cast<uint32_t>(component_name.size());
    if (std::fwrite(&name_len, sizeof(uint32_t), 1, file) != 1) return false;
    if (std::fwrite(component_name.c_str(), 1, name_len, file) != name_len) return false;

    // Write data length and data
    uint32_t data_len = static_cast<uint32_t>(data.size());
    if (std::fwrite(&data_len, sizeof(uint32_t), 1, file) != 1) return false;
    if (std::fwrite(data.c_str(), 1, data_len, file) != data_len) return false;

    chunk_count_++;
    return true;
}

bool StreamWriter::Finalize() {
    if (!file_handle_ || finalized_) return false;

    FILE* file = static_cast<FILE*>(file_handle_);

    // Seek back to chunk count position and update it
    if (std::fseek(file, 4 + sizeof(int), SEEK_SET) != 0) return false;
    if (std::fwrite(&chunk_count_, sizeof(uint32_t), 1, file) != 1) return false;

    finalized_ = true;
    std::fflush(file);
    return true;
}

// =============================================================================
// Stream Reader
// =============================================================================

StreamReader::StreamReader(const std::string& filepath)
    : filepath_(filepath)
    , file_handle_(nullptr)
    , chunks_remaining_(0)
    , header_read_(false)
{
    FILE* file = std::fopen(filepath.c_str(), "rb");
    file_handle_ = static_cast<void*>(file);
}

StreamReader::~StreamReader() {
    if (file_handle_) {
        std::fclose(static_cast<FILE*>(file_handle_));
    }
}

bool StreamReader::ReadHeader(int& out_version) {
    if (!file_handle_ || header_read_) return false;

    FILE* file = static_cast<FILE*>(file_handle_);

    // Read and verify magic number
    char magic[4];
    if (std::fread(magic, 1, 4, file) != 4) return false;
    if (std::memcmp(magic, "GSAV", 4) != 0) return false;

    // Read version
    if (std::fread(&out_version, sizeof(int), 1, file) != 1) return false;

    // Read chunk count
    if (std::fread(&chunks_remaining_, sizeof(uint32_t), 1, file) != 1) return false;

    header_read_ = true;
    return true;
}

bool StreamReader::ReadNextChunk(std::string& out_component_name, std::string& out_data) {
    if (!file_handle_ || !header_read_ || chunks_remaining_ == 0) return false;

    FILE* file = static_cast<FILE*>(file_handle_);

    // Read component name
    uint32_t name_len;
    if (std::fread(&name_len, sizeof(uint32_t), 1, file) != 1) return false;

    std::vector<char> name_buffer(name_len);
    if (std::fread(name_buffer.data(), 1, name_len, file) != name_len) return false;
    out_component_name = std::string(name_buffer.begin(), name_buffer.end());

    // Read data
    uint32_t data_len;
    if (std::fread(&data_len, sizeof(uint32_t), 1, file) != 1) return false;

    std::vector<char> data_buffer(data_len);
    if (std::fread(data_buffer.data(), 1, data_len, file) != data_len) return false;
    out_data = std::string(data_buffer.begin(), data_buffer.end());

    chunks_remaining_--;
    return true;
}

bool StreamReader::HasMoreChunks() const {
    return header_read_ && chunks_remaining_ > 0;
}

} // namespace game::core::serialization
