// ============================================================================
// Enhanced GameConfig Test Program
// Location: apps/test_enhanced_config.cpp
// ============================================================================

#include "game/config/GameConfig.h"
#include <iostream>
#include <unordered_map>

using namespace game::config;

int main() {
    std::cout << "=== Enhanced GameConfig Test ===" << std::endl;
    
    try {
        // Test configuration loading
        std::cout << "\n1. Testing Configuration Loading..." << std::endl;
        GameConfig& config = GameConfig::Instance();
        
        if (!config.LoadFromFile("config/GameConfig.json")) {
            std::cout << "⚠️  Config file not found - using defaults" << std::endl;
            config.CreateDefaultConfig();
        } else {
            std::cout << "✅ Configuration loaded successfully!" << std::endl;
        }
        
        // Test path-based value access
        std::cout << "\n2. Testing Path-Based Value Access..." << std::endl;
        
        auto tax_rate = config.GetValue<double>("economics.tax.base_rate", 0.1);
        auto thread_count = config.GetValue<int>("system.threading.thread_pool_size", 2);
        auto enable_threading = config.GetValue<bool>("system.threading.enable_threading", false);
        
        std::cout << "Tax Base Rate: " << tax_rate << std::endl;
        std::cout << "Thread Pool Size: " << thread_count << std::endl;
        std::cout << "Threading Enabled: " << (enable_threading ? "Yes" : "No") << std::endl;
        
        // Test array values
        auto efficiency_range = config.GetValue<std::vector<double>>("economics.trade.base_efficiency_range", {});
        if (!efficiency_range.empty()) {
            std::cout << "Trade Efficiency Range: [" << efficiency_range[0] << ", " << efficiency_range[1] << "]" << std::endl;
        }
        
        // Test section access
        std::cout << "\n3. Testing Section Access..." << std::endl;
        
        auto buildings_section = config.GetSection("buildings");
        std::cout << "Buildings configured: " << buildings_section.size() << std::endl;
        for (const auto& [building_name, building_data] : buildings_section) {
            std::cout << "  - " << building_name << std::endl;
        }
        
        // Test validation
        std::cout << "\n4. Testing Configuration Validation..." << std::endl;
        
        auto validation_result = config.ValidateAllSections();
        if (validation_result.is_valid) {
            std::cout << "✅ All sections are valid!" << std::endl;
        } else {
            std::cout << "❌ Validation failed with " << validation_result.errors.size() << " errors:" << std::endl;
            for (const auto& error : validation_result.errors) {
                std::cout << "  ERROR: " << error << std::endl;
            }
        }
        
        if (!validation_result.warnings.empty()) {
            std::cout << "⚠️  " << validation_result.warnings.size() << " warnings:" << std::endl;
            for (const auto& warning : validation_result.warnings) {
                std::cout << "  WARNING: " << warning << std::endl;
            }
        }
        
        // Test individual section validation
        std::cout << "\n5. Testing Individual Section Validation..." << std::endl;
        
        auto economics_validation = config.ValidateSection("economics");
        std::cout << "Economics section: " << (economics_validation.is_valid ? "✅ Valid" : "❌ Invalid") << std::endl;
        
        auto military_validation = config.ValidateSection("military");
        std::cout << "Military section: " << (military_validation.is_valid ? "✅ Valid" : "❌ Invalid") << std::endl;
        
        // Test formula evaluation
        std::cout << "\n6. Testing Simple Formula Evaluation..." << std::endl;
        
        std::unordered_map<std::string, double> tax_vars = {
            {"base_tax", 100.0},
            {"admin_efficiency", 0.8},
            {"autonomy_penalty", 0.9},
            {"stability", 0.7}
        };
        
        auto tax_income = config.EvaluateFormula("tax_income", tax_vars);
        std::cout << "Calculated tax income: " << tax_income << std::endl;
        
        // Test configuration statistics
        std::cout << "\n7. Testing Configuration Statistics..." << std::endl;
        
        std::cout << "Config size: " << config.GetConfigSize() << " sections" << std::endl;
        
        auto loaded_files = config.GetLoadedFiles();
        std::cout << "Loaded files: " << loaded_files.size() << std::endl;
        for (const auto& file : loaded_files) {
            std::cout << "  - " << file << std::endl;
        }
        
        // Test configuration export
        std::cout << "\n8. Testing Configuration Export..." << std::endl;
        
        if (config.ExportConfig("config/exported_config.json")) {
            std::cout << "✅ Configuration exported successfully!" << std::endl;
        } else {
            std::cout << "❌ Failed to export configuration" << std::endl;
        }
        
        std::cout << "\n=== Enhanced GameConfig Test Complete ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}