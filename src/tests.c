#include "bincoff.h"
#include "bincoff_internal.h"
#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>

/*
  TESTING TODO:

  [ ] test data setup as fixtures (commit basic test data files to VCS)
  [ ] unit tests for all major functions listed in bincoff.h
  [ ] CLI interface tests
  [ ] Data de-/serialization correctness tests
  -   [ ] parse_csv_columnar()
  -   [ ] integers (negative, zero, positive, underflow, overflow)
      [ ] strings (len matches string, preservation/stripping of newlines,
      handling of escapes)
      [ ] floats (use double-precision?) (NaN, Inf, bit-level
      correctness)
      [ ] enums (valid enum option, invalid enum option handling,
      dynamic enum member count (uint8/16/32 etc...?)
      [ ] delimiter handling (empty column data i.e. repeated delimiters)
      [ ] Convert SizedBincoffBuffer to dynamic arrays with actual datapoints

  [ ] Performance testing
      [ ] parse csv -> serialized data buffer
      [ ] bincoff binary file -> in-mem data buffer
      [ ] sample analytical data query
      [ ] E2E parse bincoff binary file -> write query -> write to disk
*/

Test(test_parse_schema, test_parse_schema_valid_input) {
  enum DataType expected_schema[4] = {INTEGER, INTEGER, STRING, INTEGER};
  char mock_file[] = "INTEGER;INTEGER;STRING;INTEGER";
  FILE *schema_fp = fmemopen(mock_file, sizeof(mock_file), "r");
  enum DataType *returned_schema = NULL;
  parse_schema(schema_fp, &returned_schema);

  cr_expect_arr_eq(expected_schema, returned_schema, sizeof(enum DataType) * 4);
}

// TODO: test_parse_schema, test_parse_schema_invalid_input

Test(test_parse_metadata, test_parse_metadata_valid_input) {
  enum DataType schema[3] = {INTEGER, INTEGER, STRING};
  char *col_names[3] = {"foo", "bar", "baz"};
  BincoffTableMetadata expected_metadata = {.table_name = "test_table_1",
                                            .col_names = (char **)col_names,
                                            .col_count = 3,
                                            .col_types = schema};
  char mock_file[128];
  sprintf(mock_file, "test_table_1\n3\nfoo;%d\nbar;%d\nbaz;%d\n%c", INTEGER,
          INTEGER, STRING, EOF);
  FILE *metadata_fp = fmemopen(mock_file, sizeof(mock_file), "r");
  BincoffTableMetadata *returned_metadata =
      _parse_metadata_internal(metadata_fp);

  cr_assert(
      strcmp(expected_metadata.table_name, returned_metadata->table_name) == 0);
  for (size_t i = 0; i < expected_metadata.col_count; i++) {
    cr_assert(strcmp(expected_metadata.col_names[i],
                     returned_metadata->col_names[i]) == 0);
  }
  cr_assert(expected_metadata.col_count == returned_metadata->col_count);
  for (size_t i = 0; i < expected_metadata.col_count; i++) {
    cr_assert(expected_metadata.col_types[i] ==
              returned_metadata->col_types[i]);
  }
}

Test(test_parse_csv, parse_valid_csv_into_columns_valid_schema) {
  enum DataType MOCK_SCHEMA[3] = {INTEGER, STRING, STRING};
  // set up test file
  char *MOCK_FILE_HEADERS = "foo;bar;baz\n";
  char *MOCK_FILE_ROW1 =
      "-1;a string with varying c1234 1_!! contents;another strr\n";
  char *MOCK_FILE_ROW2 = "1401298;I am on column \"bar\"!;    \n";
  char *MOCK_FILE_ROW3 = "789172389;suspicious but valid contents\f \t ;\t\t\t";

  char mock_file[256];
  sprintf(mock_file, "%s%s%s%s%c", MOCK_FILE_HEADERS, MOCK_FILE_ROW1,
          MOCK_FILE_ROW2, MOCK_FILE_ROW3, EOF);
  FILE *fp = fmemopen(mock_file, sizeof(mock_file), "r");
  size_t fsize = strlen(mock_file);
  char **headers_buffer = malloc(3 * sizeof(char *));
  SizedBincoffBuffer **column_buffers = NULL;
  char *delimiter = ";";
  size_t ret = _parse_csv_columnar_internal(fp, headers_buffer, &column_buffers,
                                            delimiter, MOCK_SCHEMA, fsize);

  cr_assert(3 == ret);
  // The reason we have 5 elements is because each string occupies 2 elements
  // due to the string length and string itself being distinct elements in the
  // buffer
  cr_assert(column_buffers[0]->element_count == 5);
  cr_assert(column_buffers[1]->element_count == 5);
  cr_assert(column_buffers[2]->element_count == 5);
}

// TODO: test_parse_csv, test_parse_csv_invalid_input_malformed_csv
// TODO: test_parse_csv, test_parse_csv_invalid_input_not_matching_schema
