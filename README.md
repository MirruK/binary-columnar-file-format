= Binary columnar file-format

TODO: Come up with a name for the project

== How to build

Use: `gcc main.c` and run with for example `./a.out test/test-short.csv ";"`

== Rationale and idea

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

== CLI

The CLI of this tool supports the following actions:

Placeholder name `bincoff` ( \[Bin\]ary \[Co\]lumnar \[F\]ile \[F\]rmat ) is used here just for examples sake

1. Serialize with `bincoff-cli --serialize --schema SCHEMA_FILE.bincoff INPUT_FILE.csv --output-dir OUT_DIR`
2. Deserialize using `bincoff-cli --deserialize INPUT_DIR --output-file OUTPUT_FILE.csv`

== Schemas

A primitive schema format is defined in order to aid the serialization/deserialization process.

As the user of the utility you must define:

1. The type of the values of each column
2. (OPTIONAL) The delimiter of the input document (Can also be passed by CLI-argument when serializing)

=== An example schema
```
// myschema.bincoff
;
foo:string
bar:i32
baz:f64
```

This schema file could then be used to serialize a CSV file like this:

```csv
foo;bar;baz
hello;1337;4.6
world;9001;16
```

== The file format

Given the previous example input file and schema, the columns would then be split up into individual files, and the values would be encoded and repeated in a long sequence.
Strings lengths have to be noted at the time of serialization in order to write it before the value so that the deserializer can correctly read the binary file.

== Parallelization

Each column can be serialized and written on separate threads and the document can be scanned in chunks, utilizing SIMD to read eg. 8 values at a time.
Writes to the resulting files should be done in a chunked fashion to limit the syscall / IO overhead.
