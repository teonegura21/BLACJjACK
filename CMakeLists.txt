cmake_minimum_required(VERSION 3.25)
project(BlackjackAIVision LANGUAGES CXX CUDA VERSION 2.0.0)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# CUDA Architecture (RTX 4060 = Ada Lovelace = SM 8.9)
set(CMAKE_CUDA_ARCHITECTURES 89)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode=arch=compute_89,code=sm_89")

# Compiler optimizations for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # MSVC (Windows)
    if(MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob3 /GL /Gw /arch:AVX2")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    # GCC/Clang (Linux)
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -mtune=native -flto")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
    endif()

    # CUDA optimizations
    set(CMAKE_CUDA_FLAGS_RELEASE "${CMAKE_CUDA_FLAGS_RELEASE} -O3 -use_fast_math --generate-line-info")
    set(CMAKE_CUDA_FLAGS_RELEASE "${CMAKE_CUDA_FLAGS_RELEASE} -Xptxas -O3,-v -Xcompiler -O3")

    # Enable Link Time Optimization
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

    # Strip symbols in release
    if(UNIX)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
    endif()
endif()

# Debug build options
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CUDA_FLAGS_DEBUG "${CMAKE_CUDA_FLAGS_DEBUG} -G -g")
    if(MSVC)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od")
    else()
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    endif()
endif()

# Find required packages
find_package(CUDAToolkit 12.0 REQUIRED)
find_package(OpenGL REQUIRED)

# TensorRT (manually specify paths if not in standard location)
set(TensorRT_DIR "" CACHE PATH "TensorRT installation directory")
if(TensorRT_DIR)
    set(TensorRT_INCLUDE_DIRS "${TensorRT_DIR}/include")
    set(TensorRT_LIBRARIES
        "${TensorRT_DIR}/lib/nvinfer.lib"
        "${TensorRT_DIR}/lib/nvinfer_plugin.lib"
        "${TensorRT_DIR}/lib/nvonnxparser.lib"
    )
else()
    find_package(TensorRT REQUIRED)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CUDAToolkit_INCLUDE_DIRS}
    ${TensorRT_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)

# Windows-specific settings
if(WIN32)
    add_compile_definitions(
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _UNICODE
        UNICODE
    )
endif()

# Source files
add_subdirectory(src)

# Main executable
add_executable(blackjack_ai_vision
    src/main.cpp
)

# Set executable properties
set_target_properties(blackjack_ai_vision PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Link libraries
target_link_libraries(blackjack_ai_vision PRIVATE
    core
    capture
    vision
    intelligence
    pipeline
    ui
    utils
    CUDA::cudart
    CUDA::cuda_driver
    ${OPENGL_LIBRARIES}
    ${TensorRT_LIBRARIES}
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(blackjack_ai_vision PRIVATE
        d3d11
        dxgi
        d3dcompiler
    )
endif()

# Post-build: Copy config and DLLs
add_custom_command(TARGET blackjack_ai_vision POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config.json"
        "$<TARGET_FILE_DIR:blackjack_ai_vision>/config.json"
    COMMENT "Copying config.json to output directory"
)

# Installation
install(TARGETS blackjack_ai_vision
    RUNTIME DESTINATION bin
)

install(FILES config.json
    DESTINATION bin
)

install(DIRECTORY models/
    DESTINATION bin/models
    PATTERN "*.md" EXCLUDE
)

# Print configuration summary
message(STATUS "=== Blackjack AI Vision Build Configuration ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "CUDA Architectures: ${CMAKE_CUDA_ARCHITECTURES}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "CUDA Standard: ${CMAKE_CUDA_STANDARD}")
message(STATUS "TensorRT Include: ${TensorRT_INCLUDE_DIRS}")
message(STATUS "===============================================")
