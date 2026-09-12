FROM kirigami-php-wasm:base

ARG JSPI

ARG NGHTTP2_VERSION

RUN set -euxo pipefail && \
    source /root/emsdk/emsdk_env.sh && \
    wget https://github.com/nghttp2/nghttp2/releases/download/v$NGHTTP2_VERSION/nghttp2-$NGHTTP2_VERSION.tar.gz && \
    tar xf nghttp2-$NGHTTP2_VERSION.tar.gz

WORKDIR /root/nghttp2-$NGHTTP2_VERSION

# The release tarball ships a pre-generated `configure`, but we pass --host
# anyway for consistency/safety (see CLAUDE.md decision 24's libssh2 note).
# --enable-lib-only skips the nghttp2 CLI apps/examples/python bindings,
# which pull in unrelated deps (libcurl, libev, jansson, boost...) we don't
# need — we only want libnghttp2.a for curl's HTTP/2 support.
RUN source /root/emsdk/emsdk_env.sh && \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --prefix=/root/install \
        --disable-shared \
        --enable-static \
        --enable-lib-only

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -D__x86_64__ -sSIDE_MODULE $JSPI_FLAGS" emmake make

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -D__x86_64__ -sSIDE_MODULE $JSPI_FLAGS" emmake make install
