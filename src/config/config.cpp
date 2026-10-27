#include "render/config/config.h"
#include "render/io/json_parser.h"
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace render {

RenderConfig Config::load(const std::string& path) {
    RenderConfig config;
    auto json = ConfigParser::parseFile(path);
    
    if (json.isNull()) {
        std::cerr << "Warning: Could not load config from " << path << std::endl;
        return config;
    }
    
    parseRenderConfig(json, config);
    return config;
}

RenderConfig Config::parse(int argc, char** argv) {
    RenderConfig config;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                config = load(argv[++i]);
            }
        } else if (arg == "-w" || arg == "--width") {
            if (i + 1 < argc) config.image.width = std::stoi(argv[++i]);
        } else if (arg == "-h" || arg == "--height") {
            if (i + 1 < argc) config.image.height = std::stoi(argv[++i]);
        } else if (arg == "-s" || arg == "--samples") {
            if (i + 1 < argc) config.samplesPerPixel = std::stoi(argv[++i]);
        } else if (arg == "-d" || arg == "--depth") {
            if (i + 1 < argc) config.maxDepth = std::stoi(argv[++i]);
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) config.image.outputPath = argv[++i];
        } else if (arg == "-i" || arg == "--integrator") {
            if (i + 1 < argc) {
                std::string type = argv[++i];
                if (type == "simple") config.integratorType = IntegratorType::Simple;
                else if (type == "whitted") config.integratorType = IntegratorType::Whitted;
                else if (type == "pathtracing" || type == "pt") config.integratorType = IntegratorType::PathTracing;
                else if (type == "volume") config.integratorType = IntegratorType::Volume;
                else if (type == "volumepath" || type == "vpt") config.integratorType = IntegratorType::VolumetricPath;
                else if (type == "normal") config.integratorType = IntegratorType::Normal;
                else if (type == "depth") config.integratorType = IntegratorType::Depth;
                else if (type == "ao") config.integratorType = IntegratorType::AmbientOcclusion;
            }
        } else if (arg == "-a" || arg == "--accelerator") {
            if (i + 1 < argc) {
                std::string name = argv[++i];
                if (!parseAcceleratorType(name, config.accelerator)) {
                    std::cerr << "Unknown accelerator '" << name << "' (using "
                              << acceleratorName(config.accelerator) << ")\n";
                }
            }
        } else if (arg == "--scene") {
            if (i + 1 < argc) config.scenePath = argv[++i];
        } else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        } else if (arg == "--no-progress") {
            config.showProgress = false;
        } else if (arg == "--no-nee") {
            config.useNEE = false;
        } else if (arg == "--no-mis") {
            config.useMIS = false;
        } else if (arg == "--no-rr") {
            config.useRussianRoulette = false;
        } else if (arg == "--help") {
            printHelp();
            exit(0);
        }
    }
    
    return config;
}

std::string Config::serialize(const RenderConfig& config) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"integrator\": {\n";
    ss << "    \"type\": \"" << integratorName(config.integratorType) << "\",\n";
    ss << "    \"maxDepth\": " << config.maxDepth << ",\n";
    ss << "    \"samplesPerPixel\": " << config.samplesPerPixel << ",\n";
    ss << "    \"russianRoulette\": " << (config.useRussianRoulette ? "true" : "false") << ",\n";
    ss << "    \"nee\": " << (config.useNEE ? "true" : "false") << ",\n";
    ss << "    \"mis\": " << (config.useMIS ? "true" : "false") << "\n";
    ss << "  },\n";
    ss << "  \"camera\": {\n";
    ss << "    \"type\": \"" << cameraName(config.camera.type) << "\",\n";
    ss << "    \"position\": [" << config.camera.position.x << ", " 
       << config.camera.position.y << ", " << config.camera.position.z << "],\n";
    ss << "    \"lookAt\": [" << config.camera.lookAt.x << ", " 
       << config.camera.lookAt.y << ", " << config.camera.lookAt.z << "],\n";
    ss << "    \"fov\": " << config.camera.fov << ",\n";
    ss << "    \"aperture\": " << config.camera.aperture << ",\n";
    ss << "    \"focusDistance\": " << config.camera.focusDistance << "\n";
    ss << "  },\n";
    ss << "  \"image\": {\n";
    ss << "    \"width\": " << config.image.width << ",\n";
    ss << "    \"height\": " << config.image.height << ",\n";
    ss << "    \"output\": \"" << config.image.outputPath << "\",\n";
    ss << "    \"toneMapping\": " << (config.image.useToneMapping ? "true" : "false") << ",\n";
    ss << "    \"gamma\": " << config.image.gamma << ",\n";
    ss << "    \"exposure\": " << config.image.exposure << "\n";
    ss << "  },\n";
    ss << "  \"scene\": \"" << config.scenePath << "\",\n";
    ss << "  \"accelerator\": \"" << acceleratorName(config.accelerator) << "\",\n";
    ss << "  \"background\": [" << config.backgroundColor.x << ", " 
       << config.backgroundColor.y << ", " << config.backgroundColor.z << "],\n";
    ss << "  \"ambient\": [" << config.ambientLight.x << ", " 
       << config.ambientLight.y << ", " << config.ambientLight.z << "]\n";
    ss << "}\n";
    return ss.str();
}

void Config::printHelp() {
    std::cout << "Ray-Based Rendering Framework\n";
    std::cout << "Usage: render [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -c, --config <file>     Load configuration from JSON file\n";
    std::cout << "  -w, --width <n>         Image width (default: 800)\n";
    std::cout << "  -h, --height <n>        Image height (default: 600)\n";
    std::cout << "  -s, --samples <n>       Samples per pixel (default: 64)\n";
    std::cout << "  -d, --depth <n>         Max ray depth (default: 10)\n";
    std::cout << "  -o, --output <file>     Output filename (default: output.png)\n";
    std::cout << "  -i, --integrator <type> Integrator type:\n";
    std::cout << "                            simple, whitted, pathtracing (pt),\n";
    std::cout << "                            volume, volumepath (vpt),\n";
    std::cout << "                            normal, depth, ao\n";
    std::cout << "  -a, --accelerator <t>   Acceleration structure: bvh (default), grid,\n";
    std::cout << "                            kdtree, octree\n";
    std::cout << "  --scene <file>         Scene file path\n";
    std::cout << "  --no-nee                Disable Next Event Estimation\n";
    std::cout << "  --no-mis                Disable Multiple Importance Sampling\n";
    std::cout << "  --no-rr                 Disable Russian Roulette\n";
    std::cout << "  -v, --verbose           Verbose output\n";
    std::cout << "  --no-progress           Disable progress bar\n";
    std::cout << "  --help                  Show this help message\n";
}

void Config::parseRenderConfig(const ConfigValue& json, RenderConfig& config) {
    // Integrator
    if (json.contains("integrator")) {
        auto& integrator = json["integrator"];
        if (integrator.contains("type")) {
            std::string type = integrator["type"].asString();
            if (type == "simple") config.integratorType = IntegratorType::Simple;
            else if (type == "whitted") config.integratorType = IntegratorType::Whitted;
            else if (type == "pathtracing") config.integratorType = IntegratorType::PathTracing;
            else if (type == "volume") config.integratorType = IntegratorType::Volume;
            else if (type == "volumepath") config.integratorType = IntegratorType::VolumetricPath;
            else if (type == "normal") config.integratorType = IntegratorType::Normal;
            else if (type == "depth") config.integratorType = IntegratorType::Depth;
            else if (type == "ao") config.integratorType = IntegratorType::AmbientOcclusion;
        }
        if (integrator.contains("maxDepth")) config.maxDepth = integrator["maxDepth"].asInt();
        if (integrator.contains("samplesPerPixel")) config.samplesPerPixel = integrator["samplesPerPixel"].asInt();
        if (integrator.contains("russianRoulette")) config.useRussianRoulette = integrator["russianRoulette"].asBool();
        if (integrator.contains("nee")) config.useNEE = integrator["nee"].asBool();
        if (integrator.contains("mis")) config.useMIS = integrator["mis"].asBool();
    }
    
    // Camera
    if (json.contains("camera")) {
        auto& camera = json["camera"];
        if (camera.contains("type")) {
            std::string type = camera["type"].asString();
            if (type == "perspective") config.camera.type = CameraType::Perspective;
            else if (type == "orthographic") config.camera.type = CameraType::Orthographic;
            else if (type == "environment") config.camera.type = CameraType::Environment;
        }
        if (camera.contains("position")) config.camera.position = camera["position"].asVec3();
        if (camera.contains("lookAt")) config.camera.lookAt = camera["lookAt"].asVec3();
        if (camera.contains("up")) config.camera.up = camera["up"].asVec3();
        if (camera.contains("fov")) config.camera.fov = camera["fov"].asFloat();
        if (camera.contains("aperture")) config.camera.aperture = camera["aperture"].asFloat();
        if (camera.contains("focusDistance")) config.camera.focusDistance = camera["focusDistance"].asFloat();
        if (camera.contains("orthoHeight")) config.camera.orthoHeight = camera["orthoHeight"].asFloat();
    }
    
    // Image
    if (json.contains("image")) {
        auto& image = json["image"];
        if (image.contains("width")) config.image.width = image["width"].asInt();
        if (image.contains("height")) config.image.height = image["height"].asInt();
        if (image.contains("output")) config.image.outputPath = image["output"].asString();
        if (image.contains("toneMapping")) config.image.useToneMapping = image["toneMapping"].asBool();
        if (image.contains("gamma")) config.image.gamma = image["gamma"].asFloat();
        if (image.contains("exposure")) config.image.exposure = image["exposure"].asFloat();
    }
    
    // Volume
    if (json.contains("volume")) {
        auto& volume = json["volume"];
        if (volume.contains("steps")) config.volumeSteps = volume["steps"].asInt();
        if (volume.contains("scattering")) config.volumeScattering = volume["scattering"].asBool();
        if (volume.contains("multipleScattering")) config.volumeMultipleScattering = volume["multipleScattering"].asBool();
    }
    
    // Scene
    if (json.contains("scene")) config.scenePath = json["scene"].asString();
    if (json.contains("accelerator")) {
        const std::string name = json["accelerator"].asString();
        if (!parseAcceleratorType(name, config.accelerator)) {
            std::cerr << "Unknown accelerator '" << name << "' in config (using "
                      << acceleratorName(config.accelerator) << ")\n";
        }
    }
    if (json.contains("background")) config.backgroundColor = json["background"].asVec3();
    if (json.contains("ambient")) config.ambientLight = json["ambient"].asVec3();
    
    // Debug
    if (json.contains("verbose")) config.verbose = json["verbose"].asBool();
    if (json.contains("showProgress")) config.showProgress = json["showProgress"].asBool();
}

std::string Config::integratorName(IntegratorType type) {
    switch (type) {
        case IntegratorType::Simple: return "simple";
        case IntegratorType::Whitted: return "whitted";
        case IntegratorType::PathTracing: return "pathtracing";
        case IntegratorType::Volume: return "volume";
        case IntegratorType::VolumetricPath: return "volumepath";
        case IntegratorType::Normal: return "normal";
        case IntegratorType::Depth: return "depth";
        case IntegratorType::AmbientOcclusion: return "ao";
        default: return "unknown";
    }
}

std::string Config::cameraName(CameraType type) {
    switch (type) {
        case CameraType::Perspective: return "perspective";
        case CameraType::Orthographic: return "orthographic";
        case CameraType::Environment: return "environment";
        default: return "unknown";
    }
}

} // namespace render
