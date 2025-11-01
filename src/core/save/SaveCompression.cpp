// Created: January 16, 2025 - 14:45:00 (FIXED: January 17, 2025)
// Location: src/core/save/SaveCompression.cpp
// Mechanica Imperii - Save Compression Implementation (C++17 Compliant)
// FIXES: Bug #2 - LZ4 decompressor undefined variable
// FIXES: Bug #4 - Missing <array> include
// FIXES: Bug #5 - Incomplete memcpy statement

#include "core/save/SaveCompression.h"
#include "core/save/SaveManager.h"
#include <lz4.h>
#include <lz4hc.h>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <array>  // FIX Bug #4: Added missing include

namespace core::save {

namespace endian {
    inline bool IsLittleEndian() {
        uint32_t test = 0x01020304;
        return *reinterpret_cast<uint8_t*>(&test) == 0x04;
    }
    
    inline uint16_t Swap16(uint16_t val) {
        return ((val & 0x00FF) << 8) | ((val & 0xFF00) >> 8);
    }
    
    inline uint32_t Swap32(uint32_t val) {
        return ((val & 0x000000FF) << 24) |
               ((val & 0x0000FF00) << 8)  |
               ((val & 0x00FF0000) >> 8)  |
               ((val & 0xFF000000) >> 24);
    }
    
    inline uint64_t Swap64(uint64_t val) {
        return ((val & 0x00000000000000FFULL) << 56) |
               ((val & 0x000000000000FF00ULL) << 40) |
               ((val & 0x0000000000FF0000ULL) << 24) |
               ((val & 0x00000000FF000000ULL) << 8)  |
               ((val & 0x000000FF00000000ULL) >> 8)  |
               ((val & 0x0000FF0000000000ULL) >> 24) |
               ((val & 0x00FF000000000000ULL) >> 40) |
               ((val & 0xFF00000000000000ULL) >> 56);
    }
    
    inline uint16_t ToLE16(uint16_t val) { return IsLittleEndian() ? val : Swap16(val); }
    inline uint32_t ToLE32(uint32_t val) { return IsLittleEndian() ? val : Swap32(val); }
    inline uint64_t ToLE64(uint64_t val) { return IsLittleEndian() ? val : Swap64(val); }
    inline uint16_t FromLE16(uint16_t val) { return IsLittleEndian() ? val : Swap16(val); }
    inline uint32_t FromLE32(uint32_t val) { return IsLittleEndian() ? val : Swap32(val); }
    inline uint64_t FromLE64(uint64_t val) { return IsLittleEndian() ? val : Swap64(val); }
}

void CompressedData::Header::ToLittleEndian() {
    magic = endian::ToLE32(magic);
    version = endian::ToLE16(version);
    algorithm_id = endian::ToLE16(algorithm_id);
    original_size = endian::ToLE64(original_size);
    compressed_size = endian::ToLE64(compressed_size);
    checksum = endian::ToLE32(checksum);
    reserved = endian::ToLE32(reserved);
}

void CompressedData::Header::FromLittleEndian() {
    magic = endian::FromLE32(magic);
    version = endian::FromLE16(version);
    algorithm_id = endian::FromLE16(algorithm_id);
    original_size = endian::FromLE64(original_size);
    compressed_size = endian::FromLE64(compressed_size);
    checksum = endian::FromLE32(checksum);
    reserved = endian::FromLE32(reserved);
}

// ============================================================================
// CompressionAlgorithm String Conversion
// ============================================================================

std::string ToString(CompressionAlgorithm algo) {
    switch (algo) {
        case CompressionAlgorithm::NONE: return "None";
        case CompressionAlgorithm::LZ4: return "LZ4";
        case CompressionAlgorithm::LZ4HC: return "LZ4HC";
        case CompressionAlgorithm::ZLIB: return "ZLIB";
        case CompressionAlgorithm::ZSTD: return "ZSTD";
        default: return "Unknown";
    }
}

Expected<CompressionAlgorithm> FromString(const std::string& name) {
    if (name == "None" || name == "none") return CompressionAlgorithm::NONE;
    if (name == "LZ4" || name == "lz4") return CompressionAlgorithm::LZ4;
    if (name == "LZ4HC" || name == "lz4hc") return CompressionAlgorithm::LZ4HC;
    if (name == "ZLIB" || name == "zlib") return CompressionAlgorithm::ZLIB;
    if (name == "ZSTD" || name == "zstd") return CompressionAlgorithm::ZSTD;
    return SaveError::VALIDATION_FAILED;
}

// ============================================================================
// CompressionLevel Presets
// ============================================================================

CompressionLevel CompressionLevel::Fast() {
    CompressionLevel level;
    level.level = 1;
    level.favor_speed = true;
    level.min_size_threshold = 512;
    return level;
}

CompressionLevel CompressionLevel::Balanced() {
    CompressionLevel level;
    level.level = 5;
    level.favor_speed = true;
    level.min_size_threshold = 1024;
    return level;
}

CompressionLevel CompressionLevel::BestSize() {
    CompressionLevel level;
    level.level = 9;
    level.favor_speed = false;
    level.min_size_threshold = 2048;
    return level;
}

CompressionLevel CompressionLevel::Default() {
    return Balanced();
}

// ============================================================================
// CompressionStats Implementation
// ============================================================================

void CompressionStats::Calculate() {
    if (uncompressed_size > 0) {
        compression_ratio = static_cast<double>(compressed_size) / uncompressed_size;
        space_saved_percent = (1.0 - compression_ratio) * 100.0;
    } else {
        compression_ratio = 0.0;
        space_saved_percent = 0.0;
    }
}

std::string CompressionStats::GenerateReport() const {
    std::ostringstream ss;
    ss << "Compression Report:\n";
    ss << "  Algorithm: " << ToString(algorithm) << "\n";
    ss << "  Original Size: " << uncompressed_size << " bytes\n";
    ss << "  Compressed Size: " << compressed_size << " bytes\n";
    ss << "  Compression Ratio: " << std::fixed << std::setprecision(2) << compression_ratio << "\n";
    ss << "  Space Saved: " << std::fixed << std::setprecision(1) << space_saved_percent << "%\n";
    
    if (compression_time.count() > 0) {
        ss << "  Compression Time: " << compression_time.count() << "ms\n";
        double throughput = (uncompressed_size / 1024.0 / 1024.0) / (compression_time.count() / 1000.0);
        ss << "  Throughput: " << std::fixed << std::setprecision(2) << throughput << " MB/s\n";
    }
    
    if (decompression_time.count() > 0) {
        ss << "  Decompression Time: " << decompression_time.count() << "ms\n";
    }
    
    return ss.str();
}

// ============================================================================
// ICompressor String Convenience Methods
// ============================================================================

Expected<std::vector<uint8_t>> ICompressor::CompressString(
    const std::string& data,
    const CompressionLevel& level) {
    return Compress(reinterpret_cast<const uint8_t*>(data.data()), data.size(), level);
}

Expected<std::string> ICompressor::DecompressString(
    const uint8_t* data,
    size_t compressed_size,
    size_t expected_uncompressed_size) {
    
    auto result = Decompress(data, compressed_size, expected_uncompressed_size);
    if (!result.has_value()) {
        return result.error();
    }
    
    const auto& decompressed = *result;
    return std::string(reinterpret_cast<const char*>(decompressed.data()), decompressed.size());
}

// ============================================================================
// LZ4Compressor Implementation
// ============================================================================

LZ4Compressor::LZ4Compressor() {
    m_last_stats = CompressionStats{};
}

std::string LZ4Compressor::GetVersion() const {
    return std::to_string(LZ4_versionNumber());
}

bool LZ4Compressor::IsAvailable() const {
    return true;  // LZ4 is always available if linked
}

Expected<std::vector<uint8_t>> LZ4Compressor::Compress(
    const uint8_t* data,
    size_t size,
    const CompressionLevel& level) {
    
    if (!data || size == 0) {
        return SaveError::SERIALIZATION_FAILED;
    }
    
    auto start_time = std::chrono::steady_clock::now();

    // Choose compression method based on level
    Expected<std::vector<uint8_t>> result = (level.favor_speed || level.level < 3)
        ? CompressFast(data, size)
        : CompressHC(data, size, level.level);
    
    if (result.has_value()) {
        m_last_stats.uncompressed_size = size;
        m_last_stats.compressed_size = result->size();
        m_last_stats.algorithm = CompressionAlgorithm::LZ4;
        m_last_stats.compression_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time);
        m_last_stats.Calculate();
    }
    
    return result;
}

Expected<std::vector<uint8_t>> LZ4Compressor::CompressFast(const uint8_t* data, size_t size) {
    // Calculate maximum compressed size
    int max_compressed_size = LZ4_compressBound(static_cast<int>(size));
    if (max_compressed_size <= 0) {
        return SaveError::SERIALIZATION_FAILED;
    }
    
    std::vector<uint8_t> compressed(max_compressed_size);
    
    int compressed_size = LZ4_compress_default(
        reinterpret_cast<const char*>(data),
        reinterpret_cast<char*>(compressed.data()),
        static_cast<int>(size),
        max_compressed_size);
    
    if (compressed_size <= 0) {
        return SaveError::SERIALIZATION_FAILED;
    }
    
    compressed.resize(compressed_size);
    return compressed;
}

Expected<std::vector<uint8_t>> LZ4Compressor::CompressHC(
    const uint8_t* data,
    size_t size,
    int level) {
    
    // Clamp level to valid range (1-12 for LZ4HC)
    level = std::clamp(level, 1, LZ4HC_CLEVEL_MAX);
    
    int max_compressed_size = LZ4_compressBound(static_cast<int>(size));
    if (max_compressed_size <= 0) {
        return SaveError::SERIALIZATION_FAILED;
    }
    
    std::vector<uint8_t> compressed(max_compressed_size);
    
    int compressed_size = LZ4_compress_HC(
        reinterpret_cast<const char*>(data),
        reinterpret_cast<char*>(compressed.data()),
        static_cast<int>(size),
        max_compressed_size,
        level);
    
    if (compressed_size <= 0) {
        return SaveError::SERIALIZATION_FAILED;
    }
    
    compressed.resize(compressed_size);
    return compressed;
}

Expected<std::vector<uint8_t>> LZ4Compressor::Decompress(
    const uint8_t* data,
    size_t compressed_size,
    size_t expected_uncompressed_size) {
    
    if (!data || compressed_size == 0) {
        return SaveError::SERIALIZATION_FAILED;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    // FIX Bug #2: CRITICAL - Must have expected size, use correct variable name
    if (expected_uncompressed_size == 0) {
        return SaveError::CORRUPTION_DETECTED;
    }
    
    // FIX Bug #2: Use expected_uncompressed_size (not undefined 'output_size')
    std::vector<uint8_t> decompressed(expected_uncompressed_size);
    
    int decompressed_size = LZ4_decompress_safe(
        reinterpret_cast<const char*>(data),
        reinterpret_cast<char*>(decompressed.data()),
        static_cast<int>(compressed_size),
        static_cast<int>(expected_uncompressed_size));
    
    if (decompressed_size < 0) {
        return SaveError::CORRUPTION_DETECTED;
    }
    
    if (static_cast<size_t>(decompressed_size) != expected_uncompressed_size) {
        return SaveError::CORRUPTION_DETECTED;
    }
    
    decompressed.resize(decompressed_size);
    
    m_last_stats.compressed_size = compressed_size;
    m_last_stats.uncompressed_size = decompressed_size;
    m_last_stats.algorithm = CompressionAlgorithm::LZ4;
    m_last_stats.decompression_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);
    m_last_stats.Calculate();
    
    return decompressed;
}

// ============================================================================
// NullCompressor Implementation (Passthrough)
// ============================================================================

Expected<std::vector<uint8_t>> NullCompressor::Compress(
    const uint8_t* data,
    size_t size,
    const CompressionLevel& level) {
    
    if (!data || size == 0) {
        return SaveError::SERIALIZATION_FAILED;
    }
    
    std::vector<uint8_t> result(data, data + size);
    
    m_last_stats.uncompressed_size = size;
    m_last_stats.compressed_size = size;
    m_last_stats.algorithm = CompressionAlgorithm::NONE;
    m_last_stats.compression_ratio = 1.0;
    m_last_stats.space_saved_percent = 0.0;
    
    return result;
}

Expected<std::vector<uint8_t>> NullCompressor::Decompress(
    const uint8_t* data,
    size_t compressed_size,
    size_t expected_uncompressed_size) {
    
    if (!data || compressed_size == 0) {
        return SaveError::SERIALIZATION_FAILED;
    }
    
    std::vector<uint8_t> result(data, data + compressed_size);
    
    m_last_stats.compressed_size = compressed_size;
    m_last_stats.uncompressed_size = compressed_size;
    m_last_stats.algorithm = CompressionAlgorithm::NONE;
    m_last_stats.compression_ratio = 1.0;
    m_last_stats.space_saved_percent = 0.0;
    
    return result;
}

// ============================================================================
// CompressionFactory Implementation
// ============================================================================

CompressionFactory& CompressionFactory::Instance() {
    static CompressionFactory instance;
    return instance;
}

std::unique_ptr<ICompressor> CompressionFactory::CreateCompressor(CompressionAlgorithm algo) {
    switch (algo) {
        case CompressionAlgorithm::NONE:
            return std::make_unique<NullCompressor>();
        
        case CompressionAlgorithm::LZ4:
        case CompressionAlgorithm::LZ4HC:
            return std::make_unique<LZ4Compressor>();
        
        case CompressionAlgorithm::ZLIB:
        case CompressionAlgorithm::ZSTD:
            // Fall back to LZ4 if not implemented
            return std::make_unique<LZ4Compressor>();
        
        default:
            return std::make_unique<NullCompressor>();
    }
}

std::unique_ptr<ICompressor> CompressionFactory::CreateDefaultCompressor() {
    if (IsAlgorithmAvailable(CompressionAlgorithm::LZ4)) {
        return CreateCompressor(CompressionAlgorithm::LZ4);
    }
    return CreateCompressor(CompressionAlgorithm::NONE);
}

bool CompressionFactory::IsAlgorithmAvailable(CompressionAlgorithm algo) const {
    switch (algo) {
        case CompressionAlgorithm::NONE:
        case CompressionAlgorithm::LZ4:
        case CompressionAlgorithm::LZ4HC:
            return true;
        
        case CompressionAlgorithm::ZLIB:
        case CompressionAlgorithm::ZSTD:
            return false;  // Not yet implemented
        
        default:
            return false;
    }
}

std::vector<CompressionAlgorithm> CompressionFactory::GetAvailableAlgorithms() const {
    std::vector<CompressionAlgorithm> available;
    available.push_back(CompressionAlgorithm::NONE);
    available.push_back(CompressionAlgorithm::LZ4);
    available.push_back(CompressionAlgorithm::LZ4HC);
    return available;
}

CompressionAlgorithm CompressionFactory::RecommendForSize(size_t data_size) const {
    if (data_size < 1024) {
        return CompressionAlgorithm::NONE;  // Too small to benefit
    } else if (data_size < 100 * 1024) {
        return CompressionAlgorithm::LZ4;   // Fast for small files
    } else if (data_size < 10 * 1024 * 1024) {
        return CompressionAlgorithm::LZ4HC; // Better ratio for medium files
    } else {
        return CompressionAlgorithm::LZ4;   // Speed matters for large files
    }
}

CompressionAlgorithm CompressionFactory::RecommendForSpeed() const {
    return CompressionAlgorithm::LZ4;
}

// ============================================================================
// CompressedData Implementation
// ============================================================================

std::vector<uint8_t> CompressedData::Serialize() const {
    Header header;
    header.algorithm_id = static_cast<uint16_t>(algorithm);
    header.original_size = original_size;
    header.compressed_size = compressed_size;
    header.checksum = checksum;
    
    // Convert to little-endian for on-disk format
    header.ToLittleEndian();
    
    std::vector<uint8_t> result(sizeof(Header) + data.size());
    std::memcpy(result.data(), &header, sizeof(Header));
    std::memcpy(result.data() + sizeof(Header), data.data(), data.size());
    
    return result;
}

Expected<CompressedData> CompressedData::Deserialize(const uint8_t* data, size_t size) {
    if (!data || size < sizeof(Header)) {
        return SaveError::CORRUPTION_DETECTED;
    }
    
    Header header;
    std::memcpy(&header, data, sizeof(Header));
    
    // Convert from little-endian
    header.FromLittleEndian();

    // Validate magic number
    if (header.magic != 0x4D435053) {
        return SaveError::CORRUPTION_DETECTED;
    }
    
    // Validate version
    if (header.version > 1) {
        return SaveError::VERSION_ERROR;
    }
    
    CompressedData result;
    result.algorithm = static_cast<CompressionAlgorithm>(header.algorithm_id);
    result.original_size = header.original_size;
    result.compressed_size = header.compressed_size;
    result.checksum = header.checksum;
    
    size_t payload_size = size - sizeof(Header);
    if (payload_size != header.compressed_size) {
        return SaveError::CORRUPTION_DETECTED;
    }
    
    result.data.resize(payload_size);
    // FIX Bug #5: Complete memcpy statement properly
    std::memcpy(result.data.data(), data + sizeof(Header), payload_size);
    
    return result;
}

bool CompressedData::ValidateHeader() const {
    if (data.size() != compressed_size) {
        return false;
    }
    
    if (original_size == 0 || compressed_size == 0) {
        return false;
    }
    
    return true;
}

bool CompressedData::ValidateChecksum(const uint8_t* original_data, size_t size) const {
    if (size != original_size) {
        return false;
    }
    uint32_t calculated = compression_utils::CalculateCRC32(original_data, size);
    return calculated == checksum;
}

// ============================================================================
// CompressionManager Implementation
// ============================================================================

CompressionManager::CompressionManager(const Config& config)
    : m_config(config) {
    InitializeCompressor();
}

CompressionManager::~CompressionManager() = default;

void CompressionManager::InitializeCompressor() {
    auto& factory = CompressionFactory::Instance();
    
    if (!m_config.enable_compression) {
        m_compressor = factory.CreateCompressor(CompressionAlgorithm::NONE);
    } else if (factory.IsAlgorithmAvailable(m_config.algorithm)) {
        m_compressor = factory.CreateCompressor(m_config.algorithm);
    } else {
        // Fallback to default if requested algorithm unavailable
        m_compressor = factory.CreateDefaultCompressor();
    }
}

Expected<CompressedData> CompressionManager::CompressData(const uint8_t* data, size_t size) {
    if (!data || size == 0) {
        return SaveError::SERIALIZATION_FAILED;
    }
    
    // Skip compression for small files
    if (size < m_config.min_size_threshold || !m_config.enable_compression) {
        CompressedData result;
        result.data.assign(data, data + size);
        result.original_size = size;
        result.compressed_size = size;
        result.algorithm = CompressionAlgorithm::NONE;
        result.checksum = CalculateChecksum(data, size);
        
        m_stats.total_compressions++;
        m_stats.bytes_compressed += size;
        m_stats.total_compressed_size += size;
        
        return result;
    }
    
    // Perform compression
    auto compressed_result = m_compressor->Compress(data, size, m_config.level);
    if (!compressed_result.has_value()) {
        return compressed_result.error();
    }
    
    auto& compressed_data = *compressed_result;
    
    // Check if compression actually helped
    if (compressed_data.size() >= size * 0.95) {
        // Less than 5% savings - not worth it
        CompressedData result;
        result.data.assign(data, data + size);
        result.original_size = size;
        result.compressed_size = size;
        result.algorithm = CompressionAlgorithm::NONE;
        result.checksum = CalculateChecksum(data, size);
        
        m_stats.total_compressions++;
        m_stats.bytes_compressed += size;
        m_stats.total_compressed_size += size;
        
        return result;
    }
    
    // Build result
    CompressedData result;
    result.data = std::move(compressed_data);
    result.original_size = size;
    result.compressed_size = result.data.size();
    result.algorithm = m_compressor->GetAlgorithm();
    result.checksum = CalculateChecksum(data, size);
    
    // Update statistics
    CompressionStats stats = m_compressor->GetLastStats();
    m_stats.Update(stats, true);
    
    return result;
}

Expected<std::vector<uint8_t>> CompressionManager::DecompressData(const CompressedData& compressed) {
    if (!compressed.ValidateHeader()) {
        return SaveError::CORRUPTION_DETECTED;
    }
    
    // Handle uncompressed data
    if (compressed.algorithm == CompressionAlgorithm::NONE) {
        m_stats.total_decompressions++;
        m_stats.bytes_decompressed += compressed.original_size;
        return compressed.data;
    }
    
    // Get appropriate decompressor
    auto decompressor = CompressionFactory::Instance().CreateCompressor(compressed.algorithm);
    if (!decompressor) {
        return SaveError::UNKNOWN_ERROR;
    }
    
    // Decompress
    auto result = decompressor->Decompress(
        compressed.data.data(),
        compressed.compressed_size,
        compressed.original_size);
    
    if (!result.has_value()) {
        return result.error();
    }
    
    // Validate checksum if enabled
    if (m_config.validate_checksums) {
        if (!compressed.ValidateChecksum(result->data(), result->size())) {
            return SaveError::CHECKSUM_MISMATCH;
        }
    }
    
    // Update statistics
    CompressionStats stats = decompressor->GetLastStats();
    m_stats.Update(stats, false);
    
    return result;
}

Expected<CompressedData> CompressionManager::CompressString(const std::string& data) {
    return CompressData(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

Expected<std::string> CompressionManager::DecompressString(const CompressedData& compressed) {
    auto result = DecompressData(compressed);
    if (!result.has_value()) {
        return result.error();
    }
    
    return std::string(reinterpret_cast<const char*>(result->data()), result->size());
}

void CompressionManager::SetAlgorithm(CompressionAlgorithm algo) {
    m_config.algorithm = algo;
    InitializeCompressor();
}

void CompressionManager::SetCompressionLevel(const CompressionLevel& level) {
    m_config.level = level;
}

void CompressionManager::SetEnabled(bool enabled) {
    m_config.enable_compression = enabled;
    InitializeCompressor();
}

CompressionManager::Statistics CompressionManager::GetStatistics() const {
    return m_stats;
}

void CompressionManager::ResetStatistics() {
    m_stats = Statistics{};
}

void CompressionManager::Statistics::Update(const CompressionStats& stats, bool is_compression) {
    if (is_compression) {
        total_compressions++;
        bytes_compressed += stats.uncompressed_size;
        total_compressed_size += stats.compressed_size;
        total_compression_time += stats.compression_time;
    } else {
        total_decompressions++;
        bytes_decompressed += stats.uncompressed_size;
        total_decompression_time += stats.decompression_time;
    }
    
    // Update average compression ratio
    if (bytes_compressed > 0 && total_compressed_size > 0) {
        average_compression_ratio = static_cast<double>(total_compressed_size) / bytes_compressed;
    }
}

std::string CompressionManager::Statistics::GenerateReport() const {
    std::ostringstream ss;
    ss << "Compression Statistics:\n";
    ss << "  Total Compressions: " << total_compressions << "\n";
    ss << "  Total Decompressions: " << total_decompressions << "\n";
    ss << "  Bytes Compressed: " << bytes_compressed << " bytes\n";
    ss << "  Total Compressed Size: " << total_compressed_size << " bytes\n";
    ss << "  Average Ratio: " << std::fixed << std::setprecision(2) << average_compression_ratio << "\n";
    
    if (bytes_compressed > 0) {
        double saved_percent = (1.0 - average_compression_ratio) * 100.0;
        ss << "  Space Saved: " << std::fixed << std::setprecision(1) << saved_percent << "%\n";
    }
    
    if (total_compression_time.count() > 0) {
        ss << "  Total Compression Time: " << total_compression_time.count() << "ms\n";
        if (total_compressions > 0) {
            double avg_time = static_cast<double>(total_compression_time.count()) / total_compressions;
            ss << "  Average Compression Time: " << std::fixed << std::setprecision(2) << avg_time << "ms\n";
        }
    }
    
    if (total_decompression_time.count() > 0) {
        ss << "  Total Decompression Time: " << total_decompression_time.count() << "ms\n";
        if (total_decompressions > 0) {
            double avg_time = static_cast<double>(total_decompression_time.count()) / total_decompressions;
            ss << "  Average Decompression Time: " << std::fixed << std::setprecision(2) << avg_time << "ms\n";
        }
    }
    
    return ss.str();
}

std::vector<CompressionManager::BenchmarkResult> CompressionManager::BenchmarkAlgorithms(
    const uint8_t* test_data,
    size_t size,
    const std::vector<CompressionAlgorithm>& algorithms) const {
    
    std::vector<BenchmarkResult> results;
    
    auto test_algorithms = algorithms.empty() 
        ? CompressionFactory::Instance().GetAvailableAlgorithms()
        : algorithms;
    
    for (auto algo : test_algorithms) {
        auto compressor = CompressionFactory::Instance().CreateCompressor(algo);
        if (!compressor || !compressor->IsAvailable()) {
            continue;
        }
        
        // Test compression
        auto start = std::chrono::steady_clock::now();
        auto compressed = compressor->Compress(test_data, size, m_config.level);
        auto compress_time = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start);
        
        if (!compressed.has_value()) {
            continue;
        }
        
        // Test decompression
        start = std::chrono::steady_clock::now();
        auto decompressed = compressor->Decompress(
            compressed->data(), compressed->size(), size);
        auto decompress_time = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start);
        
        if (!decompressed.has_value()) {
            continue;
        }
        
        // Build result
        BenchmarkResult result;
        result.algorithm = algo;
        result.level = m_config.level;
        result.compression_time_ms = compress_time.count() / 1000.0;
        result.decompression_time_ms = decompress_time.count() / 1000.0;
        result.compression_ratio = static_cast<double>(compressed->size()) / size;
        
        double total_time_sec = (compress_time.count() + decompress_time.count()) / 1000000.0;
        if (total_time_sec > 0) {
            result.throughput_mbps = (size / 1024.0 / 1024.0) / total_time_sec;
        }
        
        results.push_back(result);
    }
    
    return results;
}

uint32_t CompressionManager::CalculateChecksum(const uint8_t* data, size_t size) const {
    return compression_utils::CalculateCRC32(data, size);
}

// ============================================================================
// Compression Utilities Implementation
// ============================================================================

namespace compression_utils {

double EstimateCompressionRatio(const uint8_t* data, size_t size) {
    if (!data || size == 0) {
        return 1.0;
    }
    
    // Sample first 10% of data for estimation
    size_t sample_size = std::min(size, size_t(10 * 1024));  // Max 10KB sample
    
    auto compressor = CompressionFactory::Instance().CreateDefaultCompressor();
    auto compressed = compressor->Compress(data, sample_size, CompressionLevel::Fast());
    
    if (!compressed.has_value()) {
        return 1.0;
    }
    
    return static_cast<double>(compressed->size()) / sample_size;
}

bool IsLikelyCompressed(const uint8_t* data, size_t size) {
    if (!data || size < 100) {
        return false;
    }
    
    // Check entropy - compressed data has high entropy
    double entropy = CalculateEntropy(data, std::min(size, size_t(1024)));
    
    // Compressed data typically has entropy > 7.5 bits/byte
    return entropy > 7.5;
}

double CalculateEntropy(const uint8_t* data, size_t size) {
    if (!data || size == 0) {
        return 0.0;
    }
    
    // Calculate byte frequency (FIX Bug #4: Now has <array> included)
    std::array<size_t, 256> frequency = {0};
    for (size_t i = 0; i < size; ++i) {
        frequency[data[i]]++;
    }
    
    // Calculate Shannon entropy
    double entropy = 0.0;
    for (size_t count : frequency) {
        if (count > 0) {
            double probability = static_cast<double>(count) / size;
            entropy -= probability * std::log2(probability);
        }
    }
    
    return entropy;
}

CompressionAlgorithm RecommendAlgorithm(
    const uint8_t* data,
    size_t size,
    bool favor_speed) {
    
    if (!data || size == 0) {
        return CompressionAlgorithm::NONE;
    }
    
    // Too small to benefit from compression
    if (size < 1024) {
        return CompressionAlgorithm::NONE;
    }
    
    // Check if already compressed
    if (IsLikelyCompressed(data, size)) {
        return CompressionAlgorithm::NONE;
    }
    
    // Estimate compression potential
    double entropy = CalculateEntropy(data, std::min(size, size_t(1024)));
    
    // Low entropy = high compression potential
    if (entropy < 4.0) {
        // Very compressible - use high compression
        return favor_speed ? CompressionAlgorithm::LZ4 : CompressionAlgorithm::LZ4HC;
    } else if (entropy < 6.0) {
        // Moderately compressible
        return CompressionAlgorithm::LZ4;
    } else {
        // High entropy - limited compression potential
        return favor_speed ? CompressionAlgorithm::LZ4 : CompressionAlgorithm::NONE;
    }
}

uint32_t CalculateCRC32(const uint8_t* data, size_t size) {
    static const uint32_t CRC32_POLY = 0xEDB88320;
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ (CRC32_POLY & -(crc & 1));
        }
    }
    
    return crc ^ 0xFFFFFFFF;
}

} // namespace compression_utils

} // namespace core::save