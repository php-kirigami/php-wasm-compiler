FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBWEBP_VERSION

RUN mkdir -p /root/lib/include /root/lib/lib
COPY ./libz/ /root/libz

RUN if [ "$JSPI" = "1" ]; then \
        cp -r /root/libz/jspi/dist/root/lib/* /root/lib; \
    else \
        cp -r /root/libz/asyncify/dist/root/lib/* /root/lib; \
    fi

RUN wget https://chromium.googlesource.com/webm/libwebp/+archive/$LIBWEBP_VERSION.tar.gz -O libwebp.tar.gz
RUN tar -xzf libwebp.tar.gz
RUN     autoreconf --install
RUN     autoconf
RUN     source /root/emsdk/emsdk_env.sh && \
        CPPFLAGS="-I/root/lib/include " \
        LDFLAGS="-L/root/lib/lib " \
        emconfigure ./configure \
            PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
            --build i386-pc-linux-gnu \
            --host wasm32-unknown-emscripten \
            --target wasm32-unknown-emscripten \
            --prefix=/root/lib/
# Build/install only "sharpyuv" and "src" directly (each has its own
# generated Makefile with its own correct nested SUBDIRS), instead of the
# top-level recursive `make`/`make install`, which would also descend into
# "imageio"/"examples"/"man"/"extras". We only need libwebp.a/libsharpyuv.a
# (and the demux/mux .a files nested under src/), not the cwebp/dwebp/etc.
# CLI tools. Building those examples fails under wasm-ld with "duplicate
# symbol" errors (libwebp.a ends up listed twice on the link line — once via
# an absolute-ish path, once via a relative one — which wasm-ld's
# --whole-archive linking rejects, unlike a traditional linker).
# (A command-line `SUBDIRS=` override on the top-level `make` was tried
# first, but that propagates into every nested Makefile too, breaking
# src/Makefile's own unrelated SUBDIRS list.)
RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_SKIP="-lc -lz" EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C sharpyuv && \
    EMCC_SKIP="-lc -lz" EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C src
RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C sharpyuv install && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C src install
# @TODO: Do not remove dependencies like this – it's too easy to miss something.
#        Instead, source them from another directory and end up with a clean build.
# Note: Keep pkgconfig files for libwebp so ImageMagick can detect it
RUN rm -rf /root/lib/lib/libz.a /root/lib/include/zconf.h /root/lib/include/zlib.h && \
    rm -f /root/lib/lib/pkgconfig/zlib.pc