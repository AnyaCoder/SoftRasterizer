cmake_minimum_required(VERSION 3.10)
project(SoftRasterizer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/thirdparty/SDL2)
find_package(SDL2 REQUIRED)
message(STATUS "Found SDL2: Headers=${SDL2_INCLUDE_DIRS}, Libraries=${SDL2_LIBRARIES}")

# Find yaml-cpp
find_package(yaml-cpp QUIET)
if (NOT yaml-cpp_FOUND)
    message(STATUS "Building yaml-cpp from source")
    add_subdirectory(thirdparty/yaml-cpp EXCLUDE_FROM_ALL)
endif()

# 源文件
file(GLOB SOURCES
    "src/*.cpp"
    "src/core/*.cpp" 
    "src/core/texture/*.cpp" 
    "src/io/*.cpp"
    "src/math/*.cpp"
)

# 创建可执行文件
add_executable(SoftRasterizer ${SOURCES})

# Link libraries
if(TARGET SDL2::SDL2)
    message(STATUS "Linking against imported target SDL2::SDL2 SDL2::SDL2main yaml-cpp::yaml-cpp")
    target_link_libraries(SoftRasterizer PRIVATE SDL2::SDL2 SDL2::SDL2main yaml-cpp::yaml-cpp)
else()
    message(WARNING "SDL2 imported target not found, linking using variables SDL2_LIBRARIES")
    target_link_libraries(SoftRasterizer PRIVATE ${SDL2_LIBRARIES} yaml-cpp::yaml-cpp)
endif()

target_include_directories(SoftRasterizer PRIVATE include ${SDL2_INCLUDE_DIRS})

include(CheckCXXCompilerFlag) # 包含检查编译器标志的功能

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # 对于 GCC 或 Clang，尝试使用 -march=native
    check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
    if(COMPILER_SUPPORTS_MARCH_NATIVE)
        message(STATUS "Compiler supports -march=native. Enabling for Release builds.")
        # 使用 $<CONFIG:Release> 生成器表达式确保只在Release模式下添加
        target_compile_options(SoftRasterizer PRIVATE $<$<CONFIG:Release>:-march=native>)
    else()
        message(WARNING "Compiler does not support -march=native. Vectorization might rely on default flags or require manual SSE/AVX flags.")
        # 可以选择性地添加 -mavx2 或 -msse4.2 等后备选项
        check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
        if(COMPILER_SUPPORTS_AVX2)
           target_compile_options(SoftRasterizer PRIVATE $<$<CONFIG:Release>:-mavx2>)
        else()
            message(WARNING "Compiler does not support -march=avx2.")
        endif()
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # 对于 MSVC，尝试使用 /arch:AVX2 (需要 Visual Studio 2013 Update 2 或更高版本)
    # /O2 优化级别（Release默认）是自动向量化的前提
    check_cxx_compiler_flag("/arch:AVX2" COMPILER_SUPPORTS_ARCH_AVX2)
    if(COMPILER_SUPPORTS_ARCH_AVX2)
        message(STATUS "Compiler supports /arch:AVX2. Enabling for Release builds.")
        target_compile_options(SoftRasterizer PRIVATE $<$<CONFIG:Release>:/arch:AVX2>)
    else()
        message(WARNING "Compiler does not support /arch:AVX2. Checking for AVX.")
        check_cxx_compiler_flag("/arch:AVX" COMPILER_SUPPORTS_ARCH_AVX)
        if(COMPILER_SUPPORTS_ARCH_AVX)
            message(STATUS "Compiler supports /arch:AVX. Enabling for Release builds.")
            target_compile_options(SoftRasterizer PRIVATE $<$<CONFIG:Release>:/arch:AVX>)
        else()
            message(WARNING "Compiler does not support /arch:AVX. Relying on default vectorization (usually SSE2 for x64).")
        endif()
    endif()
endif()

# Output directories
set_target_properties(SoftRasterizer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
)

# Copy resources and scene file
add_custom_command(TARGET SoftRasterizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    ${CMAKE_BINARY_DIR}/bin/resources
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/scenes/scene.yaml
    ${CMAKE_BINARY_DIR}/bin/scenes/scene.yaml
)

# Copy SDL2.dll on Windows
set(SDL2_RUNTIME_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/thirdparty/SDL2/lib/x64/SDL2.dll")
if(WIN32 AND EXISTS "${SDL2_RUNTIME_LIBRARY_PATH}")
    add_custom_command(TARGET SoftRasterizer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_RUNTIME_LIBRARY_PATH}"
        $<TARGET_FILE_DIR:SoftRasterizer>/SDL2.dll
        COMMENT "Copying SDL2.dll to output directory"
    )
endif()