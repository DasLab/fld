# Changelog

## [1.0.0] - 2026-10-12

`fld` 1.0.0 is a rewrite in C++17 with no dependencies. It builds with `make` and has three subcommands: `pipeline`, `barcodes`, and `m2`.

### Changed

- The construct is described by `--layout`, a list of slots from 5' to 3': any number of `const:SEQ` and `barcode:N` slots, one `design`, and at most one `padding`, in any order. `--construct-length` replaces `--pad-to` and gives the total length of every construct. The 5' and 3' constant regions no longer have defaults.
- Read prediction runs by default. `--no-predict` skips it. Padding and barcodes are assigned in one step that minimizes the variance of the summed predicted reads, instead of the two rounds of pairing.
- `library.csv` has one column per slot, named `const_1`, `barcode_1`, `design`, `padding`, and so on, followed by the predicted reads of each component and of the full construct. `index` is 0-based.
- `begin` and `end` moved from the CSV to the FASTA header, as `sublibrary=X begin=B end=E`, with the Python convention: `begin` is 0-based and `end` is exclusive.
- `--sort none|reads|length` replaces `--sort-by-reads`.
- `--closing-gc` is renamed to `--terminal-gc`.
- `--seed` makes a run reproducible. `fld barcodes` and `fld m2` take it too.
- Input is uppercased and `U` is converted to `T`. Carriage returns are stripped. Any other character outside the IUPAC codes stops the run with the entry name, instead of being replaced with a random base.
- `fld m2` normalizes its input the same way as the pipeline: uppercase DNA with every IUPAC code replaced. Mutant names are inserted after the first word of the header.
- Every library is verified before it is written: construct lengths, bases, the design at `begin:end`, the constant regions, and that each padding and barcode is used exactly once.

### Removed

- The T7 FASTA output.
- The subcommands `preprocess`, `design`, `merge`, `sort`, `inspect`, `prepend`, `to-rna`, `to-dna`, `txt`, `diff`, `duplicate`, `random`, `categorize`, and `test`.

### Fixed

- Barcodes could be at Hamming distance 1, because the neighbor check only tried transitions and so missed the one-base difference between the loops `TTCG` and `TACG`. The check now covers every single substitution.
- `--m2` included each design twice.
- The mutants of a design with IUPAC codes were not single mutants of its wild type, because the codes were replaced separately for each mutant.
- `--max-gc 0` and a closing GC count larger than the stem crashed. The pair limits are now validated.
- Negative integer arguments were accepted and hung the run.
- `--overwrite` removed the output directory before the inputs were checked. A failed run now leaves no output directory.
- Input files with the same name in different directories overwrote each other's intermediate files. Duplicate sublibrary names are now rejected, including a name that collides with the `_m2` sublibrary of another input.
- Paths with spaces broke the `rn-coverage` commands.
