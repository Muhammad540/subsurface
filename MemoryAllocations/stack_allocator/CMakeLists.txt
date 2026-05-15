add_library(StackAllocator src/stack_allocator.cpp)

target_include_directories(StackAllocator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)