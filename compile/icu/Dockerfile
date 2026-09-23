FROM kirigami-php-wasm:base

ARG JSPI
ARG ICU_VERSION

RUN set -euxo pipefail && \
    wget https://github.com/unicode-org/icu/releases/download/release-$ICU_VERSION/icu4c-$ICU_VERSION-sources.tgz && \
    tar xf icu4c-$ICU_VERSION-sources.tgz && \
    rm icu4c-$ICU_VERSION-sources.tgz

# Pass 1, native: ICU cross-compiles against a native build of itself
# (--with-cross-build), and genccode (used below) is one of its tools.
# The data isn't rebuilt from source: configure finds the release's
# prebuilt data/in/icudt<major>l.dat (little-endian ASCII, the flavour
# wasm32 reads) and only repackages it. When that prebuilt file already
# matches the target, data/Makefile writes out/tmp/icudata.lst without
# creating out/tmp first (its ICUDATA_SOURCE_IS_NATIVE_TARGET branch has
# no MKINSTALLDIRS: "out/tmp/icudata.lst: No such file or directory"), so
# the directory is created up front.
RUN set -euxo pipefail && \
    mkdir /root/icu-native && cd /root/icu-native && \
    CC=gcc CXX=g++ /root/icu/source/configure \
        --disable-tests \
        --disable-samples \
        --with-data-packaging=archive && \
    mkdir -p data/out/tmp && \
    make -j$(nproc)

# ICU's configure (aclocal.m4's ICU_CHECK_MH_FRAG) knows no emscripten
# host and falls back to config/mh-unknown, a placeholder whose every rule
# fails on purpose ("configure could not detect your platform"). Its own
# comment says to copy mh-linux (generic) over it, done here.
RUN cp /root/icu/source/config/mh-linux /root/icu/source/config/mh-unknown

# Pass 2, wasm32: libicuuc (common), libicui18n and libicuio only (the
# three ext/intl's PHP_SETUP_ICU asks for: icu-uc, icu-i18n, icu-io). No
# tools, tests, samples or extras. --disable-dyload: no plugin loading with
# dlopen(). --with-data-packaging=static: libicuuc then expects the data
# linked in, under the icudt<major>_dat entry point (built below). Built
# per directory, so the lib/ and bin/ output directories the top-level
# make would create are made by hand ("llvm-ar: ../lib/libicuuc.a: No such
# file or directory" otherwise).
RUN source /root/emsdk/emsdk_env.sh && \
    mkdir /root/icu-wasm && cd /root/icu-wasm && \
    emconfigure /root/icu/source/configure \
        --build x86_64-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --prefix=/root/install \
        --with-cross-build=/root/icu-native \
        --enable-static \
        --disable-shared \
        --disable-tools \
        --disable-tests \
        --disable-samples \
        --disable-extras \
        --disable-dyload \
        --with-data-packaging=static

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    cd /root/icu-wasm && \
    mkdir -p lib bin && \
    for dir in common i18n io; do \
        EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C $dir || exit 1; \
        emmake make -C $dir install || exit 1; \
    done && \
    emmake make install-pkgconfig

# The data: the release's prebuilt .dat file (source/data/in), turned
# into C by the native genccode (an array under the icudt<major>_dat symbol
# libicuuc looks for), compiled for wasm32 and archived as libicudata.a. About 32 MB of data,
# linked into intl.so (decided 2026-09-23: data inside the .so rather than
# a separate .dat file loaded at runtime).
RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    ICU_MAJOR=${ICU_VERSION%%.*} && \
    mkdir /root/icu-data && \
    LD_LIBRARY_PATH=/root/icu-native/lib:/root/icu-native/stubdata /root/icu-native/bin/genccode \
        -d /root/icu-data -e icudt$ICU_MAJOR /root/icu/source/data/in/icudt${ICU_MAJOR}l.dat && \
    emcc -fPIC $JSPI_FLAGS -I/root/install/include -c /root/icu-data/icudt${ICU_MAJOR}l_dat.c -o /root/icu-data/icudata.o && \
    emar rcs /root/install/lib/libicudata.a /root/icu-data/icudata.o

# ICU and ext/intl are C++, and the core has no C++ runtime (see
# compile/firebird/Dockerfile for the full reasoning): PIC libc++/
# libc++abi/libunwind are staged with the ICU archives, so intl.so links
# its own.
RUN source /root/emsdk/emsdk_env.sh && \
    embuilder --pic build libc++-legacyexcept libc++abi-legacyexcept libunwind-legacyexcept && \
    PIC_DIR=/root/emsdk/upstream/emscripten/cache/sysroot/lib/wasm32-emscripten/pic && \
    cp $PIC_DIR/libc++-legacyexcept.a /root/install/lib/libc++.a && \
    cp $PIC_DIR/libc++abi-legacyexcept.a /root/install/lib/libc++abi.a && \
    cp $PIC_DIR/libunwind-legacyexcept.a /root/install/lib/libunwind.a
