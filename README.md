# Binary columnar file-format "bincoff"

The name `bincoff` ( \[Bin\]ary \[Co\]lumnar \[F\]ile \[F\]rmat ), is just a codename at this point but maybe it sticks.

## How to build

- Tests: `make test` and `./test-bincoff`

- CLI: `make cli`
  - see CLI-section for how to use the CLI binary

- debugging: `make debug` and `./debug-bincoff`
  - Add your desired code to debug.c, the compiled binary will have address sanitizer and debug options enabled

## Rationale and idea

CSV (comma-separated values) is a human readable data storage format.
The file structure starts with one row containing the names of the N-columns, separated by some user-selected delimiter (often comma or semicolon).
Directly following the column names (header) comes rows of data separated by newlines, each row has N columns of plaintext data.

The pros of this format is that it is very easy for humans to parse, it lends itself well to spreadsheet type applications, and is easy to work with.

The downsides are tough to deal with escaping when the delimiter exists in the row contents, as well as a large file-size due to everything being represented
by ascii-text, which can have a massive overhead compared to a binary representation enriched by a known schema. For example an unsigned 32-bit integer needs 4 bytes if binary encoded,
but the largest unsigned 32-bit integer in ascii (represented by the string "4294967295") would need a whole 10-bytes. Not to mention how much faster processing binary encoded data is.

Therefore I wanted to create a utility that can serialize/deserialize CSV-files into/from a custom binary format.

The goal is for the implementation to be parallelized to make the serialization/deserialization fast and make resulting file(s) smaller and faster to process to aid
for example with analytical processing of the CSV data.

## CLI

The CLI of this tool supports the following actions:

1. Serialize with `bincoff-cli serialize INPUT_FILE.csv OUT_DIR`
2. Deserialize using `bincoff-cli deserialize INPUT_DIR`

## Schemas (WIP)

A primitive schema format is defined in order to aid the serialization/deserialization process.

As the user of the utility you must define the type of the values of each column

### An example schema
```
// myschema.bincoff
STRING;INTEGER;INTEGER
```

The schema file doesn't have to have any specific file extension, but .bincoff can be used for clarity.

This schema file could then be used to serialize a CSV file like this:

```csv
foo;bar;baz
hello;1337;46
world;9001;16
```

### Supported types (WIP)

Currently variable length strings, and 4-byte signed integers are supported.

TODO: Floats and other basic datatypes

## The file format (WIP)

Currently the format for one table of data is a directory that has a file called "metadata", this
file includes information like table name, column count, column names as well as their corresponding datatype.

The actual data is stored in a file cal col_0.bin which includes the serialized table data, the idea is to split this up
so that each column of data is stored in its own file, instead of the current layout of having data of each column sequentially,
for each row.

## Parallelization

Each column can be serialized and written on separate threads and the document can be scanned in chunks, utilizing SIMD to read eg. 8 values at a time.
Writes to the resulting files should be done in a chunked fashion to limit the syscall / IO overhead.

## Testing

Currently only a few unit tests exist, and they are implemented using [criterion](https://github.com/Snaipe/Criterion).

The plan is to implement correctness tests for the parsing logic, tests of the CLI functionality, and maybe most importantly, performance
tests for some common usage flows. These tests will be further expanded once parallelization has been added.

## Development environment and clangd

For formatting and lsp features, use clangd-format and clangd, respectively.

In order to get clangd to understand how the program should be built, it needs a compile_commands.json-file, this can be created by using [bear](https://github.com/rizsotto/Bear) which can intercept the compilation commands ran by make to automate the process.
