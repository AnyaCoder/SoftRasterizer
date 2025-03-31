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

# Windows平台不需要显式链接数学库
