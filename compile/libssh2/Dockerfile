FROM kirigami-php-wasm:base

ARG JSPI

RUN mkdir -p /root/lib/include /root/lib/lib
COPY ./libz/ /root/libz
RUN cp -r /root/libz/jspi/dist/root/lib/* /root/lib

COPY ./libopenssl/ /root/libopenssl
RUN cp -r /root/libopenssl/jspi/dist/root/lib/* /root/lib

ARG LIBSSH2_VERSION

RUN set -euxo pipefail && \
    source /root/emsdk/emsdk_env.sh && \
    wget https://github.com/libssh2/libssh2/releases/download/libssh2-$LIBSSH2_VERSION/libssh2-$LIBSSH2_VERSION.tar.gz && \
    tar xf libssh2-$LIBSSH2_VERSION.tar.gz

WORKDIR /root/libssh2-$LIBSSH2_VERSION

# The release tarball ships a pre-generated `configure` (unlike libwebp/
# libxml2/oniguruma, which regenerate theirs via autoreconf/autogen.sh and
# therefore need an explicit --host to detect cross-compilation) — but we
# pass --host anyway, for consistency and safety.
RUN source /root/emsdk/emsdk_env.sh && \
    CPPFLAGS="-I/root/lib/include " \
    LDFLAGS="-L/root/lib/lib " \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --prefix=/root/install \
        --disable-shared \
        --enable-static \
        --with-libz \
        --with-libz-prefix=/root/lib \
        --with-crypto=openssl \
        --with-libssl-prefix=/root/lib \
        --disable-examples-build

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_SKIP="-lz -lssl -lcrypto" EMCC_FLAGS=" -D__x86_64__ -sSIDE_MODULE $JSPI_FLAGS" emmake make

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_SKIP="-lz -lssl -lcrypto" EMCC_FLAGS=" -D__x86_64__ -sSIDE_MODULE $JSPI_FLAGS" emmake make install
