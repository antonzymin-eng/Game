#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "core/logging/Logger.h"

namespace {

class StreamCapture {
public:
    explicit StreamCapture(std::ostream& stream)
        : stream_(stream), original_buf_(stream.rdbuf()), capture_stream_() {
        stream_.rdbuf(capture_stream_.rdbuf());
    }

    ~StreamCapture() {
        stream_.rdbuf(original_buf_);
    }

    std::string str() const { return capture_stream_.str(); }

private:
    std::ostream& stream_;
    std::streambuf* original_buf_;
    std::ostringstream capture_stream_;
};

}  // namespace

class LoggingMacrosTest : public ::testing::Test {
protected:
    void SetUp() override {
        core::logging::DisableFileSink();
        core::logging::SetGlobalLogLevel(core::logging::LogLevel::Trace);
    }

    void TearDown() override {
        core::logging::Flush();
        core::logging::DisableFileSink();
    }
};

TEST_F(LoggingMacrosTest, InfoWritesToStdout) {
    StreamCapture cout_capture(std::cout);
    StreamCapture cerr_capture(std::cerr);

    CORE_LOG_INFO("TestSystem", "Hello, logging!");
    core::logging::Flush();

    const std::string stdout_output = cout_capture.str();
    EXPECT_NE(stdout_output.find("[INFO][TestSystem] Hello, logging!"), std::string::npos);
    EXPECT_TRUE(cerr_capture.str().empty());
}

TEST_F(LoggingMacrosTest, DebugRespectsGlobalLevel) {
    core::logging::SetGlobalLogLevel(core::logging::LogLevel::Info);

    StreamCapture cout_capture(std::cout);
    CORE_LOG_DEBUG("TestSystem", "Should not appear");
    core::logging::Flush();

    EXPECT_TRUE(cout_capture.str().empty());
}

TEST_F(LoggingMacrosTest, StreamMacroFormatsExpression) {
    StreamCapture cout_capture(std::cout);

    CORE_LOGF_WARN("TestSystem", "value=" << 42 << " ready");
    core::logging::Flush();

    const std::string stdout_output = cout_capture.str();
    EXPECT_NE(stdout_output.find("[WARN][TestSystem] value=42 ready"), std::string::npos);
}

TEST_F(LoggingMacrosTest, FileSinkRotatesLogs) {
    namespace fs = std::filesystem;

    const fs::path temp_root = fs::temp_directory_path() / fs::unique_path("logging-macros-%%%%-%%%%");
    fs::create_directories(temp_root);
    const fs::path log_path = temp_root / "rotation.log";

    core::logging::FileSinkOptions options;
    options.path = log_path.string();
    options.max_file_size_bytes = 256;
    options.max_files = 1;
    options.flush_on_write = true;

    std::string error_message;
    ASSERT_TRUE(core::logging::EnableFileSink(options, &error_message)) << error_message;

    for (int i = 0; i < 32; ++i) {
        CORE_LOG_INFO("Rotation", std::string("Log line ") + std::to_string(i) + " payload data for rotation testing");
    }

    core::logging::Flush();
    core::logging::DisableFileSink();

    const fs::path rotated = fs::path(log_path).concat(".1");
    EXPECT_TRUE(fs::exists(log_path));
    EXPECT_TRUE(fs::exists(rotated));

    const auto base_size = fs::file_size(log_path);
    const auto rotated_size = fs::file_size(rotated);
    EXPECT_GT(rotated_size, 0u);
    EXPECT_GT(rotated_size, base_size);

    fs::remove_all(temp_root);
}
