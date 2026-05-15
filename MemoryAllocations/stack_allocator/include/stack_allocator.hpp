#pragma once

/* memory is managed in LIFO fashion and the memory can be freed on per-allocation basis */
#include <stdio.h>
#include <stdint.h>
#include <cstdint>

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2*sizeof(void*))
#endif

struct Stack{
    unsigned char* buffer;
    size_t buffer_length;
    size_t prev_offset;
    size_t offset;
};

struct Stack_Allocation_Header{
    /*  this padding specifies the amount of data stored before this allocation for it to be aligned correctly
        with the old allocation
    */
    size_t padding; // max alignment in bytes = 2^(8*sizeof(padding)-1)
    size_t prev_offset; 
};

//---------- helpers ----------
bool is_power_of_two(uintptr_t x);

/* computes how much padding is needed to accomodate the header and align the next bytes of user data */
size_t calculate_padding_with_header(uintptr_t ptr, uintptr_t alignment, size_t header_size);

void* stack_alloc_align(Stack *s, size_t size, size_t alignment);


// ------------------------------ 
/* initializes the parameters for the given stack */
void stack_init(Stack *s, void *backing_buffer, size_t backing_buffer_length);

/* increments the stack offset to indicate the current buffer offset while also accounting for allocation header*/
void* stack_alloc(Stack *s, size_t size);

/* frees the memory passed to it and decrements the offset to free that memory */
void stack_free(Stack *s, void *ptr);

/* reallocate new memory if there needs to be a change in allocation size */
void *stack_resize_align(Stack *s, void *ptr, size_t old_size, size_t new_size, size_t alignment);

/* free all memory from within the stack allocator by setting the buffer offsets to zero */
void stack_free_all(Stack* s);