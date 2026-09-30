#include "sized_bincoff_buffer.h"
#include <stdlib.h>
#include <string.h>

SizedBincoffBuffer *init_sized_bincoff_buffer(size_t initial_capacity) {
  SizedBincoffBuffer *buf = malloc(sizeof(SizedBincoffBuffer));
  buf->start_ptr = malloc(initial_capacity);
  buf->capacity = initial_capacity;
  buf->head_ptr = buf->start_ptr;
  buf->size = 0;
  buf->element_count = 0;
  return buf;
}

size_t get_remaining_capacity(SizedBincoffBuffer *buf) {
  return buf->capacity - (buf->head_ptr - buf->start_ptr);
}

void _resize_to_fit(SizedBincoffBuffer *buf, size_t item_size) {
  size_t remaining_capacity = get_remaining_capacity(buf);
  ptrdiff_t used_capacity = (buf->head_ptr - buf->start_ptr);
  if (remaining_capacity <= item_size) {
    // relocate start_ptr if realloc moved it
    buf->start_ptr = realloc(buf->start_ptr, used_capacity + item_size + 1);
    // reset the head_ptr to the correct location relative to start_ptr
    buf->head_ptr = buf->start_ptr + used_capacity;
    buf->capacity = used_capacity + item_size + 1;
  }
}

void append(SizedBincoffBuffer *buf, void *item, size_t item_size, int is_concrete) {
  _resize_to_fit(buf, item_size);
  buf->head_ptr = mempcpy(buf->head_ptr, item, item_size);
  // Only increment element_count if the said element is not metadata like string length
  // specifier. This places the burden on the caller to know what they are appending
  if (is_concrete){
    buf->element_count += 1;
  }
}

/** Applies function value_handler to each element in buf */
void apply(SizedBincoffBuffer* buf, void* schema, size_t (*value_handler)(void* item_ptr, void* schema, size_t idx)) {
  char* ptr = (char*)(buf->start_ptr);
  for (size_t i = 0; i < buf->element_count; i++){
    ptr += value_handler(ptr, schema, i);
  }
}
