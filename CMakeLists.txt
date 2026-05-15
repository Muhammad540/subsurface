cmake_minimum_required(VERSION 3.20)
project(subsurface LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/FetchRaylib.cmake)

# Algorithms
add_subdirectory(MemoryAllocations/linear_allocator)
add_subdirectory(MemoryAllocations/stack_allocator)

# Application
add_subdirectory(application/linear_allocator_app)