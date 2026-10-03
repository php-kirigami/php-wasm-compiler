set -euxo pipefail
source /root/.cargo/env
source /root/emsdk/emsdk_env.sh
cd /src
[ -d xberg ] || git clone --depth 1 --branch v1.3.2 https://github.com/xberg-io/xberg.git
cd xberg
rm -f rust-toolchain.toml
cp /work/Cargo.xberg-php.toml crates/xberg-php/Cargo.toml
# --- take the native paths on emscripten: neutralise the browser-wasm cfgs ---
for d in crates/xberg crates/xberg-candle-ocr crates/xberg-gliner crates/xberg-native-pdf crates/xberg-php; do
  grep -rlE 'target_arch = "wasm32"|target_family = "wasm"' $d --include=*.rs --include=Cargo.toml | xargs -r sed -i -E 's/target_arch = "wasm32"/target_arch = "wasm32_off"/g; s/target_family = "wasm"/target_family = "wasm_off"/g' || true
done
sed -i 's/new_multi_thread()/new_current_thread()/' crates/xberg-php/src/lib.rs
printf '%s\n' '#!/bin/sh' 'echo "PHP Version => 8.5.11"' 'echo "PHP API => 20250925"' 'echo "Thread Safety => disabled"' 'echo "Debug Build => no"' > /usr/local/bin/php-wasm-info
chmod +x /usr/local/bin/php-wasm-info
cargo fetch
P=$(ls /root/.cargo/registry/src/*/ext-php-rs-0.16*/src/internal/property.rs | head -1)
sed -i 's/<= 12 \* std::mem::size_of::<usize>()/<= 24 * std::mem::size_of::<usize>()/' "$P"

# --- mio: no epoll/eventfd in libc's emscripten bindings; use its poll(2) selector ---
rm -rf /root/.cargo/registry/src/*/mio-1.2.3
cargo fetch
MIO=$(ls -d /root/.cargo/registry/src/*/mio-1.2.3 | head -1)
sed -i 's/#\[cfg(all(target_family = "wasm", not(target_os = "wasi")))\]/#[cfg(all(target_family = "wasm", not(target_os = "wasi"), not(target_os = "emscripten")))]/' $MIO/src/lib.rs
grep -n 'emscripten' $MIO/src/lib.rs
perl -0pi -e 's/all\(target_os = "wasi", target_env = "p1"\)\n(\s*)\), path = "waker\/pipe.rs"\)\]/all(target_os = "wasi", target_env = "p1"),\n$1    target_os = "emscripten"\n$1), path = "waker\/pipe.rs")]/' $MIO/src/sys/unix/mod.rs
perl -0pi -e 's/(target_os = "horizon",\n)(\s*)(all\(target_arch = "x86", target_os = "android"\),\n\s*\)\)\]\n\s*let stream)/$1$2target_os = "emscripten",\n$2$3/' $MIO/src/sys/unix/tcp.rs
grep -rn 'emscripten' $MIO/src | cut -c1-200
# --- tokio: take the native (unix) paths on emscripten: net/socket2 etc. ---
rm -rf /src/patches/tokio && mkdir -p /src/patches
cp -r $(ls -d /root/.cargo/registry/src/*/tokio-1.53.1 | head -1) /src/patches/tokio
grep -rl 'target_family = "wasm"' /src/patches/tokio/src /src/patches/tokio/Cargo.toml | xargs sed -i 's/target_family = "wasm"/target_family = "wasm_off"/g' || true
perl -0pi -e 's/(#\[cfg\(any\(\n\s*target_os = "espidf",\n\s*target_os = "nuttx",\n\s*target_os = "vita",\n\s*target_os = "hurd")(\n\s*\)\)\]\npub\(crate\) use self::impl_noproc)/$1,\n    target_os = "emscripten"$2/; s/(#\[cfg\(any\(\n\s*target_os = "espidf",\n\s*target_os = "nuttx",\n\s*target_os = "vita",\n\s*target_os = "hurd")(\n\s*\)\)\]\npub\(crate\) mod impl_noproc)/$1,\n    target_os = "emscripten"$2/' /src/patches/tokio/src/net/unix/ucred.rs
grep -n emscripten /src/patches/tokio/src/net/unix/ucred.rs
grep -q 'patches/tokio' Cargo.toml || sed -i 's#^\[patch.crates-io\]#[patch.crates-io]\ntokio = { path = "/src/patches/tokio" }#' Cargo.toml
grep -n -A1 '^\[patch.crates-io\]' Cargo.toml
cargo fetch
export PHP=/usr/local/bin/php-wasm-info PHP_CONFIG=/usr/local/bin/php-config
export BINDGEN_EXTRA_CLANG_ARGS="--target=wasm32-unknown-emscripten --sysroot=/root/emsdk/upstream/emscripten/cache/sysroot -DZEND_ENABLE_ZVAL_LONG64 -D__x86_64__"
export CC_wasm32_unknown_emscripten=emcc AR_wasm32_unknown_emscripten=emar
export CFLAGS_wasm32_unknown_emscripten="-fPIC -fwasm-exceptions -sSUPPORT_LONGJMP=wasm"
export RUSTFLAGS="-C panic=abort -C relocation-model=pic --cfg mio_unsupported_force_poll_poll"
export CARGO_PROFILE_RELEASE_LTO=off CARGO_PROFILE_RELEASE_CODEGEN_UNITS=16 CARGO_PROFILE_RELEASE_OPT_LEVEL=s CARGO_PROFILE_RELEASE_STRIP=false
export LIBCLANG_PATH=/usr/lib/llvm-18/lib
# --- hf-hub's xet crates: same wasm cfg neutralisation, through [patch] copies ---
for c in xet-client xet-core-structures xet-data xet-runtime hf-xet; do
  dir=$(ls -d /root/.cargo/registry/src/*/$c-[0-9]* | head -1)
  rm -rf /src/patches/$c && cp -r $dir /src/patches/$c
  grep -rlE 'target_arch = "wasm32"|target_family = "wasm"' /src/patches/$c --include=*.rs --include=Cargo.toml | xargs -r sed -i -E 's/target_arch = "wasm32"/target_arch = "wasm32_off"/g; s/target_family = "wasm"/target_family = "wasm_off"/g' || true
  grep -q "patches/$c\"" Cargo.toml || sed -i "s#^\[patch.crates-io\]#[patch.crates-io]\n$c = { path = \"/src/patches/$c\" }#" Cargo.toml
done
grep -n -A8 '^\[patch.crates-io\]' Cargo.toml
cargo fetch
# --- ring: no SystemRandom for emscripten; use getrandom like linux does ---
RING=$(ls -d /root/.cargo/registry/src/*/ring-0.17.14 | head -1)
grep -q 'target_os = "emscripten"' $RING/src/rand.rs || sed -i '0,/    target_os = "aix",/s//    target_os = "aix",\n    target_os = "emscripten",/' $RING/src/rand.rs
grep -n emscripten $RING/src/rand.rs
cargo clean -p ring --release --target wasm32-unknown-emscripten || true
export TSLP_LANGUAGES=json,yaml,toml,html,css,javascript,typescript,python,rust,c,cpp,go,java,bash,sql,markdown TSLP_ALLOW_FAILED_GRAMMARS=1
cargo build --release -p xberg-php --target wasm32-unknown-emscripten -Zbuild-std=std,panic_abort -j2
ls -la target/wasm32-unknown-emscripten/release/*.a
