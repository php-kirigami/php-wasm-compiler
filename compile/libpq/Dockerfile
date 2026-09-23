FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBPQ_VERSION

# configure's top-level checks require bison even though we only build
# src/interfaces/libpq below (bison is only actually invoked by the backend
# parser, never reached) -- the base image ships flex/perl but not bison.
RUN apt install -y bison

RUN set -euxo pipefail && \
    source /root/emsdk/emsdk_env.sh && \
    wget https://ftp.postgresql.org/pub/source/v$LIBPQ_VERSION/postgresql-$LIBPQ_VERSION.tar.gz && \
    tar xf postgresql-$LIBPQ_VERSION.tar.gz

WORKDIR /root/postgresql-$LIBPQ_VERSION

# --with-template=linux: configure's own platform detection has no entry for
# $host_os=emscripten (a hard error, unlike GMP's graceful generic-C
# fallback for an unrecognized CPU) -- forcing the linux template is
# configure's own documented escape hatch for an unported platform.
# --without-{readline,icu,zlib,openssl}: keep this first pass dependency-free
# (readline/zlib/openssl are optional; icu defaults to "on" since PG16 and
# fails configure outright if not explicitly disabled here, matching this
# repo's own reason for not vendoring ICU at all -- see config.yaml's intl
# note).
RUN source /root/emsdk/emsdk_env.sh && \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --with-template=linux \
        --prefix=/root/install \
        --without-readline \
        --without-icu \
        --without-zlib \
        --without-openssl

# Target "all-static-lib" (not the default "all", which also builds
# libpq.so) -- the .so link step fails under wasm-ld ("unknown argument:
# --enable-new-dtags", from the rpath flags Makefile.shlib's shared-library
# recipe always adds on the "linux" template) and we don't want a .so here
# anyway (STRIP_SO removes any .so this pipeline's other libs produce, same
# "static only" convention as libgmp/libsodium's own --disable-shared).
RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C src/interfaces/libpq all-static-lib

# No top-level "make install" (it pulls in "all", i.e. the .so build again)
# -- install just the static archive via Makefile.shlib's own
# "install-lib-static" target, then the minimal public headers libpq-fe.h
# (ext/pgsql) and libpq/libpq-fs.h (both ext/pgsql and ext/pdo_pgsql, for
# the large-object O_* mode constants) actually need, by hand, same
# minimal-header approach as compile/extensions/*/PROVENANCE.md's "only
# what's needed to build" rule.
RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    mkdir -p /root/install/include/libpq && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C src/interfaces/libpq installdirs-lib install-lib-static && \
    cp src/interfaces/libpq/libpq-fe.h src/interfaces/libpq/libpq-events.h src/include/postgres_ext.h /root/install/include/ && \
    cp src/include/libpq/libpq-fs.h /root/install/include/libpq/

# libpq.a alone isn't self-contained: it's built from src/interfaces/libpq's
# own objects only, and itself depends on src/port's and src/common's static
# libs (confirmed straight from configure's own generated VAL_LIBS: "-lpgcommon
# -lpgport -lm" -- a consumer statically linking -lpq is documented to need
# those two as well). Real, concretely-observed consequence: without them
# vendored too, ext/pgsql's own smoke test (real fe-connect.c connection
# code, not just loading the module) failed at PHP runtime with a genuinely
# missing symbol, `pg_vsnprintf` (from src/port/snprintf.c, Postgres's own
# portable vsnprintf -- not a libc function the main php.wasm module could
# ever export). The "_shlib" (PIC) object variants match how our own
# libpq.a was built (-fPIC via the SIDE_MODULE EMCC_FLAGS above), same as
# what Makefile.shlib's own shared-library link line uses
# (-lpgcommon_shlib -lpgport_shlib). cli.mjs's vendorLibFlags() picks up
# every *.a file under a vendored lib's lib/ directory automatically, so no
# extra config.yaml wiring is needed -- just installing them here.
RUN cp src/port/libpgport_shlib.a src/common/libpgcommon_shlib.a /root/install/lib/
