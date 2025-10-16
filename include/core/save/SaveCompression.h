// Created: January 16, 2025 - 14:30:00
// Location: include/core/save/SaveCompression.h
// Mechanica Imperii - Save Compression System (C++17 Compliant)

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <optional>
#include <chrono>

namespace core::save {

// Forward declare Expected
template<typename T> class Expected;
enum class SaveError;

// ============================================================================
// Compression Algorithm Types
// ============================================================================

enum class CompressionAlgorithm {
    NONE = 0,           // No compression (passthrough)
    LZ4 = 1,            // Fast compression/decompression (default)
    LZ4HC = 2,          // High compression ratio, slower
    ZLIB = 3,           // Standard zlib compression
    ZSTD = 4            // Zstandard (best balance)
};

std::string ToString(CompressionAlgorithm algo);
Expected<CompressionAlgorithm> FromString(const std::string& name);

// ============================================================================
// Compression Level Configuration
// ============================================================================

struct CompressionLevel {
    int level = 0;                    // Algorithm-specific level
    bool favor_speed = true;          // Speed vs size tradeoff
    size_t min_size_threshold = 1024; // Don't compress below this size
    
    // Preset configurations
    static CompressionLevel Fast();        // Fastest compression
    static CompressionLevel Balanced();    // Speed/size balance
    static CompressionLevel BestSize();    // Maximum compression
    static CompressionLevel Default();     // Recommended default
    
    CompressionLevel() = default;
    CompressionLevel(int lvl, bool speed = true) : level(lvl), favor_speed(speed) {}
};

// ============================================================================
// Compression Statistics
// ============================================================================

struct CompressionStats {
    size_t uncompressed_size = 0;
    size_t compressed_size = 0;
    double compression_ratio = 0.0;        // compressed / uncompressed
    double space_saved_percent = 0.0;      // (1 - ratio) * 100
    std::chrono::milliseconds compression_time{0};
    std::chrono::milliseconds decompression_time{0};
    CompressionAlgorithm algorithm = CompressionAlgorithm::NONE;
    
    void Calculate();
    std::string GenerateReport() const;
};

// ============================================================================
// Abstract Compression Interface
// ============================================================================

class ICompressor {
public:
    virtual ~ICompressor() = default;
    
    // Compression operations
    virtual Expected<std::vector<uint8_t>> Compress(
        const uint8_t* data, 
        size_t size,
        const CompressionLevel& level = CompressionLevel::Default()) = 0;
    
    virtual Expected<std::vector<uint8_t>> Decompress(
        const uint8_t* data,
        size_t compressed_size,
        size_t expected_uncompressed_size = 0) = 0;
    
    // String convenience wrappers
    Expected<std::vector<uint8_t>> CompressString(
        const std::string& data,
        const CompressionLevel& level = CompressionLevel::Default());
    
    Expected<std::string> DecompressString(
        const uint8_t* data,
        size_t compressed_size,
        size_t expected_uncompressed_size = 0);
    
    // Algorithm information
    virtual CompressionAlgorithm GetAlgorithm() const = 0;
    virtual std::string GetVersion() const = 0;
    virtual bool IsAvailable() const = 0;
    
    // Statistics
    virtual CompressionStats GetLastStats() const = 0;
    virtual void ResetStats() = 0;
};

// ============================================================================
// LZ4 Compressor Implementation
// ============================================================================

class LZ4Compressor : public ICompressor {
public:
    LZ4Compressor();
    ~LZ4Compressor() override = default;
    
    Expected<std::vector<uint8_t>> Compress(
        const uint8_t* data,
        size_t size,
        const CompressionLevel& level = CompressionLevel::Default()) override;
    
    Expected<std::vector<uint8_t>> Decompress(
        const uint8_t* data,
        size_t compressed_size,
        size_t expected_uncompressed_size = 0) override;
    
    CompressionAlgorithm GetAlgorithm() const override { return CompressionAlgorithm::LZ4; }
    std::string GetVersion() const override;
    bool IsAvailable() const override;
    
    CompressionStats GetLastStats() const override { return m_last_stats; }
    void ResetStats() override { m_last_stats = CompressionStats{}; }
    
private:
    CompressionStats m_last_stats;
    
    Expected<std::vector<uint8_t>> CompressFast(const uint8_t* data, size_t size);
    Expected<std::vector<uint8_t>> CompressHC(const uint8_t* data, size_t size, int level);
};

// ============================================================================
// Null Compressor (Passthrough)
// ============================================================================

class NullCompressor : public ICompressor {
public:
    Expected<std::vector<uint8_t>> Compress(
        const uint8_t* data,
        size_t size,
        const CompressionLevel& level = CompressionLevel::Default()) override;
    
    Expected<std::vector<uint8_t>> Decompress(
        const uint8_t* data,
        size_t compressed_size,
        size_t expected_uncompressed_size = 0) override;
    
    CompressionAlgorithm GetAlgorithm() const override { return CompressionAlgorithm::NONE; }
    std::string GetVersion() const override { return "1.0.0"; }
    bool IsAvailable() const override { return true; }
    
    CompressionStats GetLastStats() const override { return m_last_stats; }
    void ResetStats() override { m_last_stats = CompressionStats{}; }
    
private:
    CompressionStats m_last_stats;
};

// ============================================================================
// Compression Factory
// ============================================================================

class CompressionFactory {
public:
    static CompressionFactory& Instance();
    
    // Create compressor for specified algorithm
    std::unique_ptr<ICompressor> CreateCompressor(CompressionAlgorithm algo);
    
    // Get default compressor (LZ4 or fallback to None)
    std::unique_ptr<ICompressor> CreateDefaultCompressor();
    
    // Check algorithm availability
    bool IsAlgorithmAvailable(CompressionAlgorithm algo) const;
    std::vector<CompressionAlgorithm> GetAvailableAlgorithms() const;
    
    // Algorithm recommendations
    CompressionAlgorithm RecommendForSize(size_t data_size) const;
    CompressionAlgorithm RecommendForSpeed() const;
    
private:
    CompressionFactory() = default;
};

// ============================================================================
// Compressed Data Container
// ============================================================================

struct CompressedData {
    std::vector<uint8_t> data;
    size_t original_size = 0;
    size_t compressed_size = 0;
    CompressionAlgorithm algorithm = CompressionAlgorithm::NONE;
    uint32_t checksum = 0;
    
    // Cross-compiler packed header
#if defined(_MSC_VER)
    #pragma pack(push, 1)
#endif
    struct Header {
        uint32_t magic = 0x4D435053;  // "MCSP" (Mechanica Compressed Save)
        uint16_t version = 1;
        uint16_t algorithm_id = 0;
        uint64_t original_size = 0;
        uint64_t compressed_size = 0;
        uint32_t checksum = 0;
        uint32_t reserved = 0;
        
        // Endianness conversion helpers
        void ToLittleEndian();
        void FromLittleEndian();
    }
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((packed))
#endif
    ;
#if defined(_MSC_VER)
    #pragma pack(pop)
#endif
    
    // Serialization for storage
    std::vector<uint8_t> Serialize() const;
    static Expected<CompressedData> Deserialize(const uint8_t* data, size_t size);
    
    bool ValidateHeader() const;
    bool ValidateChecksum(const uint8_t* original_data, size_t original_size) const;
};

// ============================================================================
// High-Level Compression Manager
// ============================================================================

class CompressionManager {
public:
    struct Config {
        CompressionAlgorithm algorithm = CompressionAlgorithm::LZ4;
        CompressionLevel level = CompressionLevel::Default();
        bool enable_compression = true;
        bool validate_checksums = true;
        size_t min_size_threshold = 1024;  // Don't compress files < 1KB
    };
    
    explicit CompressionManager(const Config& config = Config{});
    ~CompressionManager();
    
    // High-level operations
    Expected<CompressedData> CompressData(const uint8_t* data, size_t size);
    Expected<std::vector<uint8_t>> DecompressData(const CompressedData& compressed);
    
    // String convenience methods
    Expected<CompressedData> CompressString(const std::string& data);
    Expected<std::string> DecompressString(const CompressedData& compressed);
    
    // Configuration
    void SetAlgorithm(CompressionAlgorithm algo);
    void SetCompressionLevel(const CompressionLevel& level);
    void SetEnabled(bool enabled);
    
    // Statistics and diagnostics
    struct Statistics {
        size_t total_compressions = 0;
        size_t total_decompressions = 0;
        size_t bytes_compressed = 0;
        size_t bytes_decompressed = 0;
        size_t total_compressed_size = 0;
        double average_compression_ratio = 0.0;
        std::chrono::milliseconds total_compression_time{0};
        std::chrono::milliseconds total_decompression_time{0};
        
        void Update(const CompressionStats& stats, bool is_compression);
        std::string GenerateReport() const;
    };
    
    Statistics GetStatistics() const;
    void ResetStatistics();
    
    // Benchmark utilities
    struct BenchmarkResult {
        CompressionAlgorithm algorithm;
        CompressionLevel level;
        double compression_time_ms = 0.0;
        double decompression_time_ms = 0.0;
        double compression_ratio = 0.0;
        double throughput_mbps = 0.0;
    };
    
    std::vector<BenchmarkResult> BenchmarkAlgorithms(
        const uint8_t* test_data,
        size_t size,
        const std::vector<CompressionAlgorithm>& algorithms = {}) const;
    
private:
    Config m_config;
    std::unique_ptr<ICompressor> m_compressor;
    mutable Statistics m_stats;
    
    void InitializeCompressor();
    uint32_t CalculateChecksum(const uint8_t* data, size_t size) const;
};

// ============================================================================
// Compression Utilities
// ============================================================================

namespace compression_utils {
    // Estimate compression ratio for data (sampling-based)
    double EstimateCompressionRatio(const uint8_t* data, size_t size);
    
    // Check if data is likely already compressed
    bool IsLikelyCompressed(const uint8_t* data, size_t size);
    
    // Entropy calculation for compression potential
    double CalculateEntropy(const uint8_t* data, size_t size);
    
    // Recommend algorithm based on data characteristics
    CompressionAlgorithm RecommendAlgorithm(
        const uint8_t* data, 
        size_t size,
        bool favor_speed = true);

    // Unified CRC32 implementation
    uint32_t CalculateCRC32(const uint8_t* data, size_t size);
	
}

} // namespace core::save

