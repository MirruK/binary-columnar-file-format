#include "bincoff.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

enum DataType datatype_str_to_enumval(const char *str) {
  if (strcmp(str, "INTEGER") == 0) {
    return INTEGER;
  }
  if (strcmp(str, "STRING") == 0) {
    return STRING;
  }
  return STRING;
}

size_t parse_schema(FILE *fp, enum DataType** schema_ptr) {
  fseek(fp, 0L, SEEK_END);
  size_t size = ftell(fp);
  rewind(fp);
  char *buf = malloc(size);
  getline(&buf, &size, fp);
  STRIP_NEWLINE(buf);
  char *curr;
  int i = 0;
  enum DataType dt;
  enum DataType *schema = malloc(sizeof(enum DataType) * 128);
  curr = strtok(buf, ";");
  schema[i++] = datatype_str_to_enumval(curr);
  while ((curr = strtok(NULL, ";")) != NULL) {
    dt = datatype_str_to_enumval(curr);
    printf("parsed datatype: %d\n", dt);
    schema[i++] = dt;
  }
  *schema_ptr = schema;
  return i;
}

/* Caller is responsible for ensuring validity and lifetime
 of memory behind any of the addresses passed into this function **/
BincoffTableMetadata *init_table_metadata(char *table_name, uint32_t col_count,
                                          char **col_names,
                                          enum DataType *col_types) {
  BincoffTableMetadata *metadata = malloc(sizeof(BincoffTableMetadata));
  metadata->table_name = table_name;
  metadata->col_count = col_count;
  metadata->col_names = col_names;
  metadata->col_types = col_types;
  return metadata;
}

/* Serialize value in src as type data_type and write it to dst as bytes, with
 length prepended if it is a string **/
size_t serialize_and_insert(void **dst, char *src, enum DataType data_type) {
  size_t total_bytes = 0;
  switch (data_type) {
  case INTEGER: {
    int val = atoi(src);
    // printf("int val: %d\n", val);
    memcpy(*dst, &val, sizeof(int));
    *dst += sizeof(int);
    total_bytes = sizeof(int);
    break;
  }
  case STRING: {
    size_t length = strlen(src);
    // printf("str len: %ld, str val: %s\n", length, src);
    memcpy(*dst, &length, sizeof(uint32_t));
    *dst += sizeof(uint32_t);
    memcpy(*dst, src, length);
    *dst += length;
    total_bytes = sizeof(uint32_t) + length;
    break;
  }
  }
  return total_bytes;
}

size_t serialize_and_append(SizedBincoffBuffer *buf, char *src,
                            enum DataType data_type) {
  size_t total_bytes = 0;
  switch (data_type) {
  case INTEGER: {
    int val = atoi(src);
    // printf("int val: %d\n", val);
    append(buf, &val, sizeof(int));
    total_bytes = sizeof(int);
    break;
  }
  case STRING: {
    size_t length = strlen(src);
    // printf("str len: %ld, str val: %s\n", length, src);
    append(buf, &length, sizeof(uint32_t));
    append(buf, src, length);
    total_bytes = sizeof(uint32_t) + length;
    break;
  }
  }
  return total_bytes;
}

size_t deserialize_value(void *value, enum DataType data_type) {
  int str_len = 0;
  switch (data_type) {
  case INTEGER: {
    int val = 0;
    memcpy(&val, value, sizeof(int));
    printf("%d,", val);
    break;
  }
  case STRING: {
    str_len = 0;
    memcpy(&str_len, value, sizeof(int));
    char *str = malloc(str_len + 1);
    memcpy(str, value + sizeof(int), str_len);
    // This value is only written in order to print, it is not part of the
    // actual data
    str[str_len] = '\0';
    printf("%s,", str);
    break;
  }
  }
  return sizeof(int) + str_len;
}

void deserialize_and_print(void *data, BincoffTableMetadata *metadata,
                           size_t data_len) {
  void *forward_ptr = data;
  int val_size = 0;
  int col_number = 0;
  for (int i = 0; i < data_len;) {
    val_size = deserialize_value(forward_ptr, metadata->col_types[col_number]);
    forward_ptr += val_size;
    i += val_size;
    col_number = (col_number + 1) % metadata->col_count;
  }
}

size_t parse_csv(char *filename, char **headers_buffer, void *buffer,
                 char *delimiter, enum DataType *schema) {
  // Initialize variables
  char *curr_line, *curr_column, *end;
  size_t count = 0;
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
                                    char *delimiter, enum DataType *schema,
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
  printf("column count: %d\n", column_count);

  // 2. Pre-allocate buffers for all N columns (using file size / N heuristic)
  size_t column_size_estimate = fsize / column_count;
  *column_buffers_ptr = malloc(sizeof(SizedBincoffBuffer *) * column_count);
  SizedBincoffBuffer** column_buffers = *column_buffers_ptr;

  int i = 0;
  char *curr_column;
  char *curr_line;
  size_t curr_line_size = 0;
  size_t line_bytes_count = 0;
  size_t line_len = 0;
  size_t row_num = 0;
  size_t col_num = 0;

  for (i = 0; i < column_count; i++){
    column_buffers[i] = init_sized_bincoff_buffer(column_size_estimate);
  }
  // 4. For each row
  while ((line_len = getline(&curr_line, &line_bytes_count, fp) != -1)) {
    if (strcmp(curr_line, "") == 0) {break;}
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
    printf("got to %d col_num\n", col_num);
    col_num = 0;
    row_num++;
  }
  // 8. Return number of columns, sized column data is available in
  // "column_buffers"
  return column_count;
}

size_t parse_csv_columnar(char *filename, char **headers_buffer,
                          SizedBincoffBuffer ***column_buffers_ptr, char *delimiter,
                          enum DataType *schema) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    perror("Failed to open file");
    return 0;
  }

  struct stat st;
  stat(filename, &st);
  size_t filesize_total = st.st_size;

  return _parse_csv_columnar_internal(fp, headers_buffer, column_buffers_ptr,
                                      delimiter, schema, filesize_total);
}

void write_metadata(BincoffTableMetadata *metadata, FILE *outfile) {
  fprintf(outfile, "%s\n", metadata->table_name);
  fprintf(outfile, "%d\n", metadata->col_count);
  for (int i = 0; i < metadata->col_count; i++) {
    fprintf(outfile, "%s;%d\n", metadata->col_names[i], metadata->col_types[i]);
  }
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
  enum DataType *schema = malloc(sizeof(enum DataType) * col_count);
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

BincoffTableMetadata *parse_metadata(char *dir) {
  int dirname_size = strlen(dir);
  char m[] = "/metadata";
  char *metadata_path = malloc(dirname_size + sizeof(m));
  strcpy(metadata_path, dir);
  char *filename = strcat(metadata_path, m);
  FILE *fp = fopen(metadata_path, "rb");
  if (fp == NULL) {
    perror("dunno what happened but metadata file could not be loaded");
    exit(1);
  }
  return _parse_metadata_internal(fp);
}
