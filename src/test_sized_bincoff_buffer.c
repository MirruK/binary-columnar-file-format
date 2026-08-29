#include "sized_bincoff_buffer.h"
#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>

Test(test_sized_bincoff_buffer, init_sized_bincoff_buffer) {
  size_t intended_capacity = 32;
  SizedBincoffBuffer *buf = init_sized_bincoff_buffer(intended_capacity);
  cr_assert(buf->capacity == intended_capacity);
}

Test(test_sized_bincoff_buffer, get_remaining_capacity) {
  size_t initial_capacity = 32;
  SizedBincoffBuffer *buf = init_sized_bincoff_buffer(initial_capacity);
  size_t remaining_capacity = get_remaining_capacity(buf);
  cr_assert(remaining_capacity == initial_capacity);
  size_t item_size = 16;
  void *item = calloc(item_size, 1);
  append(buf, item, item_size);
  remaining_capacity = get_remaining_capacity(buf);
  cr_assert(remaining_capacity == initial_capacity - item_size);
}

Test(test_sized_bincoff_buffer, test_append_item_too_large_is_handled) {
  size_t initial_capacity = 32;
  SizedBincoffBuffer *buf = init_sized_bincoff_buffer(initial_capacity);
  size_t remaining_capacity = get_remaining_capacity(buf);
  cr_assert(remaining_capacity == initial_capacity);
  size_t item_size = 48;
  void *item = calloc(item_size, 1);
  append(buf, item, item_size);
  remaining_capacity = get_remaining_capacity(buf);
  cr_assert(remaining_capacity == initial_capacity - item_size);
}
