#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/debug_macro.h"
#include "common.h"
#include "serialization.h"

/* Serialize value in src as type data_type and write it to dst as bytes, with
 length prepended if it is a string **/
size_t serialize_and_insert(void **dst, char *src, DataType data_type) {
  size_t total_bytes = 0;
  switch (data_type) {
  case INTEGER: {
    int val = atoi(src);
    // printf("int val: %d\n", val);
    memcpy(*dst, &val, sizeof(int));
    *dst += sizeof(int);
    total_bytes = sizeof(int);
    break;
  }
  case STRING: {
    size_t length = strlen(src);
    // printf("str len: %ld, str val: %s\n", length, src);
    memcpy(*dst, &length, sizeof(uint32_t));
    *dst += sizeof(uint32_t);
    memcpy(*dst, src, length);
    *dst += length;
    total_bytes = sizeof(uint32_t) + length;
    break;
  }
  }
  return total_bytes;
}

size_t serialize_and_append(SizedBincoffBuffer *buf, char *src,
                            DataType data_type) {
  size_t total_bytes = 0;
  switch (data_type) {
  case INTEGER: {
    int val = atoi(src);
    debug_log("int val: %d\n", val);
    append(buf, &val, sizeof(int), 1);
    total_bytes = sizeof(int);
    break;
  }
  case STRING: {
    size_t length = strlen(src);
    debug_log("str len: %ld, str val: %s\n", length, src);
    append(buf, &length, sizeof(uint32_t), 0);
    append(buf, src, length, 1);
    total_bytes = sizeof(uint32_t) + length;
    break;
  }
  }
  return total_bytes;
}

size_t deserialize_value(void *value, DataType data_type) {
  int str_len = 0;
  switch (data_type) {
  case INTEGER: {
    int val = 0;
    memcpy(&val, value, sizeof(int));
    printf("%d,", val);
    break;
  }
  case STRING: {
    str_len = 0;
    memcpy(&str_len, value, sizeof(int));
    char *str = malloc(str_len + 1);
    memcpy(str, value + sizeof(int), str_len);
    // This value is only written in order to print, it is not part of the
    // actual data
    str[str_len] = '\0';
    printf("%s,", str);
    break;
  }
  }
  return sizeof(int) + str_len;
}

void deserialize_and_print(void *data, BincoffTableMetadata *metadata,
                           size_t data_len) {
  void *forward_ptr = data;
  int val_size = 0;
  int col_number = 0;
  for (size_t i = 0; i < data_len;) {
    val_size = deserialize_value(forward_ptr, metadata->col_types[col_number]);
    forward_ptr += val_size;
    i += val_size;
    col_number = (col_number + 1) % metadata->col_count;
  }
}

