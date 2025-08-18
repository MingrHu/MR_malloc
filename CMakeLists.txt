cmake_minimum_required(VERSION 3.10)

# 项目配置
project(MR_MALLOC
    VERSION 1.0
    DESCRIPTION "MR_MALLOC Memory Pool Project"
    LANGUAGES CXX
)

# === 编译器和构建选项 ===
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 默认构建类型为 Debug
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Debug 模式编译选项
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Werror -O0 -ggdb")

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 头文件目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# === 定义库和可执行文件 ===
# 将核心功能封装为静态库（便于复用和链接）
add_library(mr_malloc_lib STATIC
    src/ThreadCache.cpp
    src/CentralCache.cpp
    src/PageCache.cpp
)

# 添加可执行文件（仅包含 BenchMark.cpp）
add_executable(BenchMark src/BenchMark.cpp)

# 链接库到可执行文件
target_link_libraries(BenchMark PRIVATE mr_malloc_lib)

# === 调试工具支持 ===
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(mr_malloc_lib PRIVATE -fsanitize=address)
    target_link_options(mr_malloc_lib PRIVATE -fsanitize=address)
    target_compile_options(BenchMark PRIVATE -fsanitize=address)
    target_link_options(BenchMark PRIVATE -fsanitize=address)
endif()