FetchContent_Declare(
  gRPC
  GIT_REPOSITORY https://github.com/grpc/grpc
  GIT_TAG        v1.71.0
)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)
set(FETCHCONTENT_QUIET OFF)
set(gRPC_MSVC_STATIC_RUNTIME ON)
set(ABSL_MSVC_STATIC_RUNTIME ON)
set(protobuf_MSVC_STATIC_RUNTIME ON CACHE BOOL "Protobuf: Link static runtime libraries")
set(ABSL_ENABLE_INSTALL ON)
#add_definitions(-DBORINGSSL_NO_CXX)
#set(gRPC_BUILD_TESTS ON)
FetchContent_MakeAvailable(gRPC)
#file(READ "${grpc_SOURCE_DIR}/third_party/zlib/CMakeLists.txt" FILE_CONTENTS)
#string(REPLACE "cmake_minimum_required(VERSION 2.4.4)" "cmake_minimum_required(VERSION 3.12)" FILE_CONTENTS "${FILE_CONTENTS}")
#file(WRITE "${grpc_SOURCE_DIR}/third_party/zlib/CMakeLists.txt" "${FILE_CONTENTS}")
#add_subdirectory("${grpc_SOURCE_DIR}" "${grpc_BINARY_DIR}")

#target_compile_options(crypto PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-w>")

add_library(jsoncpp 
  "${grpc_SOURCE_DIR}/third_party/protobuf/third_party/jsoncpp/src/lib_json/json_reader.cpp"
  "${grpc_SOURCE_DIR}/third_party/protobuf/third_party/jsoncpp/src/lib_json/json_tool.h"
  "${grpc_SOURCE_DIR}/third_party/protobuf/third_party/jsoncpp/src/lib_json/json_value.cpp"
  "${grpc_SOURCE_DIR}/third_party/protobuf/third_party/jsoncpp/src/lib_json/json_writer.cpp")
target_include_directories(jsoncpp PUBLIC 
  "${grpc_SOURCE_DIR}/third_party/protobuf/third_party/jsoncpp/include")
target_compile_definitions(jsoncpp PUBLIC JSON_USE_EXCEPTION=0 JSON_HAS_INT64)

macro(apply_protoc_grpc OUTPUT_SOURCES)
  
  foreach(PROTO_FILE ${ARGN})
    get_filename_component(PROTO_ABSOLUTE "${PROTO_FILE}" ABSOLUTE)
    get_filename_component(PROTO_NAME "${PROTO_FILE}" NAME_WE)
    get_filename_component(PROTO_DIRECTORY "${PROTO_ABSOLUTE}" DIRECTORY)
    set(apply_protoc_grpc_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h"
                                    "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc"
                                    "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h"
                                    "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc")
    add_custom_command(
          OUTPUT ${apply_protoc_grpc_GENERATED}
          COMMAND $<TARGET_FILE:protobuf::protoc> --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
          --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
          --plugin=protoc-gen-grpc=$<TARGET_FILE:grpc_cpp_plugin>
          -I "${PROTO_DIRECTORY}"
          ${PROTO_ABSOLUTE}
          DEPENDS "${PROTO_ABSOLUTE}" $<TARGET_FILE:protobuf::protoc>)
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
      set_source_files_properties(
        ${${OUTPUT_SOURCES}}
        PROPERTIES
        COMPILE_FLAGS /wd4267)
    endif()
    list(APPEND ${OUTPUT_SOURCES} ${apply_protoc_grpc_GENERATED})
  endforeach()

endmacro()

macro(apply_protoc OUTPUT_SOURCES)
  
  foreach(PROTO_FILE ${ARGN})
    get_filename_component(PROTO_ABSOLUTE "${PROTO_FILE}" ABSOLUTE)
    get_filename_component(PROTO_NAME "${PROTO_FILE}" NAME_WE)
    get_filename_component(PROTO_DIRECTORY "${PROTO_ABSOLUTE}" DIRECTORY)
    set(apply_protoc_grpc_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h"
                                    "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc")
    add_custom_command(
          OUTPUT ${apply_protoc_grpc_GENERATED}
          COMMAND $<TARGET_FILE:protobuf::protoc>
          --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
          -I "${PROTO_DIRECTORY}"
          ${PROTO_ABSOLUTE}
          DEPENDS "${PROTO_ABSOLUTE}" $<TARGET_FILE:protobuf::protoc>)
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
      set_source_files_properties(
        ${${OUTPUT_SOURCES}}
        PROPERTIES
        COMPILE_FLAGS /wd4267)
    endif()
    list(APPEND ${OUTPUT_SOURCES} ${apply_protoc_grpc_GENERATED})
  endforeach()

endmacro()
