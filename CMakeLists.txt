cmake_minimum_required(VERSION 3.16)
project(dwframe)
include (utils.cmake)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++2a -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

find_library(YAMLCPP yaml-cpp)
set(LIB_SRC
    dwframe/address.cc
    dwframe/bytearray.cc
    dwframe/config.cc
    dwframe/fd_manager.cc
    dwframe/fiber.cc
    dwframe/hook.cc
    dwframe/http/http.cc
    dwframe/http/http_parser.cc
    dwframe/http/http_session.cc
    dwframe/http/http11_parser.rl.cc
    dwframe/http/httpclient_parser.rl.cc
    dwframe/http/http_server.cc
    dwframe/http/servlet.cc
    dwframe/IoManager.cc
    dwframe/log.cc
    dwframe/scheduler.cc
    dwframe/socket.cc
    dwframe/socket_stream.cc
    dwframe/stream.cc
    dwframe/Tcp_server.cc
    dwframe/thread.cc
    dwframe/timer.cc
    dwframe/util.cc
    )
set(LIB_LIB
    dwframe
    ${YAMLCPP}
    pthread
    dl
    )
add_library(dwframe SHARED ${LIB_SRC})
redefine_file_macro(dwframe) #__FILE__


add_executable(test tests/test.cc)
add_dependencies(test dwframe)
redefine_file_macro(test) #__FILE__
target_link_libraries(test ${LIB_LIB})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config dwframe)
redefine_file_macro(test_config) #__FILE__
target_link_libraries(test_config ${LIB_LIB})

add_executable(test_pthread tests/test_pthread.cc)
add_dependencies(test_pthread dwframe)
redefine_file_macro(test_pthread) #__FILE__
target_link_libraries(test_pthread ${LIB_LIB})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util dwframe)
redefine_file_macro(test_util) #__FILE__
target_link_libraries(test_util ${LIB_LIB})

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber dwframe)
redefine_file_macro(test_fiber) #__FILE__
target_link_libraries(test_fiber ${LIB_LIB})

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler dwframe)
redefine_file_macro(test_scheduler) #__FILE__
target_link_libraries(test_scheduler ${LIB_LIB})

add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager dwframe)
redefine_file_macro(test_iomanager) #__FILE__
target_link_libraries(test_iomanager ${LIB_LIB})

add_executable(test_hook tests/test_hook.cc)
add_dependencies(test_hook dwframe)
redefine_file_macro(test_hook) #__FILE__
target_link_libraries(test_hook ${LIB_LIB})

add_executable(test_address tests/test_address.cc)
add_dependencies(test_address dwframe)
redefine_file_macro(test_address) #__FILE__
target_link_libraries(test_address ${LIB_LIB})

add_executable(test_socket tests/test_socket.cc)
add_dependencies(test_socket dwframe)
redefine_file_macro(test_socket) #__FILE__
target_link_libraries(test_socket ${LIB_LIB})

add_executable(test_bytearray tests/test_bytearray.cc)
add_dependencies(test_bytearray dwframe)
redefine_file_macro(test_bytearray) #__FILE__
target_link_libraries(test_bytearray ${LIB_LIB})

add_executable(test_http_parser tests/test_http_parser.cc)
add_dependencies(test_http_parser dwframe)
redefine_file_macro(test_http_parser) #__FILE__
target_link_libraries(test_http_parser ${LIB_LIB})

add_executable(test_tcp_server tests/test_tcp_server.cc)
add_dependencies(test_tcp_server dwframe)
redefine_file_macro(test_tcp_server) #__FILE__
target_link_libraries(test_tcp_server ${LIB_LIB})

add_executable(echo_server examples/echo_server.cc)
add_dependencies(echo_server dwframe)
redefine_file_macro(echo_server) #__FILE__
target_link_libraries(echo_server ${LIB_LIB})

add_executable(test_http_server tests/test_http_server.cc)
add_dependencies(test_http_server dwframe)
redefine_file_macro(test_http_server) #__FILE__
target_link_libraries(test_http_server ${LIB_LIB})

add_executable(my_test_http_server tests/my_test_http_server.cc)
add_dependencies(my_test_http_server dwframe)
redefine_file_macro(my_test_http_server) #__FILE__
target_link_libraries(my_test_http_server ${LIB_LIB})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)