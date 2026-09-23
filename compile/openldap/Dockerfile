FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBOPENLDAP_VERSION

RUN set -euxo pipefail && \
    source /root/emsdk/emsdk_env.sh && \
    wget https://www.openldap.org/software/download/OpenLDAP/openldap-release/openldap-$LIBOPENLDAP_VERSION.tgz && \
    tar xf openldap-$LIBOPENLDAP_VERSION.tgz

WORKDIR /root/openldap-$LIBOPENLDAP_VERSION

# --disable-slapd: only the client libraries (liblber/libldap) are needed --
# ext/ldap is a client, never the standalone slapd server. --without-cyrus-
# sasl / --without-tls: keep this first pass dependency-free, matching
# libpq's own --without-openssl (ext/ldap's own SASL support is opt-in via
# --with-ldap-sasl anyway, defaulting to off -- see config.yaml's ldap
# entry) -- a possible follow-up, same shape as mysqlnd's own SSL addition
# (decision 45's follow-up).
RUN source /root/emsdk/emsdk_env.sh && \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --prefix=/root/install \
        --disable-shared \
        --enable-static \
        --disable-slapd \
        --without-cyrus-sasl \
        --without-tls \
        --with-yielding_select=yes

# "make -C libraries" alone (bypassing the top-level Makefile) skips a
# top-level prerequisite: include/ldap_config.h (generated from
# ldap_config.h.in via config.status as part of the top-level "make depend"
# dependency chain, not by ./configure itself) -- real failure without this:
# "fatal error: 'ldap_config.h' file not found" from ldap_defaults.h.
# Running "make depend"/"make" from the top level (not -C libraries)
# generates it first, same as a normal build; --disable-slapd above already
# keeps the server itself out of the build.
# "make -C libraries" (the top-level "all-common" target) also builds
# diagnostic-only binaries we don't need -- "apitest" (libldap) and "dtest"
# (liblber) -- and both fail outright under this toolchain: they link
# liblber.a in twice (once via a relative path, once absolute -- both the
# same real file), and Emscripten's SIDE_MODULE linking wraps every static
# archive in --whole-archive, so the same archive linked in twice means
# duplicate-symbol errors (wasm-ld: error: duplicate symbol: ...).
# Targeting each subdirectory's own library file by name (liblutil.a,
# liblber.la, libldap.la -- confirmed from the real Makefiles) instead of
# their bare default "all" target builds only the real libraries, never
# reaching those broken test-binary links. liblutil first: liblber's own
# "dtest" prerequisite check runs even when dtest itself isn't the
# requested target, so liblutil.a must already exist.
RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make depend && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C libraries/liblutil liblutil.a && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C libraries/liblber liblber.la && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C libraries/libldap libldap.la

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C libraries install && \
    mkdir -p /root/install/include && \
    cp include/ldap.h include/lber.h include/ldap_features.h include/lber_types.h include/ldap_cdefs.h /root/install/include/
