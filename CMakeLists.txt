# ou_sdk — 水下机器人上位机侧 SDK (C++20)
cmake_minimum_required(VERSION 3.16)
project(ou_sdk VERSION 0.1.0 LANGUAGES CXX)

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

option(OU_SDK_BUILD_TESTS "构建单元测试" ON)

# 核心库: 协议编解码 (无外部依赖, 纯标准库)
add_library(ou_proto
    src/protocol.cpp
)
add_library(ou::proto ALIAS ou_proto)

target_include_directories(ou_proto PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated>
    $<INSTALL_INTERFACE:include>
)
target_compile_definitions(ou_proto PUBLIC OU_PROTO_NAMESPACE=ou)

# 传输库: 纯字节通道 (无外部依赖, POSIX/Windows 原生 API)
add_library(ou_channel
    src/udp_channel.cpp
    src/serial_channel.cpp
)
add_library(ou::channel ALIAS ou_channel)

target_include_directories(ou_channel PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 安装规则: 供 ROVAPP 以 find_package(ou_sdk) 或 add_subdirectory 消费
include(GNUInstallDirs)
install(TARGETS ou_proto ou_channel
    EXPORT ou_sdkTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY generated/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT ou_sdkTargets
    NAMESPACE ou::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ou_sdk
)

if(OU_SDK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
