#pragma once

#include <stdio.h>

#include "types.h"

size_t parse_schema(FILE *fp, DataType** schema_ptr);

