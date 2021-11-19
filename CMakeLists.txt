cmake_minimum_required(VERSION 3.10)

# set the project name
project(kaleidoscope VERSION 1.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set(WITH_SIMD FALSE CACHE BOOLEAN "Use SIMD extensions" FORCE)
add_subdirectory(libjpeg-turbo)

# add the executable
add_executable(kaleidoscope
    main.cpp
    util.h
    rgb_image.h
    rgb_image.cpp
    rgb_pixel.h
    kaleidoscope.h
    kaleidoscope.cpp)

target_include_directories(kaleidoscope PUBLIC
    ./libjpeg-turbo
    ${PROJECT_BINARY_DIR}/libjpeg-turbo)

target_link_libraries(kaleidoscope PUBLIC turbojpeg)
