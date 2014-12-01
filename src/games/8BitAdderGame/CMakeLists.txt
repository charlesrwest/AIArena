cmake_minimum_required (VERSION 2.8.3)

FILE(GLOB SOURCEFILES *.cpp *.c)

#message( ${SOURCEFILES} ${ProtoSources} )


#Add the compilation target
ADD_EXECUTABLE(8BitAdderGameExample ${SOURCEFILES})

#link libraries to executable
target_link_libraries(8BitAdderGameExample AIArena ${PROTOBUF_LIBRARY} zmq)
