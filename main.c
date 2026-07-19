#include "bincoff.h"
#include <asm-generic/errno-base.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Higher-level TODOs

// [x] TODO: Metadata for binary file (total bytes, column_number, datatype);
// TODO: Schema from specification file
// [x] TODO: Reading data from file and deserialization
// [x] TODO: Dynamically sized input data buffer for handling large files
// TODO: Columns as separate files
// TODO: Bitmap encoding?
// TODO: Correctness test suite
// TODO: Examples of analytical queries and file sizes
// TODO: Parallelized serializiation, deserialization and analytical queries
// TODO: Conversion back to csv?
// [x] TODO: Split code into multiple files with better organization

// Smaller TODOs

// [x] TODO: Serialize fails with segfault if output table directory doesn't
// exist
// TODO: document choice of uint32_t as length specifier for strings (long is
// overkill) [x] TODO: change int type on the INTEGER column type to a
// standardized (4 byte) type so it is the same on 32-bit and 64-bit systems
// (int32_t for instance)

int main(int argc, char **argv) {
  char *cwd = malloc(256);
  getcwd(cwd, 256);
  if (cwd == NULL) {
    exit(3);
  }
  // get filename & delimiter from argv
  char *filename = "test.csv";
  char *given_tablename = "test_table";
  char* schema_name;
  char *delimiter = ";";
  char *action;
  if (argc >= 2) {
    action = argv[1];
  } else {
    printf("USAGE: \n\t./a.out serialize INPUT_FILE DELIMITER SCHEMA_FILE "
           "TABLE_NAME\n\tdeserialize TABLE_DIR_NAME");
  }

  // Handle DESERIALIZE command
  if (strcmp(argv[1], "deserialize") == 0) {
    char *table_dir_name = "test_table";
    if (argc >= 2) {
      table_dir_name = argv[2];
    }
    BincoffTableMetadata *metadata = parse_metadata(table_dir_name);
    write_metadata(metadata, stdout);
    // dirname/col_N.bin -- /col_N.bin is 9 chars + max 7 chars for N
    // 9999999 <- max number of columns
    char *table_path = malloc(strlen(table_dir_name) + 16);
    sprintf(table_path, "%s/col_0.bin", table_dir_name);
    struct stat st;
    stat(table_path, &st);
    size_t file_size = st.st_size;
    FILE *fp = fopen(table_path, "rb");
    if (fp == NULL) {
      perror("fopen: Failed to open file");
      exit(1);
    }
    // printf("file size in bytes: %ld of file at: %s\n", file_size,
    // table_path);
    void *data = malloc(file_size * 2);
    fread(data, 1, file_size, fp);
    deserialize_and_print(data, metadata, file_size);
  }
  // Handle SERIALIZE command
  else {
    if (argc >= 3) {
      // input file to serialize (in csv format)
      filename = argv[2];
    }
    if (argc >= 4) {
      // delimiter used in the input csv file
      delimiter = argv[3];
    }
    if (argc >= 5) {
      // schema file name
      schema_name = argv[4];
    }
    if (argc >= 6) {
      // output table name
      given_tablename = argv[5];
    }
    for (int i = 0; i < argc; i++) {
      printf("arg \t#%d => %s\n", i, argv[i]);
    }
    // Prefetch the filesize to allocate an appropriately sized buffer
    // POSIX only!!
    struct stat st;
    stat(filename, &st);
    size_t size = st.st_size;
    printf("Input CSV file size in bytes: %ld\n", size);

    // column 1 => string, column 2 => string, col 3 => integer, col 4 => string
    // enum DataType schema[4] = {STRING, STRING, INTEGER, STRING};
    // enum DataType schema[6] = {INTEGER, INTEGER, INTEGER,
    //                            INTEGER, INTEGER, INTEGER};
    FILE* schema_fp = fopen(schema_name, "r");
    if (schema_fp == NULL) {
      perror("fopen: failed to open schema file");
      exit(1);
    }
    enum DataType* schema = parse_schema(schema_fp);
    fclose(schema_fp);

    int col_count = sizeof(schema) / sizeof(int);
    char **headers = malloc(sizeof(char *) * col_count);
    char *data_path = malloc(strlen(given_tablename) + 32);
    char *metadata_path = malloc(strlen(given_tablename) + 32);
    sprintf(data_path, "%s/col_0.bin", given_tablename);
    sprintf(metadata_path, "%s/metadata", given_tablename);
    printf("data path: %s\tmetadata path: %s\n", data_path, metadata_path);
    // Check existence of output path, otherwise fail with error msg
    if (stat(given_tablename, &st) == 0) {
      if (!S_ISDIR(st.st_mode)) {
        printf("Output path exists, but is not a directory, exiting...\n");
        exit(1);
      }
    } else {
      if (errno == ENOENT) {
        printf("Desired output directory wasn't found, please create it and "
               "rerun bincoff\n");
        exit(1);
      } else {
        perror("stat failed when checking existence of output table directory");
        exit(1);
      }
    }

    // prealloc buffer
    int filesize = size * 2;
    void *data = malloc(filesize);
    size_t bytes_serialized =
        parse_csv(filename, headers, data, delimiter, schema);
    FILE *outfile = fopen(data_path, "wb");
    if (outfile == NULL) {
      perror("fopen: Failed to open file");
      exit(1);
    }
    FILE *metadata_outfile = fopen(metadata_path, "wb");
    if (metadata_outfile == NULL) {
      perror("fopen: Failed to open file");
      exit(1);
    }
    assert(bytes_serialized < filesize);
    BincoffTableMetadata *metadata =
        init_table_metadata(given_tablename, col_count, headers, schema);
    write_metadata(metadata, metadata_outfile);
    fwrite(data, 1, bytes_serialized, outfile);
    fclose(outfile);
    fclose(metadata_outfile);
    printf("Output file bytes written: %ld\n", bytes_serialized);
  }
}
