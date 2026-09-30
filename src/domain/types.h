#pragma once

#include <stdint.h>
#include <string.h>

typedef enum {
#define X(a, b) a,
  #include "data_types.h"
#undef X
} DataType;

static const char *DataTypeStrings[] = {
#define X(a, b) b,
  #include "data_types.h"
#undef X
};


static inline DataType datatype_str_to_enumval(const char *str) {
#define X(a, b) \
  if (strcmp(str, b) == 0) { \
    return a; \
  }
  #include "data_types.h"
#undef X
  return STRING;
}


typedef struct {
  char *table_name;
  uint32_t col_count;
  char **col_names;
  DataType *col_types;
} BincoffTableMetadata;
