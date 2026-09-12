FROM kirigami-php-wasm:base

ARG JSPI

RUN mkdir -p /root/lib/include /root/lib/lib
COPY ./libz/ /root/libz
RUN if [ "$JSPI" = "1" ]; then \
        cp -r /root/libz/jspi/dist/root/lib/* /root/lib; \
    else \
        cp -r /root/libz/asyncify/dist/root/lib/* /root/lib; \
    fi

COPY ./libopenssl/ /root/libopenssl
RUN if [ "$JSPI" = "1" ]; then \
        cp -r /root/libopenssl/jspi/dist/root/lib/* /root/lib; \
    else \
        cp -r /root/libopenssl/asyncify/dist/root/lib/* /root/lib; \
    fi

COPY ./libssh2/ /root/libssh2
RUN cp -r /root/libssh2/jspi/dist/root/lib/* /root/lib

COPY ./nghttp2/ /root/nghttp2
RUN cp -r /root/nghttp2/jspi/dist/root/lib/* /root/lib


ARG CURL_VERSION

RUN /root/copy-lib.sh lib-libz
RUN set -euxo pipefail && \
    source /root/emsdk/emsdk_env.sh && \
    wget https://curl.haxx.se/download/$CURL_VERSION.tar.gz && \
    tar xf $CURL_VERSION.tar.gz

WORKDIR /root/$CURL_VERSION

RUN CPPFLAGS="-I/root/lib/include " \
    LDFLAGS="-L/root/lib/lib " \
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
    source /root/emsdk/emsdk_env.sh && \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --target wasm32-unknown-emscripten \
        --prefix=/root/lib/ \
        --disable-shared \
        --enable-static \
        --with-ssl \
        --with-openssl=/root/lib \
        --with-libssh2=/root/lib \
        --with-nghttp2=/root/lib \
        --without-libpsl \
        --enable-https \
        --enable-http \
        --disable-pop3 \
        --disable-imap \
        --disable-smb \
        --disable-smtp \
        --disable-telnet \
        --disable-gopher \
        --disable-ftp \
        --disable-ftps \
        --disable-rtsp \
        --disable-tftp \
        --disable-pthreads \
        --disable-threaded-resolver \
        --with-zlib=/root/lib

# JSPI-only build (see CLAUDE.md decision 6/24 — no Asyncify variant here).
RUN source /root/emsdk/emsdk_env.sh && \
    EMCC_SKIP="-lc -lz -lcurl -lssl -lssh2 -lnghttp2 " \
    EMCC_FLAGS=" -D__x86_64__ -sSIDE_MODULE -Wl,--wrap=select -sSUPPORT_LONGJMP=wasm -fwasm-exceptions " emmake make || true
