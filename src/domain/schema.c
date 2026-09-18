#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/debug_macro.h"
#include "common.h"
#include "schema.h"

size_t parse_schema(FILE *fp, DataType **schema_ptr) {
  fseek(fp, 0L, SEEK_END);
  size_t size = ftell(fp);
  rewind(fp);
  char *buf = malloc(size);
  getline(&buf, &size, fp);
  STRIP_NEWLINE(buf);
  char *curr;
  int i = 0;
  DataType dt;
  DataType *schema = malloc(sizeof(DataType) * 128);
  curr = strtok(buf, ";");
  schema[i++] = datatype_str_to_enumval(curr);
  while ((curr = strtok(NULL, ";")) != NULL) {
    dt = datatype_str_to_enumval(curr);
    debug_log("parsed datatype: %d\n", dt);
    schema[i++] = dt;
  }
  *schema_ptr = schema;
  return i;
}

