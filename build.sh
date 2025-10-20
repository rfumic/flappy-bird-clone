#!/bin/bash
# My local dev build file

./build_debug_macos.sh
# ./build_emscripten.sh

ctags -R ./src/
