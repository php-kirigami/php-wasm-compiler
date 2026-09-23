FROM kirigami-php-wasm:base

ARG JSPI
ARG NETSNMP_VERSION

# OpenSSL (the vendored libopenssl, the version the core links) for
# SNMPv3: SHA-1/SHA-2 authentication and AES/DES privacy.
RUN mkdir -p /root/lib
COPY ./libopenssl/ /root/libopenssl_vendor
RUN cp -r /root/libopenssl_vendor/jspi/dist/root/lib/* /root/lib

# The GitHub tag archive, not a release tarball: net-snmp's own repo
# commits the generated configure, and its GitHub releases have no assets.
RUN set -euxo pipefail && \
    source /root/emsdk/emsdk_env.sh && \
    wget -O net-snmp-$NETSNMP_VERSION.tar.gz https://github.com/net-snmp/net-snmp/archive/refs/tags/v$NETSNMP_VERSION.tar.gz && \
    tar xf net-snmp-$NETSNMP_VERSION.tar.gz

WORKDIR /root/net-snmp-$NETSNMP_VERSION

# libnetsnmp (snmplib) only, what ext/snmp links: no agent (snmpd, and so
# none of the agent-only options: kmem, rpm, pcre, libnl, systemd), no
# apps (snmpget & co), no manuals, scripts or MIB files, no Perl/Python.
# With no MIB files installed, only numeric OIDs resolve (MIB files can be
# shipped later as package extraFiles). Non-interactive configure: the
# defaults it would otherwise prompt for (default SNMP version, contact,
# location, log file, persistent directory) are all given here.
# --with-endianness is required when cross-compiling. --disable-ipv6: the
# VM's networking (a Node-side proxy) is IPv4 only. No zlib/bzip2 (only
# used by the agent's persistent storage).
# ac_cv_func_EVP_md5=yes: configure probes EVP_md5 with AC_CHECK_FUNCS
# before -lcrypto is on the link line, so it comes back "no" (every later
# EVP_* probe finds its function), and its "Asked to use OpenSSL but I
# couldn't find it" check only looks at that one result. OpenSSL 3.6 has
# EVP_md5.
RUN source /root/emsdk/emsdk_env.sh && \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --prefix=/root/install \
        --disable-shared \
        --enable-static \
        --disable-agent \
        --disable-applications \
        --disable-manuals \
        --disable-scripts \
        --disable-mibs \
        --disable-embedded-perl \
        --disable-perl-cc-checks \
        --without-perl-modules \
        --without-python-modules \
        --disable-ipv6 \
        --without-zlib \
        --without-bzip2 \
        --with-endianness=little \
        --with-default-snmp-version=3 \
        --with-sys-contact=unknown \
        --with-sys-location=unknown \
        --with-logfile=none \
        --with-persistent-directory=/tmp/net-snmp \
        --with-openssl=/root/lib \
        ac_cv_func_EVP_md5=yes

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C snmplib && \
    emmake make -C snmplib installlibs && \
    emmake make installheaders

# ext/snmp's config.m4 has a single PKG_CHECK_MODULES([SNMP], ...) probe,
# so this vendorLib has to be self-contained (same pattern as
# libenchant2): libcrypto.a is installed next to libnetsnmp.a, so the
# SNMP_LIBS override cli.mjs builds from this lib/ directory covers both.
# OpenSSL's headers too: net-snmp's public headers include them
# (net-snmp/library/scapi.h -> openssl/ossl_typ.h).
RUN cp /root/lib/lib/libcrypto.a /root/install/lib/ && \
    cp -r /root/lib/include/openssl /root/install/include/
