#!/bin/bash
printf "\n \033[1;33mRUNNING BUILD SCRIPT\033[0m \n"
set -e

APP_NAME="flappy_bird"

SDL3_CFLAGS=$(pkg-config --cflags sdl3)
SDL3_LDFLAGS=$(pkg-config --libs sdl3)

MACCOS_FLAGS="-framework CoreVideo -framework IOKit -framework Cocoa -framework CoreAudio"
CUSTOM_FLAGS="-D FLAPPY_DEBUG"
# TODO: Think about flags (-fsanitize=...,undefined)
WARNING_FLAGS="-Wall -Wextra -Wdouble-promotion -Wconversion -Wno-unused-function  -Wno-sign-conversion"
C_FLAGS="-std=c99 -fsanitize=address $WARNING_FLAGS -g3"

mkdir -p out

set -x
clang src/sdl_main.c \
          $MACOS_FLAGS \
          $SDL3_CFLAGS $SDL3_LDFLAGS \
          $CUSTOM_FLAGS \
          $C_FLAGS \
          -o out/$APP_NAME
set +x

printf "\033[32;1mCOMPILATION SUCCESSFUL\033[0m\n"

if [ "$1" == "run" ]; then
./out/$APP_NAME
fi

ctags -R ./src/

