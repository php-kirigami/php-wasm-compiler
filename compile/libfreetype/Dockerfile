FROM kirigami-php-wasm:base

ARG JSPI
ARG FREETYPE_VERSION

RUN mkdir -p /root/lib/include /root/lib/lib
COPY ./libz/ /root/libz
COPY ./libpng16/ /root/libpng16

RUN if [ "$JSPI" = "1" ]; then \
        cp -r /root/libz/jspi/dist/root/lib/* /root/lib; \
        cp -r /root/libpng16/jspi/dist/root/lib/* /root/lib; \
    else \
        cp -r /root/libz/asyncify/dist/root/lib/* /root/lib; \
        cp -r /root/libpng16/asyncify/dist/root/lib/* /root/lib; \
    fi

RUN wget https://github.com/freetype/freetype/archive/refs/tags/VER-$(echo $FREETYPE_VERSION | tr . -).tar.gz -O freetype.tar.gz
RUN tar -xzf freetype.tar.gz
RUN mv freetype-VER-$(echo $FREETYPE_VERSION | tr . -) freetype-$FREETYPE_VERSION
WORKDIR /root/freetype-$FREETYPE_VERSION

# Only zlib/libpng support (matches what GD actually needs) — no
# HarfBuzz/Brotli/bzip2 (not vendored, not needed by GD's own usage).
RUN set -euxo pipefail; \
    source /root/emsdk/emsdk_env.sh; \
    mkdir -p build && cd build && \
    emcmake cmake -G"Unix Makefiles" .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/root/install \
        -DBUILD_SHARED_LIBS=OFF \
        -DFT_DISABLE_ZLIB=OFF \
        -DFT_REQUIRE_ZLIB=ON \
        -DFT_DISABLE_PNG=OFF \
        -DFT_REQUIRE_PNG=ON \
        -DFT_DISABLE_HARFBUZZ=ON \
        -DFT_DISABLE_BROTLI=ON \
        -DFT_DISABLE_BZIP2=ON \
        -DZLIB_LIBRARY=/root/lib/lib/libz.a \
        -DZLIB_INCLUDE_DIR=/root/lib/include \
        -DPNG_LIBRARY=/root/lib/lib/libpng16.a \
        -DPNG_PNG_INCLUDE_DIR=/root/lib/include

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_SKIP="-lc -lz -lpng -lpng16" \
    EMCC_FLAGS=" -D__x86_64__ -sSIDE_MODULE $JSPI_FLAGS" \
    emmake make -C build

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_SKIP="-lc -lz -lpng -lpng16" \
    EMCC_FLAGS=" -D__x86_64__ -sSIDE_MODULE $JSPI_FLAGS" \
    emmake make -C build install
