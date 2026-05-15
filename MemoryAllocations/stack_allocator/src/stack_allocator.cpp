#include "stack_allocator.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

/*
- stack buffer: pointer to the start of the raw backing memory block for the entire stack allocator.
- start: the address of the very first byte of the backing buffer.
- end: start + buffer length, the address one byte past the end of the backing buffer.
- stack offset: total bytes consumed from the start of buffer; always points to the byte just after the most recently allocated block (where the next allocation will begin).
- current address: this points to the start of the user's data (after the allocation header).
- header: located at current address - sizeof(Stack_Allocation_Header), immediately before the user's data in memory.
- padding in header: number of extra bytes inserted before the header to satisfy alignment for the user's data block.
- previous offset: current address - padding - start.
- [ PADDING ] [ HEADER ] [ USER DATA ]
*/ 

bool is_power_of_two(uintptr_t x){
    return (x & (x-1)) == 0;
}

void stack_init(Stack *s, void *backing_buffer, size_t backing_buffer_length){
    s->buffer = (unsigned char*)backing_buffer;
    s->buffer_length = backing_buffer_length;
    s->offset = 0;
    s->prev_offset = 0;
}

size_t calculate_padding_with_header(uintptr_t ptr, uintptr_t alignment, size_t header_size){
    uintptr_t p, a, modulo, padding, needed_space;

    assert(is_power_of_two(alignment));

    p = ptr;
    a = alignment;      // multiple of two
    modulo = p & (a-1); // a more effective way to compute (p%a), assuming alignment is a power of two

    padding = 0;
    needed_space = 0;

    if (modulo != 0){
        padding = a-modulo;
    }

    needed_space = (uintptr_t)header_size;

    if (padding < needed_space){
        needed_space -= padding;

        if ((needed_space & (a-1)) != 0){
            padding += a * (1+(needed_space/a));
        } else {
            padding += a * (needed_space/a);
        }
    }

    return (size_t)padding;
}

void* stack_alloc_align(Stack *s, size_t size, size_t alignment){
    uintptr_t current_address, next_address;
    size_t padding;
    Stack_Allocation_Header *header;

    assert(is_power_of_two(alignment));

    current_address = (uintptr_t)s->buffer + (uintptr_t)s->offset;
    padding = calculate_padding_with_header(current_address, (uintptr_t)alignment, sizeof(Stack_Allocation_Header));
    if (s->offset + padding + size > s->buffer_length){
        // stack allocator is out of memory
        return NULL; // <- better to return NULL and user handles this case rather than throwing a bad_alloc exception
    }
    s->prev_offset = s->offset;
    s->offset += padding;

    next_address = current_address + (uintptr_t)padding;
    header = (Stack_Allocation_Header*)(next_address - sizeof(Stack_Allocation_Header));
    header->padding = padding;
    header->prev_offset = s->prev_offset;
    s->offset += size;

    return memset((void*)next_address, 0, size);
}

void* stack_alloc(Stack *s, size_t size){
    return stack_alloc_align(s, size, DEFAULT_ALIGNMENT);
}

void stack_free(Stack *s, void *ptr){
    if (ptr!=NULL){
        uintptr_t start, end, current_address;
        Stack_Allocation_Header *header;
        size_t previous_offset;

        start = (uintptr_t)s->buffer;
        end = start + (uintptr_t)s->buffer_length;
        current_address = (uintptr_t)ptr;

        if (!(start <= current_address && current_address < end)){
            assert(0 && "Out of bounds memory address passed to stack allocator (free)");
            return;
        }

        if (current_address >= start + (uintptr_t)s->offset){
            return;
        }

        header = (Stack_Allocation_Header *)(current_address - sizeof(Stack_Allocation_Header));
        // Calculate previous offset from the header and its address
        previous_offset = (size_t)(current_address - (uintptr_t)header->padding - start);

        if(previous_offset != s->prev_offset){
            assert(0&&"Out of order stack allocator free");
            return;
        }

        s->offset = previous_offset;
        s->prev_offset = header->prev_offset;
    }
}

void *stack_resize_align(Stack *s, void *ptr, size_t old_size, size_t new_size, size_t alignment) {
    if (ptr == NULL){
        return stack_alloc_align(s, new_size, alignment);
    } else if (new_size == 0){
        stack_free(s, ptr);
        return NULL;
    }
    
    uintptr_t start = (uintptr_t)s->buffer;
    uintptr_t end   = start + (uintptr_t)s->buffer_length;
    uintptr_t current_address = (uintptr_t)ptr;

    if (!(start <= current_address && current_address < end)){
        assert(0 && "Out of bounds memory address passed to stack allocator (resize)");
        return NULL;
    }

    if (old_size == new_size){
        return ptr;
    }

    Stack_Allocation_Header *header = (Stack_Allocation_Header *)(current_address - sizeof(Stack_Allocation_Header));
    size_t alloc_offset = (size_t)(current_address - (uintptr_t)header->padding - start);

    if (alloc_offset == s->prev_offset){
        size_t new_offset = alloc_offset + header->padding + new_size;

        if (new_offset > s->buffer_length){
            return NULL;
        }

        s->offset = new_offset;
        return ptr;
    }

    void* new_ptr = stack_alloc_align(s, new_size, alignment);
    if(new_ptr!=NULL){
        size_t min_size = old_size < new_size ? old_size : new_size;
        memmove(new_ptr, ptr, min_size);
    }
    return new_ptr;
}

void *stack_resize(Stack *s, void *ptr, size_t old_size, size_t new_size) {
	return stack_resize_align(s, ptr, old_size, new_size, DEFAULT_ALIGNMENT);
}

void stack_free_all(Stack *s) {
    s->offset = 0;
}