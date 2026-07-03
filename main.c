#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


// Higher-level TODOs

// TODO: Metadata for binary file (total bytes, column_number, datatype);
// TODO: Schema from specification file
// TODO: Reading data from file and deserialization
// [x] TODO: Dynamically sized input data buffer for handling large files
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

typedef struct {
  // Max size of a multi-char delimiter is 8 characters
  char delimiter[8];
  size_t data_size;
  enum DataType data_type;
}BincoffColumnFileHeaders;

typedef struct {
  char* table_name;
  uint32_t col_count;
  char** col_names;
  enum DataType* col_types;
}BincoffTableMetadata;

// Schema* init_schema()

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

// Serialize value in src as type data_type and write it to dst as bytes, with length prepended
// if it is a string
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
    // printf("%d,", val);
    break;
    }
  case STRING: {
    str_len = 0;
    memcpy(&str_len, value, sizeof(int));
    char* str = malloc(str_len+1);
    memcpy(str, value + sizeof(int), str_len);
    // This value is only written in order to print, it is not part of the actual data
    str[str_len] = '\0';
    // printf("%s,", str);
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
    fprintf(outfile, "%s;", metadata->col_names[i]);
    fprintf(outfile, "%d\n", metadata->col_types[i]);
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
  char* curr_line;
  size_t curr_line_size;
  int line_len;
  // get line containing table name
  // TODO: Segfault occurs here, fix it
  line_len = getline(&curr_line, &curr_line_size, fp);
  char* table_name = curr_line;
  line_len = getline(&curr_line, &curr_line_size, fp);
  int col_count = atoi(curr_line);
  int i = 0;
  char* curr_column;
  char** column_names = malloc(sizeof(char*)*col_count);
  enum DataType* schema = malloc(sizeof(enum DataType)*col_count);
  strtok(curr_line, ";");
  for(i = 0; i < col_count && (line_len = getline(&curr_line, &curr_line_size, fp) != -1); i++){
    for (int j = 0; j < 2 && (curr_column = strtok(NULL, ";")) != NULL;){
      if(j == 0){
        column_names[i] = curr_column;
      } else {
        // TODO: Add check that column type is one of the supported types
        // i.e. atoi(curr_column) <= MAX(enum DataType) should hold
        schema[i] = atoi(curr_column);
      }
    }
  }
  if (i != col_count - 1) {
    printf("Found mismatch between col_count field and actual number of columns listed in metadata\n");
  }
  BincoffTableMetadata* metadata = init_table_metadata(table_name, col_count, column_names, schema);
  return metadata;
}

int main(int argc, char **argv) {
  // get filename & delimiter from argv
  char *filename = "test.csv";
  char *delimiter = ";";
  char *action;
  if (argc > 1) {
    action = argv[1];
  }
  else{printf("USAGE: ./a.out serialize FILENAME [DELIMITER]\n\tdeserialize TABLE_DIR_NAME");}

  // Handle DESERIALIZE command
  if (strcmp(argv[1], "deserialize") == 0) {
    char* table_dir_name = "test_table";
    if (argc > 2) {
      table_dir_name = argv[2];
    }
    BincoffTableMetadata* metadata = parse_metadata(table_dir_name);
    write_metadata(metadata, stdout);
  }
  // Handle SERIALIZE command
  else {
    if (argc > 2) {
      filename = argv[2];
    }
    if (argc > 3 ) {
      delimiter = argv[3];
    }
    // Prefetch the filesize to allocate an appropriately sized buffer
    // POSIX only!!
    struct stat st;
    stat(filename, &st);
    size_t size = st.st_size;
    printf("Input file size in bytes: %ld\n", size);

    // column 1 => string, column 2 => string, col 3 => integer, col 4 => string
    enum DataType schema[4] = {STRING, STRING, INTEGER, STRING};
    int col_count = sizeof(schema);
    char** headers = malloc(sizeof(char*)*col_count);
    BincoffTableMetadata* metadata = init_table_metadata("test_table", col_count, headers, schema);
    // prealloc buffer
    int filesize = size*2;
    void *arr = malloc(filesize);
    size_t bytes_serialized = parse_csv(filename, headers, arr, delimiter, schema);
    FILE* outfile = fopen("out.bin", "wb");
    assert(bytes_serialized < filesize);
    write_metadata(metadata, outfile);
    fwrite(arr, 1, bytes_serialized, outfile);
    fclose(outfile);
    deserialize_and_print(arr, schema, 4, bytes_serialized);
    printf("Output file size in bytes: %ld", bytes_serialized);
    
  }
}
