project(base64)

add_library(base64_lib STATIC base64.cpp)
add_library(protocol_lib STATIC protocol.cpp base64.cpp)
