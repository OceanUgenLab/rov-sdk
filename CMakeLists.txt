# rovi_sdk — 水下机器人上位机侧 SDK (C++20)
cmake_minimum_required(VERSION 3.16)
project(rovi_sdk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "构建类型" FORCE)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(ROVI_SDK_BUILD_TESTS "构建单元测试" ON)

# 核心库: 协议编解码 (无外部依赖, 纯标准库)
add_library(rovi_proto
    src/protocol.cpp
)
add_library(rovi::proto ALIAS rovi_proto)

target_include_directories(rovi_proto PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_definitions(rovi_proto PUBLIC ROVI_PROTO_NAMESPACE=rovi)

# 安装规则: 供 ROVAPP 以 find_package(rovi_sdk) 或 add_subdirectory 消费
include(GNUInstallDirs)
install(TARGETS rovi_proto
    EXPORT rovi_sdkTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT rovi_sdkTargets
    NAMESPACE rovi::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rovi_sdk
)

if(ROVI_SDK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
