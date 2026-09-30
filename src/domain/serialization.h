#pragma once

#include <stdlib.h>

#include "../lib/sized_bincoff_buffer.h"
#include "types.h"

/* Serialize value in src as type data_type and write it to dst as bytes, with
 length prepended if it is a string **/
size_t serialize_and_insert(void **dst, char *src, DataType data_type);

size_t serialize_and_append(SizedBincoffBuffer *buf, char *src,
                            DataType data_type);

size_t deserialize_value(void *value, DataType data_type);

void deserialize_and_print(void *data, BincoffTableMetadata *metadata,
                           size_t data_len);
