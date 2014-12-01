cmake_minimum_required (VERSION 2.8.3)

file(GLOB libraryHeaders *.h *.hpp)
file(GLOB librarySource *.cpp *.c)

add_library(AIArena STATIC  ${librarySource} ${libraryHeaders})
target_link_libraries(AIArena ${PROTOBUF_LIBRARY} zmq messages.a)
