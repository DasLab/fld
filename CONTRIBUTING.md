# Contributing

## Building

`make` builds `bin/fld`. `make install PREFIX=<dir>` copies it to `<dir>/bin`.

## Tests

`make test` builds and runs the test suite. The `CI` workflow runs the build and the tests on every push to `main` and every pull request.

## Releasing

1. Set the version string in `src/version.hpp`, for example `#define FLD_VERSION "1.1.0"`. This is the only place the version is stored, and `fld --version` reports it.
2. Commit, then tag the commit with the same number, for example `git tag v1.1.0`, and push the tag.
