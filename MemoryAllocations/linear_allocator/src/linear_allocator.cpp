/*
Move pointers along the memory to allocate (monotonically increase) but you cant free specific chunks, it is freed all at once  

allocation cost: O(1)
*/

#include "linear_allocator.hpp"

bool is_power_of_two(uintptr_t x){
    return (x & (x-1)) == 0;
}

uintptr_t align_forward(uintptr_t ptr, size_t align){
    uintptr_t p, a, modulo;
    assert(is_power_of_two(align));

    p = ptr;
    a = (uintptr_t)align;
    modulo = p & (a-1);

    if (modulo!=0){
        p += a-modulo;
    }
    return p;
}

void *arena_alloc_align(Arena *a, size_t size, size_t align){
    uintptr_t curr_ptr = (uintptr_t)a->buffer + (uintptr_t)a->curr_offset;
    uintptr_t offset   = align_forward(curr_ptr, align);
    offset -= (uintptr_t)a->buffer;

    if (offset+size <= a->buffer_len){
        void *ptr = &a->buffer[offset];
        a->prev_offset = offset;
        a->curr_offset = offset+size;

        memset(ptr, 0, size);
        return ptr;
    }

    // arena out of memory :(
    return NULL;
}

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2*sizeof(void*))
#endif 

void *arena_alloc(Arena *a, size_t size){
    return arena_alloc_align(a, size, DEFAULT_ALIGNMENT);
}

void arena_init(Arena *a, void *backing_buffer, size_t backing_buffer_length){
    a->buffer = (unsigned char*)backing_buffer;
    a->buffer_len = backing_buffer_length;
    a->curr_offset = 0;
    a->prev_offset = 0;
}

void arena_free(Arena *a, void *ptr){
    // :()
}

void *arena_resize_align(Arena *a, void *old_memory, size_t old_size, size_t new_size, size_t align){
    unsigned char* old_mem = (unsigned char*)old_memory;
    assert(is_power_of_two(align));

    if (old_mem == NULL || old_size == 0){
        return arena_alloc_align(a, new_size, align);
    } else if (a->buffer <= old_mem && old_mem < a->buffer+a->buffer_len){
        if (a->buffer+a->prev_offset == old_mem){
            a->curr_offset = a->prev_offset + new_size; // resize in placae
            if (new_size > old_size){
                memset(&a->buffer[a->curr_offset], 0, new_size-old_size);
            }
            return old_memory;
        } else {
            void *new_memory = arena_alloc_align(a, new_size, align);
            size_t copy_size = old_size < new_size ? old_size : new_size;
            memmove(new_memory, old_memory, copy_size);
            return new_memory;
        }
    } else {
        assert(0 && "Memory is out of bounds of the buffer in this arena");
        return NULL;
    }
}

void* arena_resize(Arena* a, void* old_memory, size_t old_size, size_t new_size){
    return arena_resize_align(a, old_memory, old_size, new_size, DEFAULT_ALIGNMENT);
}

void arena_free_all(Arena* a){
    a->curr_offset = 0;
    a->prev_offset = 0;
}


Temp_Arena_Memory temp_arena_memory_begin(Arena *a){
    Temp_Arena_Memory temp;
    temp.arena = a;
    temp.prev_offset = a->prev_offset;
    temp.curr_offset = a->curr_offset;
    return temp;
}

void temp_arena_memory_end(Temp_Arena_Memory temp){
    temp.arena->prev_offset = temp.prev_offset;
    temp.arena->curr_offset = temp.curr_offset;
}