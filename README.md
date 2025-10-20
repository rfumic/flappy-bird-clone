## macOS
Dependencies:

    - bash
    - clang
    - sdl3
    - pkg-config

Build:
```sh
./build_debug_macos.sh
```
Run executable in `./out/` folder

## Emscripten (WASM)
Install emscripten sdk and compile:
```sh
./build_emscripten.sh
```

Go to `./out/wasm/` and run:
```sh
emrun --no-browser index.html
```

