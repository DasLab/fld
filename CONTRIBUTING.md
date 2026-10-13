# Contributing

## Building

`make` builds `bin/fld`. `make install PREFIX=<dir>` copies it to `<dir>/bin`.

## Tests

`make test` builds and runs the test suite. The `CI` workflow runs the build and the tests on every push to `main` and every pull request.

## Releasing

A release is a `v*` tag on `main`. Before tagging, set two files to the version the tag will name:

1. `src/version.hpp`, which `fld --version` reports, for example `#define FLD_VERSION "1.1.0"`.
2. `CHANGELOG.md`, which needs an entry under `## [<version>]`. The release takes its notes from that entry.

Pushing the tag runs the `Release` workflow. Its first job runs `scripts/check-tag.sh`, which fails if the tag disagrees with the header, and `scripts/release-notes.sh`, which fails if the changelog holds no entry for the version. The second job builds the tagged commit, runs the test suite, and creates a draft GitHub release with the changelog entry as its notes. You publish the draft by hand.
