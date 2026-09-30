#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "csv.h"
#include "serialization.h"

size_t parse_csv(char *filename, char **headers_buffer, void *buffer,
                 char *delimiter, DataType *schema) {
  // Initialize variables
  char *curr_line, *curr_column;
  size_t line_len = 0;
  size_t line_bytes_count = 0;
  size_t bytes_serialized = 0;
  int col_num = 0;
  int row_num = 0;
  void *forward_ptr = buffer;
  // Read input file
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    perror("Failed to open file");
  }

  // Parse column names
  while ((line_len = getline(&curr_line, &line_bytes_count, fp) != -1)) {
    curr_column = strtok(curr_line, delimiter);
    if (row_num == 0) {
      // printf("column #%d name: %s\n", col_num, curr_column);
      // curr_column[strcspn(curr_column, "\n")] = '\0';
      STRIP_NEWLINE(curr_column);
      int l = strlen(curr_column);
      char *h = (char *)malloc(l);
      strcpy(h, curr_column);
      headers_buffer[col_num] = h;
    } else {
      // printf("row #%d, column #%d value: %s\n", row_num, col_num,
      // curr_column);
      bytes_serialized +=
          serialize_and_insert(&forward_ptr, curr_column, schema[col_num]);
    }
    col_num++;
    while ((curr_column = strtok(NULL, delimiter)) != NULL) {
      if (row_num == 0) {
        // printf("column #%d name: %s\n", col_num, curr_column);
        // curr_column[strcspn(curr_column, "\n")] = '\0';
        STRIP_NEWLINE(curr_column);
        int l = strlen(curr_column);
        char *h = (char *)malloc(l);
        strcpy(h, curr_column);
        headers_buffer[col_num] = h;
      } else {
        // printf("row #%d, column #%d value: %s\n", row_num, col_num,
        // curr_column);
        bytes_serialized +=
            serialize_and_insert(&forward_ptr, curr_column, schema[col_num]);
      }
      col_num++;
    }
    col_num = 0;
    row_num++;
  }
  fclose(fp);
  return bytes_serialized;
}

size_t _parse_csv_columnar_internal(FILE *fp, char **headers_buffer,
                                    SizedBincoffBuffer ***column_buffers_ptr,
                                    char *delimiter, DataType *schema,
                                    size_t fsize) {
  // 1. Lookahead to see how many headers there are (the rest of the function
  // will assume that number of columns per row)
  size_t headers_string_size = 32;
  char *headers_string = malloc(headers_string_size);
  getline(&headers_string, &headers_string_size, fp);
  char *curr_header;
  size_t column_count = 0;
  curr_header = strtok(headers_string, delimiter);
  STRIP_NEWLINE(curr_header);
  headers_buffer[column_count] = malloc(strlen(curr_header) + 1);
  strcpy(headers_buffer[column_count++], curr_header);
  while ((curr_header = strtok(NULL, delimiter)) != NULL) {
    STRIP_NEWLINE(curr_header);
    headers_buffer[column_count] = malloc(strlen(curr_header) + 1);
    strcpy(headers_buffer[column_count++], curr_header);
  }
  printf("column count: %zu\n", column_count);

  // 2. Pre-allocate buffers for all N columns (using file size / N heuristic)
  size_t column_size_estimate = fsize / column_count;
  *column_buffers_ptr = malloc(sizeof(SizedBincoffBuffer *) * column_count);
  SizedBincoffBuffer **column_buffers = *column_buffers_ptr;

  size_t i = 0;
  char *curr_column;
  char *curr_line;
  size_t line_bytes_count = 0;
  size_t line_len = 0;
  // size_t row_num = 0;
  size_t col_num = 0;

  for (i = 0; i < column_count; i++) {
    column_buffers[i] = init_sized_bincoff_buffer(column_size_estimate);
  }
  // 4. For each row
  while ((line_len = getline(&curr_line, &line_bytes_count, fp) != -1)) {
    if (strcmp(curr_line, "") == 0) {
      break;
    }
    // 5. For column in row:
    curr_column = strtok(curr_line, delimiter);
    column_buffers[col_num]->size += serialize_and_append(
        column_buffers[col_num], curr_column, schema[col_num]);
    col_num++;
    while ((curr_column = strtok(NULL, delimiter)) != NULL) {
      column_buffers[col_num]->size += serialize_and_append(
          column_buffers[col_num], curr_column, schema[col_num]);
      col_num++;
    }
    printf("got to %zu col_num\n", col_num);
    col_num = 0;
    // row_num++;
  }
  // 8. Return number of columns, sized column data is available in
  // "column_buffers"
  return column_count;
}
