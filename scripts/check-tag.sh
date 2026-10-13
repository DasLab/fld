#!/usr/bin/env bash
#
# check-tag.sh -- confirm a release tag names the version the program reports.
#
# usage: scripts/check-tag.sh TAG
#
# The release takes its name from the tag and fld --version reports what
# src/version.hpp holds, so a tag disagreeing with the header announces a
# version other than the one the program reports.
#
# Author: Hamish M. Blair <hmblair@stanford.edu>

set -euo pipefail

PROGRAM="check-tag.sh"

die() {
    printf '%s: %s\n' "$PROGRAM" "$1" >&2
    exit 1
}

[ $# -eq 1 ] || die "usage: $PROGRAM TAG"

VERSION=$(sed -n 's/.*FLD_VERSION "\(.*\)".*/\1/p' src/version.hpp)

[ -n "$VERSION" ] || die "src/version.hpp holds no version"
[ "$1" = "v$VERSION" ] || die "tag $1 names a version other than $VERSION, which src/version.hpp holds"

printf '%s: %s matches src/version.hpp\n' "$PROGRAM" "$1"
