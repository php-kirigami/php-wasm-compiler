FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBCMARKGFM_VERSION

RUN wget https://github.com/github/cmark-gfm/archive/refs/tags/$LIBCMARKGFM_VERSION.tar.gz -O cmark-gfm.tar.gz
RUN tar -xzf cmark-gfm.tar.gz
WORKDIR /root/cmark-gfm-$LIBCMARKGFM_VERSION

# Build only "libcmark-gfm_static" and "libcmark-gfm-extensions_static"
# (add_library in src/CMakeLists.txt / extensions/CMakeLists.txt), not the
# default "all" target — that also includes a "cmark-gfm" CLI executable
# we don't need, which doesn't make sense as a -sSIDE_MODULE build (side
# modules aren't standalone executables). BUILD_TESTING=OFF skips
# api_test/test (CTest's include(CTest) defaults it ON).
#
# -DCMAKE_POLICY_VERSION_MINIMUM=3.5: cmark-gfm's own CMakeLists.txt
# still declares a cmake_minimum_required() predating CMake 3.5, which
# newer CMake (>=4.0) refuses outright rather than just warning about —
# confirmed needed against a very new local CMake (4.2.3) during
# php-mdhtml's own native vendoring of this same library; harmless to
# pass unconditionally even if this base image's apt-installed CMake
# turns out old enough not to need it (an unrecognized cache variable on
# an older CMake is silently ignored, not an error).
RUN mkdir -p build && cd build && \
    source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    emcmake cmake -G"Unix Makefiles" .. \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/root/install \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_TESTING=OFF \
        -DCMAKE_C_FLAGS="-sSIDE_MODULE $JSPI_FLAGS"

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C build libcmark-gfm_static libcmark-gfm-extensions_static

RUN mkdir -p /root/install/lib /root/install/include && \
    CORE_LIB=$(find build/src -maxdepth 1 -name "libcmark-gfm.a" | head -n1) && \
    EXT_LIB=$(find build/extensions -maxdepth 1 -name "libcmark-gfm-extensions.a" | head -n1) && \
    if [ -z "$CORE_LIB" ]; then echo "libcmark-gfm.a not found"; exit 1; fi && \
    if [ -z "$EXT_LIB" ]; then echo "libcmark-gfm-extensions.a not found"; exit 1; fi && \
    cp "$CORE_LIB" /root/install/lib/libcmark-gfm.a && \
    cp "$EXT_LIB" /root/install/lib/libcmark-gfm-extensions.a && \
    cp src/cmark-gfm.h /root/install/include/ && \
    cp src/cmark-gfm-extension_api.h /root/install/include/ && \
    cp build/src/cmark-gfm_export.h /root/install/include/ && \
    cp build/src/cmark-gfm_version.h /root/install/include/ && \
    cp extensions/cmark-gfm-core-extensions.h /root/install/include/
