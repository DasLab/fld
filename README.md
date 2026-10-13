# fld

Fast library design for MaP-seq.

`fld` places each design sequence into a construct with constant regions, hairpin padding, and hairpin barcodes. It can assign the padding and barcodes so that the predicted read counts are as even as possible.

## Installing

Building `fld` needs a C++17 compiler and `make`.

```sh
git clone https://github.com/DasLab/fld
cd fld
make install PREFIX=~/.local
```

This installs the `fld` binary to `~/.local/bin`. Pass a different `PREFIX` to install it elsewhere.

`fld pipeline` calls [`rn-coverage`](https://github.com/hmblair/rn-coverage), which must be on the `PATH` unless `--no-predict` is given.

## Usage

Design a library of 200-base constructs from one FASTA file, each holding a 5' constant region, padding, the design, a barcode with a 13 base-pair stem, and a 3' constant region:

```sh
LAYOUT=const:ACTCGAGTAGAGTCGAAAA,padding,design,barcode:13,const:AAAAGAAACAACAACAACAAC
fld pipeline designs.fasta \
    -o library \
    --construct-length 200 \
    --layout $LAYOUT
```

Design a library from several FASTA files. Each file becomes one sublibrary:

```sh
LAYOUT=const:ACTCGAGTAGAGTCGAAAA,padding,design,barcode:13,const:AAAAGAAACAACAACAACAAC
fld pipeline pool1.fasta pool2.fasta \
    -o library \
    --construct-length 200 \
    --layout $LAYOUT
```

Design a library with two barcodes, one on each side of the design:

```sh
LAYOUT=const:ACTCGAGTAGAGTCGAAAA,barcode:10,design,padding,barcode:8,const:AAAAGAAACAACAACAACAAC
fld pipeline designs.fasta \
    -o library \
    --construct-length 200 \
    --layout $LAYOUT
```

Design a library that also holds the single mutants of every design, for M2-seq:

```sh
LAYOUT=const:ACTCGAGTAGAGTCGAAAA,padding,design,barcode:13,const:AAAAGAAACAACAACAACAAC
fld pipeline designs.fasta \
    -o library \
    --construct-length 200 \
    --layout $LAYOUT \
    --m2
```

Design a library without read prediction, with the padding and barcodes assigned at random:

```sh
LAYOUT=const:ACTCGAGTAGAGTCGAAAA,padding,design,barcode:13,const:AAAAGAAACAACAACAACAAC
fld pipeline designs.fasta \
    -o library \
    --construct-length 200 \
    --layout $LAYOUT \
    --no-predict
```

## Layout

`--layout` describes the construct from 5' to 3' as a comma-separated list of slots.

| Slot | Meaning |
| --- | --- |
| `const:SEQUENCE` | a constant region with the given sequence |
| `barcode:N` | a hairpin barcode with a stem of N base pairs |
| `design` | the design sequence from the input FASTA file |
| `padding` | hairpin padding that brings the construct to `--construct-length` |

A layout must contain exactly one `design` slot and at most one `padding` slot. It may contain any number of constant and barcode slots, in any order.

The padding length of each construct is the construct length less the lengths of the other slots. A design that needs more than the construct length ends the run early. Padding that is too short for a hairpin is random sequence. Without a padding slot, every construct must have exactly the construct length.

Each barcode slot is an independent set. Within one slot, every pair of barcodes is at Hamming distance two or more.

## Output

`library.fasta` holds the full sequence of every construct. The header is the input name followed by three `key=value` fields.

```
>URS0000BF2FFB_3555 Beta vulgaris microRNA MIR1122 sublibrary=designs begin=19 end=149
```

`begin` and `end` locate the design in the full sequence with the Python convention: `begin` is 0-based and `end` is exclusive, so `sequence[begin:end]` is the design and `end - begin` is its length.

`library.csv` has one row per construct.

| Column | Content |
| --- | --- |
| `index` | 0-based position in the input file |
| `name` | name from the FASTA header |
| `sublibrary` | name of the input file without its extension |
| one column per slot | the sequence of that slot, in layout order, named `const_1`, `barcode_1`, `design`, `padding`, and so on |
| `reads_design`, `reads_padding`, `reads_barcode_1`, ... | predicted reads of each component on its own (absent with `--no-predict`) |
| `reads` | predicted reads of the full construct (absent with `--no-predict`) |

The full sequence is the concatenation of the slot columns in order.

## Options

### Input and output

| Option | Description |
| --- | --- |
| `FASTA...` | input FASTA files, one sublibrary each, named after the file without its extension; the names must be distinct and contain no whitespace |
| `-o DIR` | write `library.csv` and `library.fasta` to this directory (required) |
| `--overwrite` | write into an existing output directory, replacing its files |

Sequences may be DNA or RNA, in upper or lower case. IUPAC codes such as `N` are replaced with random bases they denote. Any other character ends the run early.

### Layout

| Option | Description |
| --- | --- |
| `--layout SLOTS` | slots of the construct from 5' to 3' (required) |
| `--construct-length N` | total length of every construct (required) |
| `--m2` | add a sublibrary `<file>_m2` per input file, holding the single mutant of every position of every design with the base replaced by its complement |

### Padding stems

| Option | Description |
| --- | --- |
| `--min-stem N` | minimum padding stem length (default 7) |
| `--max-stem N` | maximum padding stem length (default 13) |
| `--spacer N` | polyA spacer between padding hairpins, and between the padding and the design (default 2) |

### Pair limits

| Option | Description |
| --- | --- |
| `--max-au N` | maximum AU pairs per stem (default unlimited) |
| `--max-gc N` | maximum GC pairs per stem (default 5) |
| `--max-gu N` | maximum GU pairs per stem (default 0) |
| `--terminal-gc N` | GC pairs at the open end of each stem, which keep it from fraying (default 1) |

The three maximums accept `unlimited`. The pair limits apply to padding and to every barcode slot. If the run reports that there are not enough barcodes, increase the barcode stem length or raise `--max-gu` or `--max-gc`.

### Prediction

By default, the pipeline predicts reads with `rn-coverage` and chooses the padding and barcodes of each design so that the predicted reads are as even as possible. The intermediate files are kept under `tmp` in the output directory.

| Option | Description |
| --- | --- |
| `--no-predict` | skip read prediction and assign the padding and barcodes at random |
| `--sort ORDER` | order of the output: `none` keeps the input order, `reads` sorts by predicted reads, lowest first, and `length` sorts by design length, shortest first (default none) |

### Determinism

| Option | Description |
| --- | --- |
| `--seed N` | seed for the random number generator |

The same seed reproduces a run on the same platform and compiler. Different standard libraries draw different random numbers from one seed.

`fld barcodes` takes `--count N`, `--length N`, `-o FILE`, `--overwrite`, `--seed N`, and the pair limits. `fld m2` takes a FASTA file, `-o FILE`, `--overwrite`, `--seed N`, and `--all`, which writes all three mutants per position instead of the complement only. The sequences are normalized the same way as in the pipeline, so the output is uppercase DNA with every IUPAC code replaced.

## Development

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for building, running the tests, and releasing a version.
