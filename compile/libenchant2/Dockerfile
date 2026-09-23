FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBENCHANT_VERSION

RUN mkdir -p /root/lib/include /root/lib/lib
COPY ./libglib/ /root/libglib_vendor
RUN cp -r /root/libglib_vendor/jspi/dist/root/lib/* /root/lib
COPY ./libffi/ /root/libffi_vendor
RUN cp -r /root/libffi_vendor/jspi/dist/root/lib/* /root/lib
COPY ./libz/ /root/libz
RUN cp -r /root/libz/jspi/dist/root/lib/* /root/lib

RUN set -euxo pipefail && \
    source /root/emsdk/emsdk_env.sh && \
    wget https://github.com/rrthomas/enchant/releases/download/v$LIBENCHANT_VERSION/enchant-$LIBENCHANT_VERSION.tar.gz && \
    tar xf enchant-$LIBENCHANT_VERSION.tar.gz

WORKDIR /root/enchant-$LIBENCHANT_VERSION

# Pinned to 2.6.9, the last release whose own configure.ac only requires
# `glib-2.0 >= 2.6 gmodule-2.0` (2.7.0+ added gobject-2.0/gio-2.0, and
# 2.8.x's own floor jumped to glib >= 2.76 -- meson-only, well past our
# vendored 2.58.3, see compile/libglib/Dockerfile's own note). No spell-
# checking provider is vendored yet (no hunspell/aspell) -- configure warns
# ("No spell-checking provider selected!") but does not fail, so the
# extension still builds; dictionaries just won't resolve to anything
# until a real provider is added as a follow-up.
RUN source /root/emsdk/emsdk_env.sh && \
    GLIB_CFLAGS="-I/root/lib/include/glib-2.0 -I/root/lib/lib/glib-2.0/include " \
    GLIB_LIBS="-L/root/lib/lib -lgmodule-2.0 -lglib-2.0 -lffi -lz " \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --prefix=/root/install \
        --disable-shared \
        --enable-static \
        --disable-relocatable

# The real libenchant-2.la lives in src/ (src/Makefile.am's own
# lib_LTLIBRARIES), not lib/ (which only builds libgnu.la, a Gnulib
# portability shim src/ depends on -- built first here as a real
# prerequisite). Targeting src/'s own library file by explicit name avoids
# its default target, which also builds two real CLI tools (enchant-2.1,
# enchant-lsmod-2.1) that link libenchant in twice (relative + absolute
# path, same real file) -- same --whole-archive duplicate-symbol failure
# shape as openldap's apitest/dtest and glib's gobject-query.
RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C lib && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C src libenchant-2.la

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C src install-libLTLIBRARIES install-libenchant_includeHEADERS && \
    mkdir -p /root/install/lib && \
    cp /root/lib/lib/libglib-2.0.a /root/lib/lib/libgmodule-2.0.a /root/lib/lib/libffi.a /root/lib/lib/libz.a /root/install/lib/
