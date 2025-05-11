cmake_minimum_required(VERSION 3.10)
project(sdl2_example)

set(CMAKE_CXX_STANDARD 17)

# Phương pháp 1: Sử dụng find_package trực tiếp
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Thêm đường dẫn đến headers của SDL2_image và SDL2_ttf
include_directories(/usr/include/SDL2)

add_executable(sdl2_example main.cpp)

# Liên kết cả ba thư viện
target_link_libraries(sdl2_example 
    ${SDL2_LIBRARIES}
    SDL2_image
    SDL2_ttf
)