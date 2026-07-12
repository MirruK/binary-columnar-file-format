#include "bincoff.h"
#include <stdlib.h>
#include <string.h>

enum DataType datatype_str_to_enumval(const char* str) {
  if(strcmp(str, "INTEGER") == 0){
    return INTEGER;
  }
  if(strcmp(str, "STRING") == 0){
    return STRING;
  }
    return STRING;
}

enum DataType* parse_schema(char* filename){
  FILE* fp = fopen(filename, "r");
  fseek(fp, 0L, SEEK_END);
  size_t size = ftell(fp);
  rewind(fp);
  char* buf = malloc(size);
  getline(&buf, &size, fp);
  STRIP_NEWLINE(buf);
  char* curr;
  int i = 0;
  enum DataType* schema = malloc(sizeof(enum DataType) * 128);
  curr = strtok(buf,";");
  schema[i++] = datatype_str_to_enumval(curr);
  while((curr = strtok(NULL, ";")) != NULL){
    schema[i++] = datatype_str_to_enumval(curr);
  }
  return schema;
}

/* Caller is responsible for ensuring validity and lifetime
 of memory behind any of the addresses passed into this function **/
BincoffTableMetadata* init_table_metadata(char* table_name, uint32_t col_count, char** col_names, enum DataType* col_types){
  BincoffTableMetadata* metadata = malloc(sizeof(BincoffTableMetadata));
  metadata->table_name = table_name;
  metadata->col_count = col_count;
  metadata->col_names = col_names;
  metadata->col_types = col_types;
  return metadata;
}

/* Serialize value in src as type data_type and write it to dst as bytes, with length prepended
 if it is a string **/
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

size_t deserialize_value(void* value, enum DataType data_type){
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
    char* str = malloc(str_len+1);
    memcpy(str, value + sizeof(int), str_len);
    // This value is only written in order to print, it is not part of the actual data
    str[str_len] = '\0';
    printf("%s,", str);
    break;
    }
  }
  return sizeof(int) + str_len;
}

void deserialize_and_print(void* data, BincoffTableMetadata* metadata, size_t data_len) {
  void* forward_ptr = data;
  int val_size = 0;
  int col_number = 0;
  for(int i = 0; i < data_len;){
    val_size = deserialize_value(forward_ptr, metadata->col_types[col_number]);
    forward_ptr += val_size;
    i += val_size;
    col_number = (col_number + 1) % metadata->col_count;
  }
}

size_t parse_csv(char* filename, char** headers_buffer, void *buffer, char* delimiter, enum DataType* schema){  
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
      char* h = (char*)malloc(l);
      strcpy(h, curr_column);
      headers_buffer[col_num] = h;
    }
    else {
      // printf("row #%d, column #%d value: %s\n", row_num, col_num,
      // curr_column);
      bytes_serialized += serialize_and_insert(&forward_ptr, curr_column, schema[col_num]);
    }
    col_num++;
    while ((curr_column = strtok(NULL, delimiter)) != NULL) {
      if (row_num == 0) {
        // printf("column #%d name: %s\n", col_num, curr_column);
        // curr_column[strcspn(curr_column, "\n")] = '\0';
        STRIP_NEWLINE(curr_column);
        int l = strlen(curr_column);
        char* h = (char*)malloc(l);
        strcpy(h, curr_column);
        headers_buffer[col_num] = h;
      }
      else {
        // printf("row #%d, column #%d value: %s\n", row_num, col_num,
        // curr_column);
        bytes_serialized += serialize_and_insert(&forward_ptr, curr_column, schema[col_num]);
      }
      col_num++;
    }
    col_num = 0;
    row_num++;
  }
  fclose(fp);
  return bytes_serialized;
}

void write_metadata(BincoffTableMetadata* metadata, FILE* outfile) {
  fprintf(outfile, "%s\n", metadata->table_name);
  fprintf(outfile, "%d\n", metadata->col_count);
  for (int i = 0; i < metadata->col_count; i++){
    fprintf(outfile, "%s;%d\n", metadata->col_names[i], metadata->col_types[i]);
  }
}

BincoffTableMetadata* parse_metadata(char* dir){
  int dirname_size = strlen(dir);
  char m[] = "/metadata";
  char* metadata_path = malloc(dirname_size + sizeof(m));
  strcpy(metadata_path, dir);
  char* filename = strcat(metadata_path, m);
  FILE* fp = fopen(metadata_path, "rb");
  if (fp == NULL){
    perror("dunno what happened but metadata file could not be loaded");
    exit(1);
  }
  size_t curr_line_size = 32;
  int line_len;
  // get line containing table name
  char* table_name = malloc(curr_line_size);
  line_len = getline(&table_name, &curr_line_size, fp);
  STRIP_NEWLINE(table_name);
  char* col_count_str = malloc(curr_line_size);
  line_len = getline(&col_count_str, &curr_line_size, fp);
  STRIP_NEWLINE(col_count_str);
  int col_count = atoi(col_count_str);
  int i = 0;
  char* curr_column;
  char* curr_line;
  curr_line_size = 0;
  char** column_names = malloc(sizeof(char*)*col_count);
  enum DataType* schema = malloc(sizeof(enum DataType)*col_count);
  for(i = 0; i < col_count && (line_len = getline(&curr_line, &curr_line_size, fp) != -1); i++){
    curr_column = strtok(curr_line, ";");
    column_names[i] = malloc(strlen(curr_column));
    strcpy(column_names[i], curr_column);
    curr_column = strtok(NULL, ";");
    STRIP_NEWLINE(curr_column);
    schema[i] = atoi(curr_column);
  }
  if (i != col_count - 1) {
    printf("Found mismatch between col_count field and actual number of columns listed in metadata\n");
  }
  BincoffTableMetadata* metadata = init_table_metadata(table_name, col_count, column_names, schema);
  return metadata;
}
