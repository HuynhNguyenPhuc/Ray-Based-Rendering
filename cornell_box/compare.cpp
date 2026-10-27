// Renders the Cornell box with every rendering technique and compares the results.
//
// Usage: render_cornell_compare [-w W] [-h H] [-s SPP] [-d DEPTH] [-o OUT_DIR] [--no-fog]
//
// Writes one PNG per technique plus a contact sheet (comparison.png) to OUT_DIR
// and prints a timing table.

#include "cornell_box.h"

#include "render/core/film.h"
#include "render/io/image_exporter.h"
#include "render/renderers/debug_integrators.h"
#include "render/renderers/path_tracing_integrator.h"
#include "render/renderers/simple_integrator.h"
#include "render/renderers/volume_integrator.h"
#include "render/renderers/volumetric_path_integrator.h"
#include "render/renderers/whitted_integrator.h"

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace render;

namespace {

struct Technique {
    std::string id;     // file name stem
    std::string label;  // human readable name
    std::unique_ptr<Integrator> integrator;
    bool needsFog;      // true: rendered in the foggy scene
};

struct Result {
    std::string id;
    std::string label;
    double seconds = 0.0;
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;
};

struct Options {
    int width = 256;
    int height = 256;
    int samples = 16;
    int depth = 8;
    std::string outDir = "results/cornell_box";
    bool fog = true;
};

Options parseArgs(int argc, char** argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](int& target) { if (i + 1 < argc) target = std::atoi(argv[++i]); };
        if (arg == "-w" || arg == "--width") next(opt.width);
        else if (arg == "-h" || arg == "--height") next(opt.height);
        else if (arg == "-s" || arg == "--samples") next(opt.samples);
        else if (arg == "-d" || arg == "--depth") next(opt.depth);
        else if ((arg == "-o" || arg == "--output") && i + 1 < argc) opt.outDir = argv[++i];
        else if (arg == "--no-fog") opt.fog = false;
        else if (arg == "--help") {
            std::cout << "Usage: render_cornell_compare [-w W] [-h H] [-s SPP] [-d DEPTH] "
                         "[-o OUT_DIR] [--no-fog]\n";
            std::exit(0);
        }
    }
    return opt;
}

std::vector<Technique> makeTechniques(const Options& opt) {
    IntegratorConfig base;
    base.maxDepth = opt.depth;
    base.samplesPerPixel = opt.samples;

    IntegratorConfig noNEE = base;
    noNEE.useNEE = false;

    VolumeConfig vol;

    std::vector<Technique> t;
    t.push_back({"simple", "Direct lighting (Blinn-Phong)", std::make_unique<SimpleIntegrator>(base), false});
    t.push_back({"whitted", "Whitted ray tracing", std::make_unique<WhittedIntegrator>(base), false});
    t.push_back({"path_tracing_nee", "Path tracing (NEE)", std::make_unique<PathTracingIntegrator>(base), false});
    t.push_back({"path_tracing_no_nee", "Path tracing (no NEE)", std::make_unique<PathTracingIntegrator>(noNEE), false});
    t.push_back({"ambient_occlusion", "Ambient occlusion", std::make_unique<AOIntegrator>(16, 2.0f), false});
    t.push_back({"normals", "Normals", std::make_unique<NormalIntegrator>(), false});
    t.push_back({"depth", "Depth", std::make_unique<DepthIntegrator>(3.0f, 8.0f), false});
    if (opt.fog) {
        t.push_back({"volume", "Volume ray marching (fog)", std::make_unique<VolumeIntegrator>(base, vol), true});
        t.push_back({"volumetric_path", "Volumetric path tracing (fog)",
                     std::make_unique<VolumetricPathIntegrator>(base, vol), true});
    }
    return t;
}

// Tile all results into one image (row-major, `columns` per row).
bool writeContactSheet(const std::string& path, const std::vector<Result>& results, int columns) {
    if (results.empty()) return false;
    const int tileW = results[0].width;
    const int tileH = results[0].height;
    const int rows = (static_cast<int>(results.size()) + columns - 1) / columns;
    const int sheetW = tileW * columns;
    const int sheetH = tileH * rows;

    std::vector<unsigned char> sheet(static_cast<size_t>(sheetW) * sheetH * 3, 0);
    for (size_t n = 0; n < results.size(); ++n) {
        const int ox = (static_cast<int>(n) % columns) * tileW;
        const int oy = (static_cast<int>(n) / columns) * tileH;
        for (int y = 0; y < tileH; ++y) {
            for (int x = 0; x < tileW; ++x) {
                for (int c = 0; c < 3; ++c) {
                    sheet[((static_cast<size_t>(oy + y) * sheetW) + (ox + x)) * 3 + c] =
                        results[n].pixels[(static_cast<size_t>(y) * tileW + x) * 3 + c];
                }
            }
        }
    }
    return ImageExporter::writeRGB8(path, sheetW, sheetH, sheet);
}

} // namespace

int main(int argc, char** argv) {
    Options opt = parseArgs(argc, argv);

    std::cout << "=== Cornell box technique comparison ===\n"
              << "Resolution: " << opt.width << "x" << opt.height
              << ", samples/pixel: " << opt.samples
              << ", max depth: " << opt.depth << "\n\n";

    Scene plainScene = cornell_box::createScene(false);
    Scene fogScene = cornell_box::createScene(true);
    auto camera = cornell_box::createCamera(opt.width, opt.height);

    std::vector<Technique> techniques = makeTechniques(opt);
    std::vector<Result> results;
    bool allWritten = true;

    for (auto& tech : techniques) {
        const Scene& scene = tech.needsFog ? fogScene : plainScene;
        Film film(opt.width, opt.height);

        auto start = std::chrono::high_resolution_clock::now();
        tech.integrator->render(scene, *camera, film);
        auto end = std::chrono::high_resolution_clock::now();

        Result r;
        r.id = tech.id;
        r.label = tech.label;
        r.seconds = std::chrono::duration<double>(end - start).count();
        r.width = opt.width;
        r.height = opt.height;
        r.pixels = film.toRGB8ToneMapped(1.0f, 2.2f);

        const std::string path = opt.outDir + "/" + tech.id + ".png";
        if (!ImageExporter::writeRGB8(path, r.width, r.height, r.pixels)) {
            std::cerr << "Error: failed to write " << path << "\n";
            allWritten = false;
        }

        std::cout << std::left << std::setw(32) << tech.label
                  << std::right << std::fixed << std::setprecision(2) << std::setw(8) << r.seconds << " s"
                  << "   -> " << path << "\n";
        results.push_back(std::move(r));
    }

    const std::string sheetPath = opt.outDir + "/comparison.png";
    if (writeContactSheet(sheetPath, results, 3)) {
        std::cout << "\nContact sheet (row-major, 3 per row): " << sheetPath << "\n";
    } else {
        std::cerr << "Error: failed to write " << sheetPath << "\n";
        allWritten = false;
    }

    return allWritten ? 0 : 1;
}
