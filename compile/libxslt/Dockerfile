FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBXSLT_VERSION

# Built against the same vendored libxml2 the core links (compile/libxml2).
# Its whole install tree is copied in: headers for compiling, and
# libxml2.a, which is also re-installed next to libxslt.a below.
RUN mkdir -p /root/lib
COPY ./libxml2/ /root/libxml2_vendor
RUN cp -r /root/libxml2_vendor/jspi/dist/root/lib/* /root/lib

RUN set -euxo pipefail && \
    source /root/emsdk/emsdk_env.sh && \
    wget https://download.gnome.org/sources/libxslt/${LIBXSLT_VERSION%.*}/libxslt-$LIBXSLT_VERSION.tar.xz && \
    tar xf libxslt-$LIBXSLT_VERSION.tar.xz

WORKDIR /root/libxslt-$LIBXSLT_VERSION

# LIBXML_CFLAGS/LIBXML_LIBS set explicitly: configure then skips both its
# xml2-config and pkg-config probes (configure.ac only runs them when
# LIBXML_LIBS is empty). LIBXML_LIBS is an -L/-l pair, not the archive's
# path: given a path, libtool copies libxml2.a whole into libxslt.a and
# libexslt.a as a member ("archive member 'libxml2.a' is neither Wasm
# object file nor LLVM bitcode" at xsl.so's link). --without-python: no
# bindings.
# --without-crypto: EXSLT's crypto:* functions need libgcrypt, not
# vendored. --without-debugger: xsltproc's interactive debugger, and with
# it the debugger hooks in libxslt itself. --without-plugins: extension
# modules loaded with dlopen() at runtime.
RUN source /root/emsdk/emsdk_env.sh && \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --prefix=/root/install \
        --disable-shared \
        --enable-static \
        --without-python \
        --without-crypto \
        --without-debugger \
        --without-plugins \
        LIBXML_CFLAGS="-I/root/lib/include/libxml2" \
        LIBXML_LIBS="-L/root/lib/lib -lxml2"

# The two libraries only (libxslt, then libexslt, which depends on it), by
# directory: the top-level "all" also builds xsltproc and the docs.
RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C libxslt && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C libexslt && \
    emmake make -C libxslt install && \
    emmake make -C libexslt install

# ext/xsl's config.m4 has one PKG_CHECK_MODULES per library (XSL, EXSLT)
# and no PHP_SETUP_LIBXML, so this vendorLib has to be self-contained, the
# same "one self-contained vendorLib" pattern as libenchant2: libxml2.a and
# its headers are installed next to libxslt's own. xsl.so then carries its
# own copy of libxml2 2.15.4 (as soap.so does), the same version the core
# links statically. See compile/extensions/xsl/PROVENANCE.md for what that
# means for documents shared with ext/dom.
RUN cp /root/lib/lib/libxml2.a /root/install/lib/ && \
    cp -r /root/lib/include/libxml2 /root/install/include/
