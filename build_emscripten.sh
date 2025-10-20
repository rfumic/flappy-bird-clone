#!/bin/bash
source ./build_common.sh

PrintBuildStart

output_path="$output_dir/wasm/index.html"

emscripten_template_html="$project_root/emscripten_template.html"

c_standard="-std=gnu99"
emscripten_flags="-sUSE_SDL=3 -lidbfs.js -gsource-map \
                  --shell-file $emscripten_template_html \
                  --embed-file $assets_output_dir_path@assets \
                  -sEXPORTED_RUNTIME_METHODS=['callMain']"

all_flags="$emscripten_flags \
           $common_custom_flags \
           $common_warning_flags \
           $common_compiler_flags \
           $c_standard" 


PrepareOutputDirectory
mkdir -p $output_dir/wasm

set -x
emcc $src_entry_file $all_flags -o $output_path
set +x

PrintBuildEnd
