#pragma once

#include <stdio.h>

#include "types.h"

/* Caller is responsible for ensuring validity and lifetime
 of memory behind any of the addresses passed into this function **/
BincoffTableMetadata *init_table_metadata(char *table_name, uint32_t col_count,
                                          char **col_names,
                                          DataType *col_types);

BincoffTableMetadata *_parse_metadata_internal(FILE *fp);


