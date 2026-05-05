#pragma once

#include <cstdint>
#include <stddef.h>
#include <cassert>
#include <cstring>

struct Arena{
    unsigned char *buffer;
    size_t  buffer_len;
    size_t  prev_offset;
    size_t  curr_offset;
};

bool is_power_of_two(uintptr_t x);
uintptr_t align_forward(uintptr_t ptr, size_t align);
void *arena_alloc_align(Arena *a, size_t size, size_t align);

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2*sizeof(void*))
#endif 

void *arena_alloc(Arena *a, size_t size);
void arena_init(Arena *a, void *backing_buffer, size_t backing_buffer_length);
void arena_free(Arena *a, void *ptr);
void *arena_resize_align(Arena *a, void *old_memory, size_t old_size, size_t new_size, size_t align);
void* arena_resize(Arena* a, void* old_memory, size_t old_size, size_t new_size);
void arena_free_all(Arena* a);
struct Temp_Arena_Memory{
    Arena* arena;
    size_t prev_offset;
    size_t curr_offset;
};
Temp_Arena_Memory temp_arena_memory_begin(Arena *a);
void temp_arena_memory_end(Temp_Arena_Memory temp);