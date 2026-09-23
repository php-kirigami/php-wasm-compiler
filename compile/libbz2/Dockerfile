FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBBZIP2_VERSION

RUN set -euxo pipefail && \
    source /root/emsdk/emsdk_env.sh && \
    wget https://sourceware.org/pub/bzip2/bzip2-$LIBBZIP2_VERSION.tar.gz && \
    tar xf bzip2-$LIBBZIP2_VERSION.tar.gz

WORKDIR /root/bzip2-$LIBBZIP2_VERSION

# No autoconf here — upstream bzip2 ships a plain, hand-written Makefile with
# CC/AR/RANLIB left overridable for cross-compiling (its own top comment:
# "To assist in cross-compiling"). No configure step, so no --host/emconfigure
# needed either, unlike every autoconf-based lib in this pipeline (sodium,
# libssh2, ...). Building the `libbz2.a` target only, not the default `all`
# (which also builds and runs the `bzip2`/`bzip2recover` CLI binaries and a
# `test` target that would try to execute cross-compiled wasm binaries on the
# build host).
RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    emmake make libbz2.a \
        CC=emcc \
        AR=emar \
        RANLIB=emranlib \
        CFLAGS="-D_FILE_OFFSET_BITS=64 -fPIC -O2 $JSPI_FLAGS"

# No `make install` (its own PREFIX-based install target also installs the
# CLI binaries/man pages we don't want) — stage just the two files the
# `bz2` PHP extension's config.m4 actually needs (bzlib.h, libbz2.a) into
# the same install/include + install/lib layout every other vendored lib in
# this pipeline produces.
RUN mkdir -p /root/install/include /root/install/lib && \
    cp bzlib.h /root/install/include/ && \
    cp libbz2.a /root/install/lib/
