#!/bin/bash
source ./build_common.sh

PrintBuildStart

executable_output_path="$output_dir/$app_name"

sdl3_cflags=$(pkg-config --cflags sdl3)
sdl3_ldflags=$(pkg-config --libs sdl3)
macos_flags="-framework CoreVideo -framework IOKit -framework Cocoa \
-framework CoreAudio"

all_flags="$macos_flags \
           $sdl3_cflags \
           $sdl3_ldflags \
           $common_custom_flags \
           $common_warning_flags \
           $common_compiler_flags \
           $common_c_standard"

PrepareOutputDirectory

set -x
clang $src_entry_file $all_flags -o $executable_output_path
set +x

PrintBuildEnd
