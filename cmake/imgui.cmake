
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.9-docking)
FetchContent_MakeAvailable(imgui)
file(MAKE_DIRECTORY ${imgui_BINARY_DIR}/Debug)
file(MAKE_DIRECTORY ${imgui_BINARY_DIR}/Release)

file(GLOB IMGUI_HEADERS "${imgui_SOURCE_DIR}/*.h")
list(APPEND IMGUI_HEADERS "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.h")
list(APPEND IMGUI_HEADERS "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h")
list(APPEND IMGUI_HEADERS "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3_loader.h")
list(APPEND IMGUI_HEADERS "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.h")
file(GLOB IMGUI_SOURCE "${imgui_SOURCE_DIR}/*.cpp")
list(APPEND IMGUI_SOURCE "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp")
list(APPEND IMGUI_SOURCE "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp")
list(APPEND IMGUI_SOURCE "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp")
add_library(imgui OBJECT ${IMGUI_HEADERS} ${IMGUI_SOURCE})
target_link_libraries(imgui PUBLIC sdl)
target_include_directories(imgui PUBLIC "${imgui_SOURCE_DIR}" "${imgui_SOURCE_DIR}/backends")

add_executable(imgui_demo "${imgui_SOURCE_DIR}/examples/example_sdl3_opengl3/main.cpp")
target_link_libraries(imgui_demo PUBLIC imgui)
if(WIN32)
  target_link_libraries(imgui_demo PUBLIC opengl32)
else()
  target_link_libraries(imgui_demo PUBLIC GL)
endif()
