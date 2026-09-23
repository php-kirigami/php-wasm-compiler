FROM kirigami-php-wasm:base

ARG JSPI
ARG UNIXODBC_VERSION

RUN set -euxo pipefail && \
    source /root/emsdk/emsdk_env.sh && \
    wget https://github.com/lurcher/unixODBC/releases/download/v$UNIXODBC_VERSION/unixODBC-$UNIXODBC_VERSION.tar.gz && \
    tar xf unixODBC-$UNIXODBC_VERSION.tar.gz

WORKDIR /root/unixODBC-$UNIXODBC_VERSION

# Driver manager only (libodbc + libodbcinst), no bundled drivers
# (--enable-drivers defaults to off already). Kept dependency-free for a
# first pass: --disable-readline (only used by the isql/iusql CLI tools,
# never built here), --disable-threads (single-threaded WASM VM, same as
# every other vendored lib), --disable-iconv (the Unicode W-function
# conversions fall back to unixODBC's own naive UCS-2 <-> ASCII copy rather
# than pulling a second iconv implementation into the side module).
# --with-included-ltdl: libltdl (the driver loader, dlopen() underneath) is
# bundled in the tarball; no system libltdl exists in the build container.
RUN source /root/emsdk/emsdk_env.sh && \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --prefix=/root/install \
        --sysconfdir=/etc \
        --disable-shared \
        --enable-static \
        --disable-readline \
        --disable-threads \
        --disable-iconv \
        --with-included-ltdl

# Build each library directory by name instead of the top-level "all",
# which also descends into exe/ (isql, iusql, odbcinst, dltest CLI tools --
# not needed, and the same --whole-archive duplicate-symbol risk as
# openldap's apitest/dtest). Order follows the top-level SUBDIRS list:
# extras/log/lst/ini/libltdl are convenience libraries odbcinst and DriverManager
# both link in.
RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    for dir in extras log lst ini libltdl odbcinst DriverManager; do \
        EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C $dir || exit 1; \
    done

# Install libodbc.a only, NOT odbcinst's own libodbcinst.a: libodbc.a
# already embeds every odbcinst object (plus extras/log/lst/ini/libltdl,
# via libtool's convenience-library merge), and cli.mjs's vendorLibFlags()
# links every *.a under lib/ with --whole-archive -- shipping both would be
# a duplicate-symbol link error. include/ installs the public headers
# (sql.h, sqlext.h, sqltypes.h, sqlucode.h, odbcinst.h, ...), but not
# unixodbc.h, which sqltypes.h #includes: that one is installed by the
# top-level Makefile (include_HEADERS there), which we never run, so it's
# copied by hand. Real failure without it: "sqltypes.h:56:10: fatal error:
# 'unixodbc.h' file not found" building ext/odbc.
RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C DriverManager install && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C include install && \
    cp unixodbc.h /root/install/include/
