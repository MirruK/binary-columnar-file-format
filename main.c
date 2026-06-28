#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Higher-level TODOs

// TODO: Metadata for binary file (total bytes, column_number, datatype);
// TODO: Schema from specification file
// TODO: Reading data from file and deserialization
// TODO: Dynamically sized input data buffer for handling large files
// TODO: Columns as separate files
// TODO: Bitmap encoding?
// TODO: Correctness test suite
// TODO: Examples of analytical queries and file sizes
// TODO: Parallelized serializiation, deserialization and analytical queries
// TODO: Conversion back to csv?

// Smaller TODOs

// TODO: document choice of uint32_t as length specifier for strings (long is overkill)
// TODO: change int type on the INTEGER column type to a standardized (4 byte)
// type so it is the same on 32-bit and 64-bit systems (int32_t for instance)

enum DataType { INTEGER, STRING };

// typedef struct {
//   enum DataType* col_types;
//   size_t col_count;
// }Schema;

// Schema* init_schema()

// Serialize value in src as type data_type and write it to dst as bytes, with length prepended
// if it is a string
size_t serialize_and_insert(void **dst, char *src, enum DataType data_type) {
  size_t total_bytes = 0;
  switch (data_type) {
  case INTEGER: {
    int val = atoi(src);
    long int_type_len = sizeof(int);
    printf("int val: %d\n", val);
    memcpy(*dst, &val, sizeof(int));
    *dst += sizeof(int);
    total_bytes = sizeof(long) + sizeof(int);
    break;
  }
  case STRING: {
    size_t length = strlen(src);
    printf("str len: %ld, str val: %s\n", length, src);
    memcpy(*dst, &length, sizeof(uint32_t));
    *dst += sizeof(uint32_t);
    memcpy(*dst, src, length);
    *dst += length;
    total_bytes = sizeof(long) + length;
    break;
  }
  }
  return total_bytes;
}

int deserialize_value(void* value, enum DataType data_type){
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
    char* str = malloc(str_len);
    memcpy(&str_len, value, sizeof(int));
    memcpy(str, value + sizeof(int), str_len);
    printf("%s,", str);
    break;
    }
  }
  return sizeof(int) + str_len;
}

void deserialize_and_print(void* data, enum DataType* schema, int column_count, size_t data_len) {
  void* forward_ptr = data;
  int val_size = 0;
  int col_number = 0;
  for(int i = 0; i < data_len;){
    val_size = deserialize_value(forward_ptr, schema[col_number]);
    forward_ptr += val_size;
    i += val_size;
    col_number = (col_number + 1) % column_count;
  }
}

int main(int argc, char **argv) {
  // get filename & delimiter from argv
  char *filename = "test.csv";
  char *delimiter = ";";
  if (argc > 1) {
    filename = argv[1];
  }
  if (argc > 2) {
    delimiter = argv[2];
  }

  // column 1 => string, column 2 => string, col 3 => integer, col 4 => string
  enum DataType schema[4] = {STRING, STRING, INTEGER, STRING};
  // Read input file
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    perror("Failed to open file");
  }
  // Initialize variables
  char *curr_line, *curr_column, *end;
  size_t count = 0;
  size_t line_len = 0;
  size_t line_bytes_count = 0;
  size_t bytes_serialized = 0;
  int col_num = 0;
  int row_num = 0;
  // prealloc 20mb buffer
  int filesize = 2000000;
  void *arr = malloc(filesize);
  void *forward_ptr = arr;

  // Parse column names
  while ((line_len = getline(&curr_line, &line_bytes_count, fp) != -1)) {
    curr_column = strtok(curr_line, delimiter);
    if (row_num == 0) {
      printf("column #%d name: %s\n", col_num, curr_column);
    }
    else {
      printf("row #%d, column #%d value: %s\n", row_num, col_num,
      curr_column);
      bytes_serialized += serialize_and_insert(&forward_ptr, curr_column, schema[col_num]);
    }
    col_num++;
    while ((curr_column = strtok(NULL, delimiter)) != NULL) {
      if (row_num == 0) {
        printf("column #%d name: %s\n", col_num, curr_column);
      }
      else {
        printf("row #%d, column #%d value: %s\n", row_num, col_num,
        curr_column);
        bytes_serialized += serialize_and_insert(&forward_ptr, curr_column, schema[col_num]);
      }
      col_num++;
    }
    col_num = 0;
    row_num++;
  }
  fclose(fp);
  FILE* outfile = fopen("out.bin", "wb");
  assert(bytes_serialized < filesize);
  fwrite(arr, 1, bytes_serialized, outfile);
  fclose(outfile);
  deserialize_and_print(arr, schema, 4, bytes_serialized);
}
