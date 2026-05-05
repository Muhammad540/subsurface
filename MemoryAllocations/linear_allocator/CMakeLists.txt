add_library(MemoryAllocations src/linear_allocator.cpp)

target_include_directories(MemoryAllocations
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)