FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBSODIUM_VERSION

RUN set -euxo pipefail && \
    source /root/emsdk/emsdk_env.sh && \
    wget https://github.com/jedisct1/libsodium/releases/download/$LIBSODIUM_VERSION-RELEASE/libsodium-$LIBSODIUM_VERSION.tar.gz && \
    tar xf libsodium-$LIBSODIUM_VERSION.tar.gz

WORKDIR /root/libsodium-$LIBSODIUM_VERSION

# The release tarball ships a pre-generated `configure` (same as libssh2) —
# still pass --host for consistency and safety. --disable-asm/--disable-ssp/
# --disable-pie mirror libsodium's own dist-build/emscripten.sh: the x86 asm
# optimizations, stack-smashing protection, and PIE binaries it would
# otherwise try to enable either don't apply or don't cross-compile to wasm.
RUN source /root/emsdk/emsdk_env.sh && \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --prefix=/root/install \
        --disable-shared \
        --enable-static \
        --disable-asm \
        --disable-ssp \
        --disable-pie

# No -D__x86_64__ here, unlike every other lib in this pipeline: libsodium's
# runtime.c (_cpuid()) and ed25519_ref10.c/core.c gate real inline __asm__
# blocks (raw "cpuid", x86 register constraints like "q") behind
# `defined(__x86_64__)`, not just intrinsics Emscripten can lower to wasm
# SIMD. Defining it here doesn't unlock a useful fast path like it does for
# SIMD-intrinsics-based libs (libjpeg, libwebp) — it just makes libsodium
# think it's really running on x86_64 and pulls in asm with no wasm
# equivalent ("invalid input constraint 'q' in asm"). Without it, Emscripten's
# target naturally doesn't define __x86_64__/__i386__, so libsodium falls
# through to its portable ref/ref10 implementations, which is what we want.
RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make install
