# fld

Fast library design for MaP-seq.

`fld` places each design sequence into a construct with constant regions, hairpin padding, and hairpin barcodes. It can assign the padding and barcodes so that the predicted read counts are as even as possible.

## Installing

Building `fld` needs a C++17 compiler and `make`.

```sh
git clone https://github.com/DasLab/fld
cd fld
make install
```

This installs the `fld` binary to `~/.local/bin`. Pass a different `PREFIX` to install it elsewhere, for example `make install PREFIX=/usr/local`.

`fld pipeline` calls [`rn-coverage`](https://github.com/hmblair/rn-coverage), which must be on the `PATH` unless `--no-predict` is given.

## Usage

Design a library of 200-base constructs from one FASTA file, each holding a 5' constant region, padding, the design, a barcode with a 13 base-pair stem, and a 3' constant region:

```sh
TEMPLATE=const:ACTCGAGTAGAGTCGAAAA,padding,design,barcode:13,const:AAAAGAAACAACAACAACAAC
fld pipeline designs.fasta \
    -o library \
    --construct-length 200 \
    --construct-template $TEMPLATE
```

Design a library from several FASTA files. Each file becomes one sublibrary:

```sh
TEMPLATE=const:ACTCGAGTAGAGTCGAAAA,padding,design,barcode:13,const:AAAAGAAACAACAACAACAAC
fld pipeline pool1.fasta pool2.fasta \
    -o library \
    --construct-length 200 \
    --construct-template $TEMPLATE
```

Design a library with two barcodes, one on each side of the design:

```sh
TEMPLATE=const:ACTCGAGTAGAGTCGAAAA,barcode:10,design,padding,barcode:8,const:AAAAGAAACAACAACAACAAC
fld pipeline designs.fasta \
    -o library \
    --construct-length 200 \
    --construct-template $TEMPLATE
```

Design a library that also holds the single mutants of every design, for M2-seq:

```sh
TEMPLATE=const:ACTCGAGTAGAGTCGAAAA,padding,design,barcode:13,const:AAAAGAAACAACAACAACAAC
fld pipeline designs.fasta \
    -o library \
    --construct-length 200 \
    --construct-template $TEMPLATE \
    --m2
```

Design a library without read prediction, with the padding and barcodes assigned at random:

```sh
TEMPLATE=const:ACTCGAGTAGAGTCGAAAA,padding,design,barcode:13,const:AAAAGAAACAACAACAACAAC
fld pipeline designs.fasta \
    -o library \
    --construct-length 200 \
    --construct-template $TEMPLATE \
    --no-predict
```

## Construct Template

`--construct-template` describes the construct from 5' to 3' as a comma-separated list of regions.

| Region | Meaning |
| --- | --- |
| `const:SEQUENCE` | A constant region with the given sequence. |
| `barcode:N` | A hairpin barcode with a stem of N base pairs. |
| `design` | The design sequence from the input FASTA file. |
| `padding` | Hairpin padding that brings the construct to `--construct-length`. |

A template must contain exactly one `design` region and at most one `padding` region. It may contain any number of constant and barcode regions, in any order.

The padding length of each construct is the construct length less the lengths of the other regions. A design that needs more than the construct length ends the run early. Padding that is too short for a hairpin is filled with random bases. Without a padding region, every construct must have exactly the construct length.

Each barcode region is an independent set. Within one region, every pair of barcodes is at Hamming distance two or more.

## Output

`library.fasta` holds the full sequence of every construct. The header is the input name followed by three `key=value` fields.

```
>URS0000BF2FFB_3555 Beta vulgaris microRNA MIR1122 sublibrary=designs begin=19 end=149
```

`begin` and `end` locate the design in the full sequence with the Python convention: `begin` is 0-based and `end` is exclusive, so `sequence[begin:end]` is the design and `end - begin` is its length.

`library.csv` has one row per construct.

| Column | Content |
| --- | --- |
| `index` | 0-based position in the input file. |
| `name` | Name from the FASTA header. |
| `sublibrary` | Name of the input file without its extension. |
| one column per region | the sequence of that region, in template order, named `const_1`, `barcode_1`, `design`, `padding`, and so on |
| `reads_design`, `reads_padding`, `reads_barcode_1`, ... | Predicted reads of each region on its own (absent with `--no-predict`). |
| `reads` | Predicted reads of the full construct (absent with `--no-predict`). |

The full sequence is the concatenation of the region columns in order.

## Common Post-Design Operations

`fld` writes the designed DNA only. These commands make the usual derived files from `library.fasta`. Each one changes the sequence lines and leaves the headers as they are.

Add the T7 leader `GGGAACG` to every sequence, in a separate file:

```sh
sed '/^>/{n;s/^/GGGAACG/;}' library.fasta > t7-library.fasta
```

Convert to RNA:

```sh
sed '/^>/!y/T/U/' library.fasta > library-rna.fasta
```

## Options

### Input and Output

| Option | Description |
| --- | --- |
| `FASTA...` | Input FASTA files. Each file becomes one sublibrary, named after the file without its extension. The names must be distinct and contain no whitespace. |
| `-o DIR`, `--output DIR` | Write `library.csv` and `library.fasta` to this directory (required). |
| `--overwrite` | Write into an existing output directory, replacing its files. |

Sequences may be DNA or RNA, in upper or lower case. IUPAC codes such as `N` are replaced with random bases they denote. Any other character ends the run early.

### Construct Template

| Option | Description |
| --- | --- |
| `--construct-template REGIONS` | Regions of the construct from 5' to 3' (required). |
| `--construct-length N` | Total length of every construct (required). |
| `--m2` | Add a sublibrary `<file>_m2` per input file, holding the single mutant of every position of every design with the base replaced by its complement. |

The first word of each FASTA header names the design. It must not be empty, and it must be distinct across all input files. A mutant is named `<name>_mm_<position>_<from>_<to>`, where the position is 0-based and `<from>` and `<to>` are the wild type and mutant bases. The suffix is inserted after the first word of the header, so the rest of the header is kept.

### Padding Stems

| Option | Description |
| --- | --- |
| `--min-stem N` | Minimum padding stem length (default 7). |
| `--max-stem N` | Maximum padding stem length (default 13). |
| `--spacer N` | PolyA spacer between padding hairpins, and between the padding and the design (default 2). |

### Pair Limits

| Option | Description |
| --- | --- |
| `--max-au N` | Maximum AU pairs per stem (default unlimited). |
| `--max-gc N` | Maximum GC pairs per stem (default unlimited). |
| `--max-gu N` | Maximum GU pairs per stem (default 0). |
| `--terminal-gc N` | GC pairs at the open end of each stem, which keep it from fraying (default 1). |

The three maximums accept `unlimited`. The pair limits apply to padding and to every barcode region. If the run reports that there are not enough barcodes, increase the barcode stem length or raise `--max-gu`.

### Prediction

By default, the pipeline predicts reads with `rn-coverage` and chooses the padding and barcodes of each design so that the predicted reads are as even as possible. The intermediate files are kept under `tmp` in the output directory.

| Option | Description |
| --- | --- |
| `--no-predict` | Skip read prediction and assign the padding and barcodes at random. |
| `--sort ORDER` | Order of the output: `none` keeps the input order, `reads` sorts by predicted reads, lowest first, and `length` sorts by design length, shortest first (default none). |

### Determinism

| Option | Description |
| --- | --- |
| `--seed N` | Seed for the random number generator. |

The same seed reproduces a run on the same platform and compiler. Different standard libraries draw different random numbers from one seed.

`fld barcodes` takes `--count N`, `--length N`, `-o FILE`, `--overwrite`, `--seed N`, and the pair limits. `fld m2` takes a FASTA file, `-o FILE`, `--overwrite`, `--seed N`, and `--all`, which writes all three mutants per position instead of the complement only. The sequences are normalized the same way as in the pipeline, so the output is uppercase DNA with every IUPAC code replaced. The wild type is written as `<name>_wt`, followed by its mutants, named as described for `--m2`.

## Development

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for building, running the tests, and releasing a version.
