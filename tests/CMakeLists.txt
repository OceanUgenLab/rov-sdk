# ou_sdk 单元测试
add_executable(test_protocol test_protocol.cpp)
target_link_libraries(test_protocol PRIVATE ou::proto)
add_test(NAME test_protocol COMMAND test_protocol)
