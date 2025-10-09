#!/bin/bash
printf "\n \033[1;33mRUNNING WASM BUILD SCRIPT\033[0m \n"
set -e

APP_NAME="flappy_bird"

CUSTOM_FLAGS="-D FLAPPY_DEBUG"
WARNING_FLAGS="-Wall -Wextra -Wdouble-promotion -Wconversion -Wno-unused-function  -Wno-sign-conversion"
C_FLAGS="-std=c99 -fsanitize=address $WARNING_FLAGS -g3"
EMSCRIPTEN_FLAGS="-sUSE_SDL=3 -gsource-map"


mkdir -p out/assets

cp ./assets/*.bmp ./out/assets/

set -x
emcc src/sdl_main.c \
         $EMSCRIPTEN_FLAGS \
         $C_FLAGS \
         $CUSTOM_FLAGS \
         --embed-file ./out/assets@assets \
         -o ./out/wasm-out-test/index.html

set +x

printf "\033[32;1mCOMPILATION SUCCESSFUL\033[0m\n"

ctags -R ./src/

