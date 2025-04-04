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
)

# 创建可执行文件
add_executable(SoftRasterizer ${SOURCES})

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
