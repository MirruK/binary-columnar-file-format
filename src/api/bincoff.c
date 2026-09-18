#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "bincoff.h"

size_t parse_csv_columnar(char *filename, char **headers_buffer,
                          SizedBincoffBuffer ***column_buffers_ptr,
                          char *delimiter, DataType *schema) {
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
  for (size_t i = 0; i < metadata->col_count; i++) {
    fprintf(outfile, "%s;%d\n", metadata->col_names[i], metadata->col_types[i]);
  }
}

BincoffTableMetadata *parse_metadata(char *dir) {
  int dirname_size = strlen(dir);
  char m[] = "/metadata";
  char *metadata_path = malloc(dirname_size + sizeof(m));
  strcpy(metadata_path, dir);
  strcat(metadata_path, m);
  FILE *fp = fopen(metadata_path, "rb");
  if (fp == NULL) {
    perror("dunno what happened but metadata file could not be loaded");
    exit(1);
  }
  return _parse_metadata_internal(fp);
}
