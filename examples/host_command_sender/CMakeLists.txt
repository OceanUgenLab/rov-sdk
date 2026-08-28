# 上位机发指令示例：独立构建，直接引用 SDK 根（不依赖 install）
cmake_minimum_required(VERSION 3.16)
project(host_command_sender LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 关闭 SDK 自带单元测试（示例只需库本身）
set(OU_SDK_BUILD_TESTS OFF CACHE BOOL "示例不构建 SDK 测试" FORCE)

# 引入 SDK 根目录（本目录上一级为 examples/，再上一级即 SDK 根）
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. ou_sdk)

add_executable(host_command_sender main.cpp)
# ou::link 经 PUBLIC 传递链接 ou::proto + ou::channel，一处链接即可
target_link_libraries(host_command_sender PRIVATE ou::link)
