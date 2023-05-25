cmake_minimum_required(VERSION 3.22)
project(message_test)

set(CMAKE_CXX_STANDARD 17)

# defining if compile server or client
if (DEFINED ENV{SERVER})
    message(STATUS "Compile server")
    set(CXXFLAGS "${CXXFLAGS} -DSERVER")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSERVER")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(message_test main.cpp)
target_link_libraries(message_test PRIVATE Threads::Threads)
