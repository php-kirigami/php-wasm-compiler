FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBYAML_VERSION

RUN set -euxo pipefail && \
    source /root/emsdk/emsdk_env.sh && \
    wget https://github.com/yaml/libyaml/releases/download/$LIBYAML_VERSION/yaml-$LIBYAML_VERSION.tar.gz && \
    tar xf yaml-$LIBYAML_VERSION.tar.gz

WORKDIR /root/yaml-$LIBYAML_VERSION

# The bundled config.sub (from whenever 0.2.5 was released) predates
# wasm32-unknown-emscripten and rejects it outright ("config.sub
# wasm32-unknown-emscripten failed") — unlike every other lib in this
# pipeline so far, which happened to ship a recent enough one. Replace it
# with the base image's own (Ubuntu Noble's autoconf/automake package,
# already confirmed to recognize the triplet) rather than fetching one over
# the network at build time.
RUN cp /usr/share/misc/config.sub config/config.sub

# The release tarball ships a pre-generated `configure` (same as libssh2/
# nghttp2/libsodium) — still pass --host for consistency and safety.
RUN source /root/emsdk/emsdk_env.sh && \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --prefix=/root/install \
        --disable-shared \
        --enable-static

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -D__x86_64__ -sSIDE_MODULE $JSPI_FLAGS" emmake make

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -D__x86_64__ -sSIDE_MODULE $JSPI_FLAGS" emmake make install
