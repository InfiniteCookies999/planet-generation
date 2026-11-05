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

add_executable(planet_generation "src/main.cpp" "src/text_3d.cpp" "src/icosphere.cpp" "src/uvsphere.cpp"
                                 "src/geometry.cpp" "src/perlin_noise.cpp"
                                 "src/controlled_camera.cpp" "src/star_renderer.cpp" "src/skybox.cpp"
                                 "src/simplex_noise.cpp" "src/planet_renderer.cpp" "src/planet.cpp"
                                 "src/star.cpp" "src/billboard_render.cpp")

target_link_libraries(planet_generation PRIVATE raylib)

# Make sure raylib is not giving warnings.
target_include_directories(planet_generation SYSTEM PRIVATE
    ${raylib_SOURCE_DIR}/src
)

if (MSVC)
    # Suppress warnings when compiling raylib.
    target_compile_options(raylib PRIVATE /W0)

    # warning level 4
    target_compile_options(planet_generation PRIVATE /W4 /wd4100)
else()
    # Suppress warnings when compiling raylib.
    target_compile_options(raylib PRIVATE -w)

    # additional warnings
    target_compile_options(planet_generation PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-but-set-variable
    )
endif()

# Checks if OSX and links appropriate frameworks (only required on MacOS)
if (APPLE)
    target_link_libraries(planet_generation "-framework IOKit")
    target_link_libraries(planet_generation "-framework Cocoa")
    target_link_libraries(planet_generation "-framework OpenGL")
endif()
