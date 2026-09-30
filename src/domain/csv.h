#pragma once

#include <stdio.h>

#include "../lib/sized_bincoff_buffer.h"
#include "types.h"

size_t parse_csv(char *filename, char **headers_buffer, void *buffer,
                 char *delimiter, DataType *schema);

size_t _parse_csv_columnar_internal(FILE* fp, char **headers_buffer, SizedBincoffBuffer*** column_buffers_ptr, char *delimiter, DataType *schema, size_t fsize);
