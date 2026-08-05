#pragma once

#include "bincoff.h"

BincoffTableMetadata *_parse_metadata_internal(FILE *fp);

size_t _parse_csv_columnar_internal(FILE* fp, char **headers_buffer, SizedBincoffBuffer*** column_buffers_ptr, char *delimiter, enum DataType *schema, size_t fsize);
