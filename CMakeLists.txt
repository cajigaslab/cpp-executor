cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0042 NEW)
cmake_policy(SET CMP0071 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0091 NEW)
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24")
  cmake_policy(SET CMP0135 NEW)
endif()
set(CMAKE_POLICY_DEFAULT_CMP0042 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project(cpp-executor)

# Default to C99
if(NOT CMAKE_C_STANDARD) 
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++17
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(ABSL_PROPAGATE_CXX_STD ON)

if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  set(CMAKE_BUILD_TYPE Debug)
endif()

include(FetchContent)

if("${SANITIZER}" STREQUAL thread)
  list(APPEND ALL_COMPILE_OPTIONS -fsanitize=thread -O1)
  list(APPEND ALL_LINK_OPTIONS -fsanitize=thread -O1)
elseif("${SANITIZER}" STREQUAL address)
  if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    list(APPEND ALL_COMPILE_OPTIONS -fsanitize=address)
  else()
    list(APPEND ALL_COMPILE_OPTIONS -fsanitize=address -O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls)
    list(APPEND ALL_LINK_OPTIONS -fsanitize=address -O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls)
  endif()
elseif("${SANITIZER}" STREQUAL memory)
  list(APPEND ALL_COMPILE_OPTIONS -fsanitize=memory -fsanitize-memory-track-origins=2 -O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls)
  list(APPEND ALL_LINK_OPTIONS -fsanitize=memory -fsanitize-memory-track-origins=2 -O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls)
endif()

string(REPLACE ";" " " ALL_C_COMPILE_OPTIONS_SPACED "${ALL_COMPILE_OPTIONS}")
string(REPLACE ";" " " ALL_C_LINK_OPTIONS_SPACED "${ALL_LINK_OPTIONS}")

if(NOT WIN32 AND NOT APPLE)
  include(cmake/llvm.cmake)
  list(APPEND ALL_COMPILE_OPTIONS ${LIBCXX_COMPILE_OPTIONS})
  list(APPEND ALL_LINK_OPTIONS ${LIBCXX_LINK_OPTIONS})
endif()

add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:${ALL_COMPILE_OPTIONS}>")
add_link_options("$<$<COMPILE_LANGUAGE:CXX>:${ALL_LINK_OPTIONS}>")
string(REPLACE ";" " " ALL_COMPILE_OPTIONS_SPACED "${ALL_COMPILE_OPTIONS}")
string(REPLACE ";" " " ALL_LINK_OPTIONS_SPACED "${ALL_LINK_OPTIONS}")

find_package(Python COMPONENTS Interpreter)
include(cmake/grpc.cmake)
include(cmake/perfetto.cmake)
#include(cmake/boost.cmake)
include(cmake/skia.cmake)
include(cmake/sdl.cmake)
include(cmake/imgui.cmake)
include(cmake/lua.cmake)
include(cmake/boost.cmake)

set(PROTO_SOURCES thalamus.proto task_controller.proto)
apply_protoc_grpc(PROTO_GENERATED_SOURCES ${PROTO_SOURCES})
add_library(protoc_generated OBJECT ${PROTO_GENERATED_SOURCES} ${PROTO_SOURCES})
target_link_libraries(protoc_generated grpc++ grpc++_reflection)

set(WARNING_FLAGS 
  -Weverything -Wno-deprecated-declarations -Wno-c11-extensions -Wno-gcc-compat -Wno-c++98-compat-pedantic
  -Wno-c++20-compat -Wno-unsafe-buffer-usage -Wno-switch-default -Wno-c99-extensions
  -Wno-missing-template-arg-list-after-template-kw -Wno-padded -Wno-unknown-warning-option)

include_directories(${CMAKE_BINARY_DIR})

add_executable(cpp_executor 
  src/task.cpp
  src/task.hpp
  src/simple_task.cpp
  src/simple_task.hpp
  src/lua_task.cpp
  src/lua_task.hpp
  src/hold_state_machine.cpp
  src/hold_state_machine.hpp
  src/state.cpp
  src/state.hpp
  src/state_manager.cpp
  src/state_manager.hpp
  src/lua/rect.cpp
  src/lua/rect.hpp
  src/lua/font.cpp
  src/lua/font.hpp
  src/lua/paint.cpp
  src/lua/paint.hpp
  src/lua/color.cpp
  src/lua/color.hpp
  src/lua/fontstyle.cpp
  src/lua/fontstyle.hpp
  src/lua/typeface.cpp
  src/lua/typeface.hpp
  src/lua/fontmanager.cpp
  src/lua/fontmanager.hpp
  src/lua/canvas.cpp
  src/lua/canvas.hpp
  src/thalamus/tracing.cpp
  src/thalamus/tracing.hpp
  src/main.cpp)

if(WIN32)
  target_compile_definitions(cpp_executor PRIVATE _WIN32_WINNT=0x0A00 _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN BOOST_STACKTRACE_USE_WINDBG)
  target_link_libraries(cpp_executor dbgeng ole32 kernel32 user32 gdi32 winmm imm32 oleaut32 version uuid advapi32 setupapi shell32 dinput8 opengl32 usp10)
  set_target_properties(cpp_executor PROPERTIES WIN32_EXECUTABLE YES)
elseif(APPLE)
  target_compile_definitions(cpp_executor PRIVATE _GNU_SOURCE)
else()
  target_link_libraries(cpp_executor m)
endif()
target_compile_options(cpp_executor PRIVATE ${WARNING_FLAGS})
target_include_directories(cpp_executor PRIVATE src)
target_link_libraries(cpp_executor sdl skia protoc_generated grpc++ perfetto imgui jsoncpp lua boost perfetto)

if(NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
  target_link_options(cpp_executor PRIVATE -g)
endif()

