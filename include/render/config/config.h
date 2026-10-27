// Command-line and JSON configuration for render runs

#pragma once

#include "render_config.h"
#include "../io/json_parser.h"
#include <string>

namespace render {

/**
 * @class Config
 * @brief Main configuration manager.
 */
class Config {
public:
    // Load from file
    static RenderConfig load(const std::string& path);
    
    // Parse command-line arguments
    static RenderConfig parse(int argc, char** argv);
    
    // Serialize to JSON string
    static std::string serialize(const RenderConfig& config);
    
    static void printHelp();

private:
    static void parseRenderConfig(const ConfigValue& json, RenderConfig& config);
    
    static std::string integratorName(IntegratorType type);
    
    static std::string cameraName(CameraType type);
};

} // namespace render
