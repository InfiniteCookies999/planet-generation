cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 23)

project("planet_generation")

include(FetchContent)

# Tell raylib to select web platform if compiling for web assembly.
if(EMSCRIPTEN)
    set(PLATFORM Web CACHE STRING "" FORCE)
endif()

# Fetch raylib source and build it for the current toolchain.
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)

FetchContent_MakeAvailable(raylib)

add_executable(planet_generation "src/main.cpp" "src/text_3d.cpp" "src/icosphere.cpp"
                                 "src/geometry.cpp" "src/planet_deform.cpp" "src/perlin_noise.cpp"
                                 "src/controlled_camera.cpp")

target_link_libraries(planet_generation raylib)

# Checks if OSX and links appropriate frameworks (only required on MacOS)
if (APPLE)
    target_link_libraries(planet_generation "-framework IOKit")
    target_link_libraries(planet_generation "-framework Cocoa")
    target_link_libraries(planet_generation "-framework OpenGL")
endif()
