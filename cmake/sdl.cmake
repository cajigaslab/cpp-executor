FetchContent_Declare(
  sdl
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.2.10
  SOURCE_SUBDIR thalamus-nonexistant)
FetchContent_MakeAvailable(sdl)
file(MAKE_DIRECTORY ${sdl_BINARY_DIR}/Debug)
file(MAKE_DIRECTORY ${sdl_BINARY_DIR}/Release)

if(WIN32)
  set(SDL_LIB_FILES "${sdl_BINARY_DIR}/$<CONFIG>/install/lib/SDL3-static.lib")
else()
  set(SDL_LIB_FILES "${sdl_BINARY_DIR}/$<CONFIG>/install/lib/libSDL3.a")
endif()

add_custom_command(DEPENDS "${sdl_SOURCE_DIR}/CMakeLists.txt"
                   OUTPUT "${sdl_BINARY_DIR}/$<CONFIG>/CMakeCache.txt"
                   COMMAND 
                   cmake "${sdl_SOURCE_DIR}" -Wno-dev 
		                  -DSDL_LIBSAMPLERATE=OFF
		                  -DSDL_SNDIO=OFF
                      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                      -DCMAKE_LINKER=${CMAKE_LINKER}
                      -DBUILD_SHARED_LIBS=OFF
                      "-DSDL_CMAKE_DEBUG_POSTFIX=\"\""
                      "-DCMAKE_CXX_FLAGS=${ALL_COMPILE_OPTIONS_SPACED}" 
                      "-DCMAKE_C_FLAGS=${ALL_COMPILE_OPTIONS_SPACED}"
                      "-DCMAKE_BUILD_TYPE=$<CONFIG>" "-DCMAKE_INSTALL_PREFIX=${sdl_BINARY_DIR}/$<CONFIG>/install"
                      -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
                      -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
                      -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}
                      -DCMAKE_MSVC_RUNTIME_LIBRARY_DEFAULT=${CMAKE_MSVC_RUNTIME_LIBRARY}
                      -G "${CMAKE_GENERATOR}"
                   && cmake -E touch_nocreate "${sdl_BINARY_DIR}/$<CONFIG>/CMakeCache.txt"
                   WORKING_DIRECTORY "${sdl_BINARY_DIR}/$<CONFIG>")
add_custom_command(OUTPUT ${SDL_LIB_FILES}
                   DEPENDS "${sdl_BINARY_DIR}/$<CONFIG>/CMakeCache.txt"
                   COMMAND 
                   echo SDL BUILD
                   && cmake --build . --config "$<CONFIG>" --parallel ${CPU_COUNT}
                   && cmake --install . --config "$<CONFIG>"
                   && cmake -E touch_nocreate ${SDL_LIB_FILES}
                   WORKING_DIRECTORY ${sdl_BINARY_DIR}/$<CONFIG>)
      
set(SDL_INCLUDE "${sdl_BINARY_DIR}/$<CONFIG>/install/include")
set(SDL_PKG_CONFIG_DIR "${sdl_BINARY_DIR}/$<CONFIG>/install/lib/pkgconfig")

add_library(sdl INTERFACE ${SDL_LIB_FILES})
target_link_libraries(sdl INTERFACE ${SDL_LIB_FILES})
if(WIN32)
  target_link_libraries(sdl INTERFACE User32.lib Gdi32.lib Setupapi.lib Advapi32.lib Imm32.lib Winmm.lib Shell32.lib Ole32.lib Oleaut32.lib Version.lib Shlwapi.lib Vfw32.lib)
elseif(APPLE)
  find_library(CORE_AUDIO CoreAudio)
  find_library(CORE_IMAGE CoreImage)
  find_library(CORE_HAPTICS CoreHaptics)
  find_library(CORE_GRAPHICS CoreGraphics)
  find_library(FORCE_FEEDBACK ForceFeedback)
  find_library(GAME_CONTROLLER GameController)
  find_library(IO_KIT IOKit)
  find_library(APP_KIT AppKit)
  find_library(SECURITY Security)
  find_library(OPENGL OpenGL)
  find_library(CARBON Carbon)
  find_library(METAL Metal)
  target_link_libraries(sdl INTERFACE ${CORE_AUDIO} ${CORE_IMAGE} ${CORE_HAPTICS} ${CORE_GRAPHICS} ${FORCE_FEEDBACK}
    ${GAME_CONTROLLER} ${IO_KIT} ${APP_KIT} ${SECURITY} ${OPENGL} ${CARBON} ${METAL})
else()
  target_link_libraries(sdl INTERFACE xcb xcb-shm xcb-xfixes xcb-shape dl)
endif()
target_include_directories(sdl INTERFACE "${SDL_INCLUDE}")

