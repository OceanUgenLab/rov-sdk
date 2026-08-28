# ou_sdk 单元测试
add_executable(test_protocol test_protocol.cpp)
target_link_libraries(test_protocol PRIVATE ou::proto)
add_test(NAME test_protocol COMMAND test_protocol)

add_executable(test_channel test_channel.cpp)
target_link_libraries(test_channel PRIVATE ou::channel)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(test_channel PRIVATE util)
endif()
add_test(NAME test_channel COMMAND test_channel)

add_executable(test_link test_link.cpp)
target_link_libraries(test_link PRIVATE ou::link)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(test_link PRIVATE util)
endif()
add_test(NAME test_link COMMAND test_link)
