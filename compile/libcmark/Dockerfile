FROM kirigami-php-wasm:base

ARG JSPI
ARG CMARK_VERSION

RUN wget https://github.com/commonmark/cmark/archive/refs/tags/$CMARK_VERSION.tar.gz -O cmark.tar.gz
RUN tar -xzf cmark.tar.gz
WORKDIR /root/cmark-$CMARK_VERSION

# Build only the "cmark" library target (add_library in src/CMakeLists.txt),
# not the default "all" target — that also includes a "cmark_exe" CLI
# executable we don't need, which doesn't make sense as a -sSIDE_MODULE
# build (side modules aren't standalone executables). BUILD_TESTING=OFF
# skips api_test/test (CTest's include(CTest) defaults it ON).
RUN mkdir -p build && cd build && \
    source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    emcmake cmake -G"Unix Makefiles" .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/root/install \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_TESTING=OFF \
        -DCMAKE_C_FLAGS="-sSIDE_MODULE $JSPI_FLAGS"

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C build cmark

RUN mkdir -p /root/install/lib /root/install/include && \
    LIBFILE=$(find /root/cmark-$CMARK_VERSION/build -maxdepth 4 -name "libcmark*.a" | head -n1) && \
    if [ -z "$LIBFILE" ]; then echo "libcmark static archive not found"; exit 1; fi && \
    cp "$LIBFILE" /root/install/lib/libcmark.a && \
    cp /root/cmark-$CMARK_VERSION/src/cmark.h /root/install/include/ && \
    cp /root/cmark-$CMARK_VERSION/build/src/cmark_export.h /root/install/include/ && \
    cp /root/cmark-$CMARK_VERSION/build/src/cmark_version.h /root/install/include/
