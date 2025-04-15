cmake_minimum_required(VERSION 3.10)
project(SoftRasterizer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(include)

# 源文件
file(GLOB SOURCES
    "src/*.cpp"
    "src/core/*.cpp" 
    "src/io/*.cpp"
    "src/math/*.cpp"
)

# 创建可执行文件
add_executable(SoftRasterizer ${SOURCES})

# --- 添加优化和向量化标志 (主要针对Release构建) ---
# 注意：-O2 (或 /O2) 优化通常由 CMAKE_BUILD_TYPE=Release 隐式设置。
# 以下代码主要添加特定于平台的向量化增强标志。

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
# --- 结束 优化和向量化标志 ---

set_target_properties(SoftRasterizer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
)

# 编译后拷贝obj文件夹到构建目录
add_custom_command(TARGET SoftRasterizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    ${CMAKE_BINARY_DIR}/bin/resources
)

# Windows平台不需要显式链接数学库
