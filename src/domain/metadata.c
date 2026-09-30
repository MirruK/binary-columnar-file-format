#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "metadata.h"

/* Caller is responsible for ensuring validity and lifetime
 of memory behind any of the addresses passed into this function **/
BincoffTableMetadata *init_table_metadata(char *table_name, uint32_t col_count,
                                          char **col_names,
                                          DataType *col_types) {
  BincoffTableMetadata *metadata = malloc(sizeof(BincoffTableMetadata));
  metadata->table_name = table_name;
  metadata->col_count = col_count;
  metadata->col_names = col_names;
  metadata->col_types = col_types;
  return metadata;
}

BincoffTableMetadata *_parse_metadata_internal(FILE *fp) {
  size_t curr_line_size = 0;
  int line_len;
  char *table_name;
  // get line containing table name
  line_len = getline(&table_name, &curr_line_size, fp);
  // replaces newline with null byte
  STRIP_NEWLINE(table_name);
  char *col_count_str;
  curr_line_size = 0;
  // same procedure but for the number of columns of the stored data
  line_len = getline(&col_count_str, &curr_line_size, fp);
  STRIP_NEWLINE(col_count_str);
  int col_count = atoi(col_count_str);
  int i = 0;
  char *curr_column;
  char *curr_line;
  curr_line_size = 0;
  char **column_names = malloc(sizeof(char *) * col_count);
  DataType *schema = malloc(sizeof(DataType) * col_count);
  for (i = 0; i < col_count &&
              (line_len = getline(&curr_line, &curr_line_size, fp) != -1);
       i++) {
    curr_column = strtok(curr_line, ";");
    column_names[i] = (char *)malloc(strlen(curr_column) + 1);
    strcpy(column_names[i], curr_column);
    curr_column = strtok(NULL, ";");
    STRIP_NEWLINE(curr_column);
    schema[i] = atoi(curr_column);
  }
  if (i != col_count) {
    printf("Found mismatch between col_count field and actual number of "
           "columns listed in metadata\n, col_count = %d, actual count = %d\n",
           col_count, i);
  }
  BincoffTableMetadata *metadata =
      init_table_metadata(table_name, col_count, column_names, schema);
  return metadata;
}
