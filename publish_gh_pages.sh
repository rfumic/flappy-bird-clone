#!/bin/bash
set -e
source ./build_common.sh

$project_root/build_emscripten.sh

cp -r $output_dir/wasm/* $project_root

printf "\033[32;1mChanges ready for publishing\033[0m\n"


