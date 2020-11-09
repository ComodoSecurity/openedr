#!/bin/bash

set -e

THIS_SCRIPT=`basename "$0"`

function usage
{
    echo "$THIS_SCRIPT <GIT_URL> <GIT_BRANCH> [<SRC_DIR>] [<GPG_KEY>]"
}

function gpg_sign
{
    FILE="$1"
    rm -f "$FILE".sig
    $GPG --batch --use-agent --detach-sign --local-user "$GPG_KEY" \
        "$FILE"
}

function maybe_gpg_sign
{
    [[ -e "$1" ]] && gpg_sign "$1"
}

function command_exists
{
    type "$1" &>/dev/null
}

function find_archiver
{
    command_exists "$1" && echo "$1" || echo ':'
}

GIT_URL="$1"
if [[ -z "$GIT_URL" ]] ; then
    usage
    exit 1
fi

GIT_BRANCH="$2"
if [[ -z "$GIT_BRANCH" ]] ; then
    usage
    exit 1
fi

if [[ -z "$3" ]] ; then
    SRC_DIR=$GIT_BRANCH
else
    SRC_DIR="$3"
fi

if [[ ! -z "$4" ]] ; then
    GPG_KEY="$4"
else
    GPG_KEY=
fi

DEST_DIR="$PWD"

TMPDIR=${TMPDIR:-${TMP:-${TEMP}}}
if [[ -z "${TMPDIR}" ]] ; then
    unset TMPDIR
else
    export TMPDIR
fi
TMP_DIR=`mktemp -d -t log4cplus.XXXXXXX`
pushd "$TMP_DIR"

TAR=${TAR:-$(find_archiver tar)}
XZ=${XZ:-$(find_archiver xz)}
BZIP2=${BZIP2:-$(find_archiver bzip2)}
GZIP=${GZIP:-$(find_archiver gzip)}
SEVENZA=${SEVENZA:-$(find_archiver 7za)}
LRZIP=${LRZIP:-$(find_archiver lrzip)}
GIT=${GIT:-git}
GPG=${GPG:-gpg}

$GIT clone -v --recursive --depth=1 "$GIT_URL" -b "$GIT_BRANCH" "$SRC_DIR"
(cd "$SRC_DIR" && $GIT rev-parse @ >REVISION)
rm -rf "$SRC_DIR/.git"
find "$SRC_DIR" -name .git -print -exec rm -f '{}' ';'

pushd "$SRC_DIR"
$SHELL ./scripts/fix-timestamps.sh
popd

$SEVENZA a -t7z "$DEST_DIR/$SRC_DIR".7z "$SRC_DIR" >/dev/null \
& $SEVENZA a -tzip "$DEST_DIR/$SRC_DIR".zip "$SRC_DIR" >/dev/null

TAR_FILE="$SRC_DIR".tar
$TAR -c --format=posix --numeric-owner --owner=0 --group=0 -f "$TAR_FILE" "$SRC_DIR"

$XZ -e -c "$TAR_FILE" >"$DEST_DIR/$TAR_FILE".xz \
& $BZIP2 -9 -c "$TAR_FILE" >"$DEST_DIR/$TAR_FILE".bz2 \
& $GZIP -9 -c "$TAR_FILE" >"$DEST_DIR/$TAR_FILE".gz \
& $LRZIP -q -o - "$TAR_FILE" |([[ "$LRZIP" = ":" ]] && cat >/dev/null || cat >"$DEST_DIR/$TAR_FILE".lrz)

echo waiting for tarballs...
wait
echo done waiting

if [[ ! -z "$GPG_KEY" ]] ; then
    maybe_gpg_sign "$DEST_DIR/$SRC_DIR".7z
    maybe_gpg_sign "$DEST_DIR/$SRC_DIR".zip
    maybe_gpg_sign "$DEST_DIR/$TAR_FILE".xz
    maybe_gpg_sign "$DEST_DIR/$TAR_FILE".bz2
    maybe_gpg_sign "$DEST_DIR/$TAR_FILE".gz
    maybe_gpg_sign "$DEST_DIR/$TAR_FILE".lrz
fi

rm -rf "$SRC_DIR"
rm -f "$TAR_FILE"
popd
rmdir "$TMP_DIR"
