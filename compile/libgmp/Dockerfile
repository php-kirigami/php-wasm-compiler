FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBGMP_VERSION

RUN set -euxo pipefail && \
    source /root/emsdk/emsdk_env.sh && \
    wget https://gmplib.org/download/gmp/gmp-$LIBGMP_VERSION.tar.xz && \
    tar xf gmp-$LIBGMP_VERSION.tar.xz

WORKDIR /root/gmp-$LIBGMP_VERSION

# GMP's own configure dispatches per-CPU hand-written assembly (mpn/<cpu>/...)
# via a big "case $host in" table in its acinclude.m4/configure.ac -- unlike
# a normal autoconf project, an unrecognized $host_cpu (wasm32 isn't in that
# table) isn't a hard error, it falls through to the portable mpn/generic C
# implementation on its own (slower, never wrong -- exactly what we want
# here, same tradeoff --disable-asm makes explicit for libsodium above).
# --host still needs to be a triplet config.sub itself recognizes, which
# recent (2020+) GNU config.sub does for wasm32 -- GMP 6.3.0 ships one new
# enough.
RUN source /root/emsdk/emsdk_env.sh && \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --prefix=/root/install \
        --disable-shared \
        --enable-static

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make install
