cmake_minimum_required(VERSION 3.16)
project(EmergencyResourceAllocator VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(emergency_allocator
    src/main.cpp
)

target_include_directories(emergency_allocator PRIVATE include)

add_executable(emergency_allocator_web
    src/web_main.cpp
)

target_include_directories(emergency_allocator_web PRIVATE include)
target_link_libraries(emergency_allocator_web PRIVATE ws2_32)
