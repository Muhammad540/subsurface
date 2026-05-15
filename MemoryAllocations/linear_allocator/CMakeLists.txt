add_library(LinearAllocator src/linear_allocator.cpp)

target_include_directories(LinearAllocator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)