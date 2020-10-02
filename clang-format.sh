#!/bin/bash
set -e

#
# Runs clang-format on files in all directories except artifacts, data and non-code dirs
#
# If no arguments are passed, it applies the suggested modifications to the source files
# If --return-error-if-not-clang-formatted, then this script will return 1 if any file
# needs clang formatting
#
# Otherwise, arguments are passed directly to clang-format.
#    clang-format.sh file.cpp
# will display the formated file. Adding "-i" flag will edit it in place.

CLANG_FORMAT="clang-format-8"
CLANG_FORMAT_ARGS="-style=file"

# assert clang-format is installed
if [ ! -x "$(which $CLANG_FORMAT)" ]; then
    echo "ERROR: $CLANG_FORMAT not found. Install it apt-get. See the README.md file for details."
    exit 1
fi


ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

EXCLUDE_DIRS="cmake .git build build-release graphHttpServer/civetweb"

EXCLUDE_DIR_CMD=""
for DIR in $EXCLUDE_DIRS; do
    EXCLUDE_DIR_CMD="${EXCLUDE_DIR_CMD} -path $ROOT/$DIR -prune -o "
done

# This command will return 1 if there were any changes, and 0 otherwise
function fail_if_not_formatted() {
    ${CLANG_FORMAT} ${CLANG_FORMAT_ARGS} "$1" | diff -u "$1" -
    local ret=$?
    if [[ "$ret" != 0 ]]; then
        echo -e "\nERROR: $1 need clang-formatting. Run ./scripts/clang-format.sh"
        return 1
    fi
    return 0
}

# Exporting the function and the env vars is necessary so that parallel can access them
export -f fail_if_not_formatted
export CLANG_FORMAT
export CLANG_FORMAT_ARGS

if [ "$1" == "--return-error-if-not-formatted" ]; then
    CLANG_FORMAT_COMMAND="fail_if_not_formatted"
elif [ -z "$1" ]; then
    CLANG_FORMAT_COMMAND="${CLANG_FORMAT} ${CLANG_FORMAT_ARGS} -i"
fi

find "${ROOT}"  ${EXCLUDE_DIR_CMD} \( -iname '*.h' -o -iname '*.cpp' -o -iname '*.hpp' \) \
    -print0 | xargs -0 -n1 -P$(nproc) bash -c "${CLANG_FORMAT_COMMAND} "'"$@"' _

