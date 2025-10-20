### Code shared between builds

set -e

app_name="flappy_bird"

project_root=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
output_dir="$project_root/out"
assets_output_dir_path="$output_dir/assets/"
src_entry_file="$project_root/src/sdl_main.c"


# TODO: Think about flags (-fsanitize=...,undefined)
common_custom_flags="-D FLAPPY_DEBUG"
common_warning_flags="-Wall -Wextra -Wdouble-promotion -Wconversion \
                      -Wno-unused-function  -Wno-sign-conversion"
common_compiler_flags="-fsanitize=address -g3"
common_c_standard="-std=c99"

PrintBuildStart() {
    printf "\n\033[1;33mBUILD STARTED\033[0m \n"
}

PrintBuildEnd() {
    printf "\033[32;1mBUILD SUCCESSFUL\033[0m\n"
}

PrepareOutputDirectory() {
    mkdir -p $assets_output_dir_path

    cp ./assets/*.bmp $assets_output_dir_path
}
