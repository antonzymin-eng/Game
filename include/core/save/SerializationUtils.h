// Created: December 5, 2025
// Location: include/core/save/SerializationUtils.h
// Purpose: Utility functions for serialization (CRC32, compression, streaming)

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace game::core::serialization {

// =============================================================================
// CRC32 Checksum
// =============================================================================

/**
 * @brief Calculate CRC32 checksum for data integrity validation
 * @param data Input data string
 * @return 32-bit CRC checksum
 */
uint32_t CalculateCRC32(const std::string& data);

/**
 * @brief Wrap data with CRC32 checksum
 * @param data Serialized JSON data
 * @return JSON string with embedded checksum
 */
std::string WrapWithChecksum(const std::string& data);

/**
 * @brief Unwrap and validate data with CRC32 checksum
 * @param wrapped Wrapped JSON data with checksum
 * @param out_data Output parameter for unwrapped data
 * @return true if checksum is valid, false otherwise
 */
bool UnwrapAndValidate(const std::string& wrapped, std::string& out_data);

// =============================================================================
// Compression
// =============================================================================

/**
 * @brief Compress data using zlib/deflate
 * @param data Uncompressed data
 * @return Compressed data (base64 encoded)
 */
std::string Compress(const std::string& data);

/**
 * @brief Decompress data using zlib/inflate
 * @param compressed Compressed data (base64 encoded)
 * @return Decompressed data
 */
std::string Decompress(const std::string& compressed);

/**
 * @brief Check if compression would be beneficial
 * @param data Data to potentially compress
 * @return true if data is large enough to benefit from compression
 */
bool ShouldCompress(const std::string& data);

// Compression threshold (compress if data > 1KB)
constexpr size_t COMPRESSION_THRESHOLD = 1024;

// =============================================================================
// Streaming Serialization
// =============================================================================

/**
 * @brief Stream writer for large serialization operations
 */
class StreamWriter {
public:
    explicit StreamWriter(const std::string& filepath);
    ~StreamWriter();

    // Write header with version info
    bool WriteHeader(int version);

    // Write a single component chunk
    bool WriteChunk(const std::string& component_name, const std::string& data);

    // Finalize and close stream
    bool Finalize();

private:
    std::string filepath_;
    void* file_handle_;  // FILE* wrapped as void* to avoid including stdio
    uint32_t chunk_count_;
    bool finalized_;
};

/**
 * @brief Stream reader for large deserialization operations
 */
class StreamReader {
public:
    explicit StreamReader(const std::string& filepath);
    ~StreamReader();

    // Read and validate header
    bool ReadHeader(int& out_version);

    // Read next component chunk
    bool ReadNextChunk(std::string& out_component_name, std::string& out_data);

    // Check if more chunks available
    bool HasMoreChunks() const;

private:
    std::string filepath_;
    void* file_handle_;  // FILE* wrapped as void* to avoid including stdio
    uint32_t chunks_remaining_;
    bool header_read_;
};

} // namespace game::core::serialization
