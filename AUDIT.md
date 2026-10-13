# Audit of fld

This document lists the bugs and design issues in fld. Line references refer to commit `cc78bce`.

## Bugs confirmed by running

Each bug below was reproduced with a release build of commit `cc78bce`.

### 1. `pipeline --predict` with barcodes always fails at the final check

The design step computes the `begin` and `end` columns while padding and constant regions are still empty (`src/pipeline.cpp:322`). The later steps fill in padding and constant regions but do not recompute `begin` and `end` (`src/merge_padding.cpp:143`, `src/pipeline.cpp:461`). The final check then fails with `Row 1: design mismatch`, and `library.csv` keeps the stale values. The `--predict` test is skipped when `rn-coverage` is not installed, so CI does not run it.

### 2. `pipeline --predict --no-barcodes` writes an unfinished library

When `--predict` is set, the design step skips padding and leaves the constant regions empty (`src/pipeline.cpp:322`). The prediction branch runs only when barcodes are enabled (`src/pipeline.cpp:339`). Without barcodes, the `else` branch copies the unpadded library, with no constant regions, to `library.csv` and `library.fasta`.

### 3. The `begin`/`end` check rejects designs at the 3' end

The `end` column is exclusive, so `end` can equal the sequence length plus one. The check `end > fasta_seq.size()` (`src/pipeline.cpp:138`) rejects that valid case. For example, `fld pipeline --no-barcodes --three-const "" ...` always fails with `end position 60 exceeds FASTA sequence length 59`.

### 4. The README and the code disagree about `end`

The README states that `end - begin + 1 == len(design)`. The code writes an exclusive `end`, so `end - begin == len(design)` (`src/library.cpp:104`).

### 5. Output CSV from `pipeline --predict` has a duplicate column name

`_merge_padding` appends the columns `design_reads,padding_reads` (`src/merge_padding.cpp:161`). `_merge` then appends `design_reads,barcode_reads` (`src/merge.cpp:161`). The final header contains `design_reads` twice. The second `design_reads` column holds the read count of the padded design, not of the design alone.

### 6. Lowercase input is replaced with random sequence

`get_polybase_arr` treats every character that it does not recognize as `N` (`src/domain/sequence.cpp:41`). Lowercase bases are not recognized, so the design step replaces each one with a random base (`src/library.cpp:342`). For example, the input `acgtacgtacgtacgtacgt` gave the design `CGCTGCTCAGGGGTATCTTG`. No warning is printed.

### 7. CRLF line endings add a random base to each design

`for_each_fasta` does not remove `\r` from the end of each line (`src/io/fasta_io.cpp:30`). Because of bug 6, the design step replaces the `\r` with a random base. Each design becomes one base longer, and each name ends in `\r`.

### 8. Barcodes can be at Hamming distance 1

The README states that all barcodes are at Hamming distance 2 or more. The neighbor check only considers transitions (A to G and C to T) (`src/domain/barcode.cpp:9`). The tetraloops `TTCG` and `TACG` differ by a transversion (`src/domain/hairpin.cpp:6`). Two barcodes with the same stem and these two loops are at distance 1, and the check does not detect them. In one run of `fld barcodes --count 400 --length 5`, 31 pairs of barcodes were at distance 1.

### 9. `pipeline --m2` includes each design twice

`_m2` writes the wild type of each sequence as `<name>_wt` (`src/m2.cpp:56`). The pipeline also keeps the original input file (`src/pipeline.cpp:276`). Each design therefore appears twice in the library, once as `<name>` and once as `<name>_wt`.

### 10. `closing_gc` is not validated

`StemConfig::validate` does not check that `closing_gc <= max_gc` or that `closing_gc <= min_length` (`src/config/stem_config.cpp:5`). This causes three failures:

- `fld barcodes --max-gc 0` crashes with a segmentation fault. The subtraction `max_gc -= closing_gc` underflows, and the program then tries to allocate a very large vector (`src/barcodes.cpp:76`).
- In padding, `max_gc--` underflows (`src/domain/hairpin.cpp:88`). With `--max-gc 0`, padding stems then contain many GC pairs.
- `fld barcodes --length 2 --closing-gc 4` writes past the end of the `pairs` vector (`src/domain/hairpin.cpp:86`) and stops with the message `Error: vector`.

### 11. Negative integer arguments are not rejected

Integer arguments are parsed as `int` and then converted to `size_t` without a range check. For example, `--pad-to -5` becomes a very large number, and the pipeline hangs.

### 12. `pipeline --overwrite` deletes the output directory before it checks the inputs

`_pipeline` removes the output directory (`src/pipeline.cpp:250`) before it checks that the input files exist (`src/pipeline.cpp:258`). If an input file is inside the output directory, the pipeline deletes it and then reports that it does not exist. The command `fld pipeline -o . --overwrite ...` would delete the contents of the current directory.

### 13. Temporary file names can collide

The pipeline names each temporary CSV after the base name of its input file (`src/pipeline.cpp:286`).

- Two inputs with the same base name in different directories, such as `d1/lib.fasta` and `d2/lib.fasta`, write to the same temporary CSV. The sequences of the first file are lost, and the sequences of the second file appear twice.
- An input named `preprocessed.fasta` writes to `tmp/preprocessed.csv`, which is also the file that the stacking step writes (`src/pipeline.cpp:296`). The stacking step truncates the file before it reads it. The sequence count then underflows to `18446744073709551615` (`src/pipeline.cpp:307`), and the pipeline fails.

### 14. `fld txt` silently removes barcodes

The `Library` constructor removes every barcode that is a neighbor of an earlier barcode (`src/library.cpp:221-240`). `fld txt` loads its input through this constructor. If two rows share a barcode, the second line of the output has no barcode.

### 15. Most README examples use an `-o` flag that does not exist

The commands `preprocess`, `m2`, `random`, `duplicate`, `barcodes` and `txt` define `--output` but not `-o` (`src/preprocess.cpp:10`, `src/m2.cpp:94`, `src/random.cpp:51`, `src/duplicate.cpp:33`, `src/barcodes.cpp:178`, `src/totxt.cpp:9`). The README examples for these commands use `-o` and fail with `Unknown argument: -o`.

## Bugs found by reading the code

These issues were found by reading the code. They were not reproduced.

1. `_load_reads` adds zeros when a read-count file is too short and ignores extra lines when it is too long (`src/utils.cpp:134`). A read-count file with the wrong number of lines is therefore misaligned with the library, and no error is reported.
2. `Barcode::random` has no limit on the number of attempts (`src/domain/barcode.cpp:60`). The capacity check counts all hairpins that satisfy the stem limits, but it does not apply the neighbor rule (`src/barcodes.cpp:66`). The check can therefore pass when there are not enough barcodes that satisfy the neighbor rule, and generation then loops forever. This is most likely when `--max-gu` is greater than 0.
3. Quotes are handled differently in different places:
   - `_escape_with_quotes` removes quotes from names (`src/utils.cpp:76`).
   - `_quote_csv_field` doubles quotes (`src/utils.cpp:80`).
   - `_split_by_delimiter` changes state at every quote character, so a doubled quote is lost when the field is read (`src/utils.cpp:46`).
   - `csv::Header` uses a different splitter that does not handle quotes (`src/io/csv_format.cpp:57`).
   - `_preprocess` writes the sublibrary name without quotes (`src/preprocess.cpp:46`). An input file name that contains a comma therefore breaks the CSV.
4. `for_each_fasta` drops an entry with an empty sequence at the end of a file, but keeps entries with empty sequences in other positions (`src/io/fasta_io.cpp:49`).
5. When `--predict` is set, designs longer than `--pad-to` get empty padding and no error (`src/padding.cpp:44`). Without `--predict`, the same input causes an error (`src/library.cpp:186`).
6. `categorize` puts sequences that are longer than the largest bin into the largest bin, and its summary labels that bin `<= N` (`src/categorize.cpp:26`). If `--bins` is empty, the bin index underflows. The command also converts all sequences to DNA without saying so.
7. The `rn-coverage` commands are built as shell strings with paths that are not quoted (`src/pipeline.cpp:222`). A path that contains a space breaks the command.
8. `-march=native` (`CMakeLists.txt:59`) compiles for the CPU of the build machine. The binary can fail on a machine with an older CPU, for example when it is built on a Sherlock login node and run on a different type of compute node.
9. `FetchContent_Declare` has no `GIT_TAG` for `indicators` or `argparse` (`CMakeLists.txt:77-84`). Each build uses the latest upstream version, so a change upstream can break the build or CI.
10. Identifiers such as `_PARSER_NAME` start with an underscore and an uppercase letter. C++ reserves these names.
11. `_run_tests` applies `WEXITSTATUS` to the result of `std::system` without checking for `-1` (`src/test.cpp:71`).
12. `examples/fld-all/run` calls a program named `fld-all`, which no longer exists.
13. Some tests do not check their results:
    - The test `preprocess converts U to T` has no assertions (`tests/test_preprocess.cpp`).
    - The test `pipeline without --predict` only checks that the `tmp` directory exists, and its comment does not match what it checks (`tests/test_pipeline.cpp`).

## Design issues

These issues are measured against the development philosophy in `~/CLAUDE.md`.

### Single source of truth

1. The default constant regions are defined in three places: `src/config/design_config.hpp:25`, `src/library.cpp:402` and `src/pipeline.cpp:28`. The T7 prefix `GGGAACG` is an inline literal (`src/pipeline.cpp:500`).
2. Stem defaults differ between commands. The default maximum stem length is 9 for `design` (`src/library.cpp:414`, `src/config/stem_config.hpp:41`) and 13 for `pipeline` (`src/pipeline.cpp:39`). The default `max_gc` is `INT_MAX` for `barcodes` (`src/barcodes.cpp:194`) and 5 for every other command. `StemConfig::for_padding` and `StemConfig::for_barcode` define a third set of defaults.
3. The calculation of the barcode count exists twice, in `src/barcodes.cpp:66` and `src/config/barcode_config.cpp:5`. The two copies throw different exception types.
4. The loop that reads a library CSV appears in about eight places. Each copy reads the header, validates it, and splits each row.
5. The code that joins the six sequence parts and formats the FASTA name appears in `src/merge.cpp:178`, `src/merge_padding.cpp:178`, `src/sort.cpp:117` and `src/pipeline.cpp:428`, and again in `Construct::str` and `Construct::name`.
6. The sort by sublibrary and index appears in `src/merge.cpp`, `src/merge_padding.cpp` and `src/sort.cpp`.
7. There are two FASTA writers (`FastaOutputStream` and `FastaWriter`) and two CSV splitters (`_split_by_delimiter` and `csv::_split`).
8. The `begin` and `end` columns are stored in the CSV and copied through each step, instead of being computed from the sequence parts when the file is written. Bug 1 is a result of this.

### Composition

1. `_pipeline` is about 270 lines long and contains inline blocks that rewrite CSV files (`src/pipeline.cpp:243-510`).
2. `_merge`, `_merge_padding` and `_sort` each parse the input, pair or sort the entries, and write the output in one function.
3. `main` builds the configuration of each command inline in its `switch` statement (`src/main.cpp`).

### Requirements enforced in helpers

1. `StemConfig::validate` does not check `closing_gc` (bug 10).
2. Integer arguments are converted to `size_t` without a range check (bug 11).
3. The prediction branch of the pipeline depends on the design step having skipped padding and constant regions (bug 2). The design step does not enforce this, and the pipeline does not check it.

### Dead code

1. The `Sequence` class is used only by tests.
2. `StemConfig::for_padding` and `StemConfig::for_barcode` are not called.
3. `count_fasta_entries`, `for_each_fasta_indexed`, `Construct::remove_padding` and `Hairpin::loop` are not called.
4. `csv::is_valid_header` is marked as deprecated, and `csv::Column` is marked as legacy. Only tests use them.
5. `_merge_padding` reads the padding file twice (`src/merge_padding.cpp:72-82`).

### Naming

1. Some free functions start with an underscore (`_design`, `_merge`) and others do not (`get_padding`, `read_fasta`, `output_csv`).
2. Both `neighbour` and `neighbor` are used.
3. Member variables use `fivep_` and `threep_`, but CSV columns use `five_` and `three_`.
4. The base-pair code uses `AT_PAIRS`, but also `is_au` and `BasePairType::AU`.
5. Some commands use `-o` for the output and others use `--output`.
6. Some files define argument names and help text as named constants, and others use inline literals.
7. Header guards follow different patterns, for example `FLD_TOTXT_HPP`, `RANDOM_HEADER` and `#pragma once`.

### Comments

1. The comment `Also converts U to T` (`src/library.cpp:341`) does not mention that the same call replaces unrecognized characters with random bases.
2. `src/barcodes.hpp` contains decorative comment blocks with many blank lines.
