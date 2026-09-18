#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../domain/csv.h"
#include "../domain/metadata.h"
#include "../domain/schema.h"
#include "../domain/serialization.h"
#include "../domain/types.h"
#include "../lib/sized_bincoff_buffer.h"

/* Parse csv file into N column data buffers, laid out in order and pointed to by "buffer" **/
size_t parse_csv_columnar(char* filename, char **headers_buffer, SizedBincoffBuffer*** column_buffers_ptr,
                 char *delimiter, DataType *schema);

void write_metadata(BincoffTableMetadata *metadata, FILE *outfile);

BincoffTableMetadata *parse_metadata(char *dir);
