FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBXML2_VERSION

RUN source /root/emsdk/emsdk_env.sh && \
    env GIT_SSL_NO_VERIFY=true git clone https://gitlab.gnome.org/GNOME/libxml2.git libxml2 \
        --branch $LIBXML2_VERSION \
        --single-branch     \
        --depth 1 && \
    cd libxml2 && \
    ./autogen.sh && \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --target wasm32-unknown-emscripten \
        --with-http=no --with-ftp=no --with-python=no --with-threads=no --enable-shared=no --prefix=/root/lib/ &&\
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -D__x86_64__  -sSIDE_MODULE $JSPI_FLAGS" emmake make && \
    emmake make install
