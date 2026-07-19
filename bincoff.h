#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define STRIP_NEWLINE(string) string[strcspn(string, "\n")] = '\0'

enum DataType { INTEGER, STRING };

typedef struct {
  char *table_name;
  uint32_t col_count;
  char **col_names;
  enum DataType *col_types;
} BincoffTableMetadata;

enum DataType datatype_str_to_enumval(const char *str);

enum DataType *parse_schema(FILE *fp);

/* Caller is responsible for ensuring validity and lifetime
 of memory behind any of the addresses passed into this function **/
BincoffTableMetadata *init_table_metadata(char *table_name, uint32_t col_count,
                                          char **col_names,
                                          enum DataType *col_types);

/* Serialize value in src as type data_type and write it to dst as bytes, with
 length prepended if it is a string **/
size_t serialize_and_insert(void **dst, char *src, enum DataType data_type);

size_t deserialize_value(void *value, enum DataType data_type);

void deserialize_and_print(void *data, BincoffTableMetadata *metadata,
                           size_t data_len);

size_t parse_csv(char *filename, char **headers_buffer, void *buffer,
                 char *delimiter, enum DataType *schema);

void write_metadata(BincoffTableMetadata *metadata, FILE *outfile);

BincoffTableMetadata *parse_metadata(char *dir);
