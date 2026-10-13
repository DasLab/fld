# Changelog

## [1.0.0] - 2026-10-13

A rewrite with no dependencies. It builds with `make` and has three subcommands: `pipeline`, `barcodes`, and `m2`. Designing one million sequences with barcodes and without prediction is about 20% faster.

### Changed

- Every construct followed one fixed template. `--construct-template` now lists its regions from 5' to 3'.
- `--pad-to` gave the padded design length. `--construct-length` now gives the total construct length.
- The constant regions had defaults. They are now given in `--construct-template`.
- Prediction ran only with `--predict`. It now runs by default, and `--no-predict` skips it.
- Padding and barcodes were assigned in two rounds. They are now assigned in one step that minimizes the variance of the summed predicted reads.
- `library.csv` had columns for the six fixed regions. It now has one column per region of the template, plus the predicted reads of each region and of the full construct.
- `index` was 1-based. It is now 0-based.
- `begin` and `end` were 1-based inclusive CSV columns. They are now 0-based exclusive fields in the FASTA header, with `sublibrary`.
- `--max-gc` defaulted to 5. It now defaults to `unlimited`.
- `--sort-by-reads` sorted by reads. `--sort none|reads|length` now chooses the order.
- `-o` was the only name of the output option. `--output` is now its long name.
- `--closing-gc` is renamed to `--terminal-gc`. `--min-stem-length` and `--max-stem-length` are renamed to `--min-stem` and `--max-stem`.
- Runs were seeded from the clock. They are now seeded from the operating system, or from `--seed`, which makes a run reproducible.
- A character outside the IUPAC codes was replaced with a random base. It now stops the run with the entry name.
- Lowercase and RNA input was passed through. It is now uppercased, with `U` converted to `T`.
- The first word of a FASTA header could be empty or repeated. It must now be non-empty and distinct across all input files.
- Output was not checked against the input. Every library is now verified before it is written.

### Removed

- The T7 FASTA output.
- The subcommands `preprocess`, `design`, `merge`, `sort`, `inspect`, `prepend`, `to-rna`, `to-dna`, `txt`, `diff`, `duplicate`, `random`, `categorize`, and `test`.

### Fixed

- Barcodes could be at Hamming distance 1, because the neighbor check missed the loops `TTCG` and `TACG`. The check now covers every single substitution.
- `--m2` included each design twice. It now includes each design once.
- A design with IUPAC codes had its codes replaced with random bases separately in each mutant, so the mutants differed from the wild type at more than one position. The codes are now replaced once, before the mutants are made.
- A closing GC count larger than the stem or than `--max-gc` crashed. The pair limits are now checked against each other.
- A negative integer argument hung the run. It is now rejected.
- `--overwrite` removed the output directory before the inputs were checked. The directory is now created after the inputs are checked, so an early failure leaves no output directory.
- Two input files with the same name in different directories overwrote each other's intermediate files. They are now rejected.
- A path with a space broke the `rn-coverage` commands. Paths are now quoted.
- A repeated option kept the last value. It is now rejected.
- `-o --overwrite` wrote the library to a directory named `--overwrite`. An option that is given another option as its value is now rejected.
