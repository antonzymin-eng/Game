#pragma once

#include "core/threading/ThreadingTypes.h"
#include <string>

namespace game::config::helpers {

    // Configuration file generation
    void GenerateDefaultConfigurations();
    bool CreateDefaultGameConfig(const std::string& config_directory);
    
    // Threading strategy queries
    ::core::threading::ThreadingStrategy GetThreadingStrategyForSystem(const std::string& system_name);
    std::string GetThreadingRationale(const std::string& system_name);
    
    // Utility functions
    bool EnsureConfigDirectoryExists(const std::string& config_directory);
    std::string GetDefaultConfigContent();

} // namespace game::config::helpers