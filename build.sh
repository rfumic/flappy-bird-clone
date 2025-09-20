#!/bin/bash
printf "\n \033[1;33mRUNNING BUILD SCRIPT\033[0m \n"
set -e

APP_NAME="flappy_bird"

SDL3_CFLAGS=$(pkg-config --cflags sdl3)
SDL3_LDFLAGS=$(pkg-config --libs sdl3)

MACCOS_FLAGS="-framework CoreVideo -framework IOKit -framework Cocoa -framework CoreAudio"
# TODO: Think about flags
C_FLAGS="-std=c99 -Wall -Wextra -Wunused-function -g"

mkdir -p out

set -x
clang src/sdl_main.c \
          $MACOS_FLAGS \
          $SDL3_CFLAGS $SDL3_LDFLAGS \
          $C_FLAGS \
          -o out/$APP_NAME
set +x

printf "\033[32;1mCOMPILATION SUCCESSFUL\033[0m\n"

if [ "$1" == "run" ]; then
./out/$APP_NAME
fi

ctags -R ./src/

