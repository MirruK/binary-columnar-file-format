#pragma once

#include <stddef.h>

typedef struct {
 void* start_ptr;
 void* head_ptr;
 // The allocated capacity of this buffer in bytes,
 // writing (char*)(ptr)[capacity] is an out of bounds write
 // writing (char*)(ptr)[capacity-1] is not
 size_t capacity;
 // The total size in bytes that all currently present elements use
 size_t size;
 // Effective number of "elements"
 size_t element_count;
} SizedBincoffBuffer;

SizedBincoffBuffer* init_sized_bincoff_buffer(size_t initial_capacity);

size_t get_remaining_capacity(SizedBincoffBuffer* buf);

void _resize_to_fit(SizedBincoffBuffer* buf, size_t item_size);

void append(SizedBincoffBuffer* buf, void* item, size_t item_size);
