cmake_minimum_required(VERSION 3.16)
project(RayBasedRendering VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /permissive-")
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
endif()

# Find OpenMP for parallelization
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found - enabling parallel rendering")
else()
    message(WARNING "OpenMP not found - parallel rendering disabled")
endif()

# Options
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_EXAMPLES "Build example applications" ON)

# Core rendering library (headers in include/render/<module>, sources in src/<module>)
set(RENDER_SOURCES
    # acceleration
    src/acceleration/accelerator_factory.cpp
    src/acceleration/bvh.cpp
    src/acceleration/uniform_grid.cpp
    src/acceleration/kd_tree.cpp
    src/acceleration/octree.cpp
    # camera
    src/camera/camera.cpp
    src/camera/pinhole_camera.cpp
    src/camera/thin_lens_camera.cpp
    # config
    src/config/config.cpp
    src/config/render_factory.cpp
    # core
    src/core/film.cpp
    src/core/sampler.cpp
    # geometry
    src/geometry/mesh.cpp
    src/geometry/plane.cpp
    src/geometry/primitive.cpp
    src/geometry/quad.cpp
    src/geometry/sphere.cpp
    src/geometry/triangle.cpp
    # io
    src/io/image_exporter.cpp
    src/io/json_parser.cpp
    src/io/obj_importer.cpp
    # lighting
    src/lighting/area_light.cpp
    src/lighting/directional_light.cpp
    src/lighting/point_light.cpp
    src/lighting/spot_light.cpp
    # materials
    src/materials/bsdf.cpp
    src/materials/lambertian_bsdf.cpp
    src/materials/microfacet_bsdf.cpp
    src/materials/mixture_bsdf.cpp
    src/materials/material.cpp
    src/materials/medium.cpp
    src/materials/phase_function.cpp
    src/materials/specular_bsdf.cpp
    # renderers
    src/renderers/debug_integrators.cpp
    src/renderers/integrator.cpp
    src/renderers/path_tracing_integrator.cpp
    src/renderers/simple_integrator.cpp
    src/renderers/volume_integrator.cpp
    src/renderers/volumetric_path_integrator.cpp
    src/renderers/whitted_integrator.cpp
    # scene
    src/scene/scene.cpp
    src/scene/scene_builder.cpp
    # cornell box scene (shared by the CLI and the comparison app)
    cornell_box/cornell_box.cpp
)

add_library(render_lib STATIC ${RENDER_SOURCES})
target_include_directories(render_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include   # "render/<module>/<header>.h"
    ${CMAKE_CURRENT_SOURCE_DIR}           # "third_party/..." and "cornell_box/..."
)
if(OpenMP_CXX_FOUND)
    target_link_libraries(render_lib PUBLIC OpenMP::OpenMP_CXX)
endif()

# Command-line renderer
add_executable(render apps/main.cpp)
target_link_libraries(render PRIVATE render_lib)

# Accelerator comparison: BVH vs grid vs kd-tree vs octree on an OBJ mesh
add_executable(render_accel_compare apps/accelerator_compare.cpp)
target_link_libraries(render_accel_compare PRIVATE render_lib)

# Cornell box: compare all rendering techniques on one scene
add_executable(render_cornell_compare cornell_box/compare.cpp)
target_link_libraries(render_cornell_compare PRIVATE render_lib)

# Example applications
if(BUILD_EXAMPLES)
    foreach(example simple_scene cornell_box volume_scene material_cell)
        add_executable(render_${example} examples/${example}.cpp)
        target_link_libraries(render_${example} PRIVATE render_lib)
    endforeach()
endif()

# Tests (each tests/test_*.cpp is built as its own executable)
if(BUILD_TESTS)
    enable_testing()
    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.cpp)
    if(NOT TEST_SOURCES)
        message(WARNING "BUILD_TESTS is ON but no tests/test_*.cpp files were found")
    endif()
    foreach(test_src ${TEST_SOURCES})
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_link_libraries(${test_name} PRIVATE render_lib)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

# Installation
install(TARGETS render render_cornell_compare render_accel_compare RUNTIME DESTINATION bin)
install(TARGETS render_lib ARCHIVE DESTINATION lib)
install(DIRECTORY include/render/ DESTINATION include/render FILES_MATCHING PATTERN "*.h")

# Print configuration summary
message(STATUS "")
message(STATUS "=== Ray-Based Rendering Framework Configuration ===")
message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler:      ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "OpenMP:            ${OpenMP_CXX_FOUND}")
message(STATUS "Build tests:       ${BUILD_TESTS}")
message(STATUS "Build examples:    ${BUILD_EXAMPLES}")
message(STATUS "")
