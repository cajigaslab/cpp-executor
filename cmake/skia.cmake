FetchContent_Declare(
  skia
  GIT_REPOSITORY https://skia.googlesource.com/skia.git
  GIT_TAG        chrome/m136
)
FetchContent_MakeAvailable(skia)

message("skia_BINARY_DIR ${skia_BINARY_DIR}")
set(SKIA_LIB "${skia_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}skia${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_custom_command(OUTPUT "${skia_BINARY_DIR}/cmake_did_sync"
                   COMMAND "${Python_EXECUTABLE}" tools/git-sync-deps && cmake -E touch "${skia_BINARY_DIR}/cmake_did_sync"
                   WORKING_DIRECTORY "${skia_SOURCE_DIR}")

add_custom_command(DEPENDS "${skia_BINARY_DIR}/cmake_did_sync"
                   OUTPUT "${skia_BINARY_DIR}/build.ninja"
                   COMMAND cmake 
                   "-Dskia_BINARY_DIR=${skia_BINARY_DIR}"
                   "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
                   "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
		   "-DCMAKE_CXX_FLAGS=\"${LIBCXX_COMPILE_OPTIONS}\""
		   "-DCMAKE_EXE_LINKER_FLAGS=\"${LIBCXX_LINK_OPTIONS}\""
                   -P "${CMAKE_SOURCE_DIR}/cmake/generate_skia.cmake"
                   WORKING_DIRECTORY "${skia_SOURCE_DIR}")

add_custom_command(DEPENDS "${skia_BINARY_DIR}/build.ninja"
                   OUTPUT "${SKIA_LIB}"
                   COMMAND cmake "-Dskia_BINARY_DIR=${skia_BINARY_DIR}" -P "${CMAKE_SOURCE_DIR}/cmake/build_skia.cmake"
                   WORKING_DIRECTORY "${skia_BINARY_DIR}")

add_library(skia INTERFACE "${SKIA_LIB}")
target_include_directories(skia INTERFACE "${skia_SOURCE_DIR}")
target_link_libraries(skia INTERFACE ${SKIA_LIB})
if(WIN32)
else()
  target_link_libraries(skia INTERFACE GLX)
endif()
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  target_compile_definitions(skia INTERFACE "-DSK_TRIVIAL_ABI=[[clang::trivial_abi]]")
endif()
