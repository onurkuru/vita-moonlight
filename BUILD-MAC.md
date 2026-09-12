# Building on macOS (Apple Silicon)

```bash
export VITASDK=$HOME/vitasdk-pm
export PATH=$VITASDK/bin:$PATH
# one-time deps (pacman-based vdpm):
#   yes | vdpm -f zlib bzip2 libpng libjpeg-turbo freetype libvita2d openssl curl expat opus mbedtls
git submodule update --init
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5
make -C build -j8
# -> build/moonlight.vpk
```

Host-side test of the keyboard-mode translation: `cc -std=gnu99 tests/kbm_test.c -o kbm_test && ./kbm_test`

Host setup for a Mac (Sunshine): `encoder = software`, `sw_tune = zerolatency` —
VideoToolbox does not honour on-demand IDR requests, which makes the Vita freeze on packet loss.
