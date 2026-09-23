FROM kirigami-php-wasm:base

ARG JSPI
# Full version including Firebird's build number (e.g. 5.0.4.1812): the
# source tarball is named after it, the GitHub release tag (v5.0.4) after
# the first three components only.
ARG FIREBIRD_VERSION
ARG ICU_VERSION

# Firebird's own configure hard-requires ICU's headers (unicode/ucnv.h),
# but the client library never links ICU: src/common/unicode_util.cpp
# loads it at runtime with dlopen(), only when a charset/collation
# operation actually needs it. So only the headers are staged here, from
# the ICU source release (nothing is built) -- same "no ICU" stance as the
# rest of this repo (config.yaml's intl note).
RUN set -euxo pipefail && \
    wget https://github.com/unicode-org/icu/releases/download/release-$ICU_VERSION/icu4c-$ICU_VERSION-sources.tgz && \
    tar xf icu4c-$ICU_VERSION-sources.tgz && \
    mkdir -p /root/icu-include/unicode && \
    cp icu/source/common/unicode/*.h icu/source/i18n/unicode/*.h /root/icu-include/unicode/ && \
    rm -rf icu icu4c-$ICU_VERSION-sources.tgz

RUN set -euxo pipefail && \
    wget https://github.com/FirebirdSQL/firebird/releases/download/v${FIREBIRD_VERSION%.*}/Firebird-$FIREBIRD_VERSION-0-source.tar.xz && \
    tar xf Firebird-$FIREBIRD_VERSION-0-source.tar.xz

WORKDIR /root/Firebird-$FIREBIRD_VERSION-0-source

# --enable-client-only: libfbclient (yvalve + remote client + common) and
# its headers, no engine/server/tools. --with-builtin-tommath/tomcrypt:
# build Firebird's bundled copies (extern/), no system package exists in
# the build container. --with-wire-compress=no: no zlib for now.
# --without-editline: only isql uses it (and it would need termcap).
# ac_cv_file__proc_self_maps/exe=no: binreloc's and the LINUX platform's
# AC_CHECK_FILE(S) can't run when cross-compiling ("cannot check for file
# existence").
# ac_cv_lib_icuuc_main=yes: skips configure's AC_CHECK_LIB(icuuc) link
# test (headers only, see above).
#
# configure.ac has five AC_RUN_IFELSE tests with no cross-compiling
# fallback, which hard-fail ("cannot run test program while cross
# compiling") and aren't cached, so they can't be answered with ac_cv_*
# alone. The sed below turns that error into a no-op, so each test's
# cross-compiling branch just does nothing, and the values are supplied
# instead:
# - alignment of long / double: ac_cv_c_alignment=8/ac_cv_c_double_align=8
#   (wasm32: long long and double are both 8-byte aligned), preset below
#   and left untouched by the now-empty cross branch.
# - working sem_init(): -DWORKING_SEM_INIT=1 (it works under Emscripten;
#   without it Firebird falls back to named sem_open() semaphores).
# - 64-bit compare: left at FB_INT64_COMPARE_FAILED=1, Firebird's own
#   default and the safe workaround path (src/common/utils_proto.h).
# - native large file support: nothing needed, off_t is 64-bit already.
RUN sed -i 's/^as_fn_error \$? "cannot run test program while cross compiling/: "cannot run test program while cross compiling/' configure

# src/common/ThreadStart.cpp: on LINUX, Thread::getCurrentThreadId() uses
# the kernel thread id, syscall(SYS_gettid) -- SYS_gettid doesn't exist
# under Emscripten ("use of undeclared identifier 'SYS_gettid'"). The WASM
# VM is single-threaded, so pthread_self() (never 0, which is INVALID_ID)
# is an equally stable per-thread id.
RUN sed -i 's/tid = syscall(SYS_gettid);/tid = (int) (intptr_t) pthread_self();/' src/common/ThreadStart.cpp && \
    grep -q 'tid = (int) (intptr_t) pthread_self();' src/common/ThreadStart.cpp

RUN source /root/emsdk/emsdk_env.sh && \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --prefix=/root/install \
        --enable-client-only \
        --with-builtin-tommath \
        --with-builtin-tomcrypt \
        --with-wire-compress=no \
        --without-editline \
        CPPFLAGS="-I/root/icu-include -DWORKING_SEM_INIT=1" \
        ac_cv_lib_icuuc_main=yes \
        ac_cv_file__proc_self_maps=no \
        ac_cv_file__proc_self_exe=no \
        ac_cv_c_alignment=8 \
        ac_cv_c_double_align=8

# Not Firebird's top-level make (master_process): in client-only mode it
# builds libtommath/libtomcrypt through their makefile.shared (needs a
# system libtool, and produces .so files), and its "rest" step runs
# native tools built with em++. Instead, follow Firebird's own
# cross-compiling sequence (cross2 in builds/posix/Makefile.in): static
# tommath/tomcrypt via Firebird's own targets, plus re2 (common.a's
# SimilarToRegex needs it), then yvalve for the client objects. The
# chacha wire-encryption plugin is skipped (a separate plugin loaded with
# dlopen); the built-in Arc4 plugin stays available.
#
# FB_MAKE_VARS overrides prefix.linux's flags (configure picks the i386
# "linux" prefix from --build): OPTIMIZE_FLAGS drops -march=i586/
# -mtune=i686 (clang: "unsupported option '-march=' for target
# 'wasm32-unknown-emscripten'"), COMMON_FLAGS drops -ggdb (debug info). As
# command-line variables they also cancel the file's own target-specific
# "COMMON_FLAGS += -msse4" for CRC32C.o, just as invalid here. Firebird's
# make ignores configure's CPPFLAGS, so the ICU include path and
# WORKING_SEM_INIT (see the configure step) go into COMMON_FLAGS too.
# -DFB_CPU=CpuIntel: src/common/common.h has no wasm32 entry ("Define
# FB_CPU for your platform"). It's informational only (the
# "Firebird/Linux/..." implementation string, and an ODS header field only
# the server writes); CpuIntel is the closest match (32-bit, little-endian).
ENV FB_MAKE_VARS="TARGET=Release OPTIMIZE_FLAGS=-O2 COMMON_FLAGS='-DFB_SEND_FLAGS=MSG_NOSIGNAL -DLINUX -MMD -fPIC -I/root/icu-include -DWORKING_SEM_INIT=1 -DFB_CPU=CpuIntel'"
RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    export EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" && \
    ln -sf autoconfig.auto src/include/gen/autoconfig.h && \
    cd gen && \
    eval emmake make $FB_MAKE_VARS updateBuildNum && \
    eval emmake make $FB_MAKE_VARS export_lists && \
    emmake make -C ../extern/decNumber CC=emcc AR=emar && \
    ln -sf ../../extern/decNumber/libdecFloat.a ../temp/Release/ && \
    eval emmake make $FB_MAKE_VARS tommath && \
    eval emmake make $FB_MAKE_VARS LTCSOURCE=-DLTC_SOURCE tomcrypt && \
    eval emmake make $FB_MAKE_VARS re2

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    export EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" && \
    cd gen && \
    eval emmake make $FB_MAKE_VARS yvalve

# yvalve links libfbclient.so -- itself a WASM side module here, but
# cli.mjs's vendorLib mechanism links static archives into the extension's
# own side module. So the same objects are archived statically too, by a
# rule appended to the generated Makefile (YValve_Objects/
# Remote_Client_Objects/STATICLIB_LINK are Firebird's own variables), and
# installed along with every static dependency they need: common.a (as
# libfbcommon.a), libtommath, libtomcrypt, libdecFloat, libre2. Headers
# come from Firebird's own include_generic target (ibase.h,
# firebird/Interface.h and the rest of the public API).
#
# cloop (extern/cloop, Firebird's interface generator) is rebuilt natively
# with g++ first: make regenerates IdlFbInterfaces.h/fb_c_api.h from
# FirebirdInterface.idl whenever it considers them stale (the tarball's
# identical timestamps are enough), and the copy "external" built with em++
# can't run ("cloop: No such file or directory").
#
# firebird/impl/iberror_c.h (#included by the public iberror.h, and so by
# ibase.h) is generated too, by Firebird's src/msgs/build_file.cpp (its
# "message_file" step, skipped above), also built natively. It includes
# firebird.h, whose gen/autoconfig.h was configured for wasm32
# (SIZEOF_LONG 4, which conflicts with native 64-bit g++: "conflicting
# declaration 'typedef int ISC_LONG'"), so it's compiled against a
# throwaway copy of src/ with the three size defines set for the host. The
# generated header is just the error-code table, independent of the
# target. (The firebird.msg message file it also writes, used for error
# message text at runtime, isn't installed yet: errors still carry their
# codes, just not the formatted text.)
RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    export EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" && \
    cd gen && \
    make -C ../extern/cloop CXX=g++ TARGET=release WITH_FPC=0 \
        BUILD_DIR=$PWD/../temp/Release/cloop OUT_DIR=$PWD/Release/cloop core && \
    printf '\nfbclient_static: $(YValve_Objects) $(Remote_Client_Objects)\n\t$(STATICLIB_LINK) $(TMP_ROOT)/libfbclient.a $^\n' >> Makefile && \
    eval emmake make $FB_MAKE_VARS fbclient_static && \
    eval emmake make $FB_MAKE_VARS include_generic && \
    cp -r ../src /tmp/native-src && \
    sed -i 's/#define SIZEOF_LONG 4/#define SIZEOF_LONG 8/; s/#define SIZEOF_VOID_P 4/#define SIZEOF_VOID_P 8/; s/#define SIZEOF_SIZE_T 4/#define SIZEOF_SIZE_T 8/' \
        /tmp/native-src/include/gen/autoconfig.h && \
    g++ -std=c++17 -I/tmp/native-src/include -I/tmp/native-src/include/gen -DLINUX -DAMD64 \
        /tmp/native-src/msgs/build_file.cpp -o /tmp/build_file && \
    mkdir -p Release/firebird/include/firebird/impl && \
    /tmp/build_file -f /tmp/firebird.msg -c Release/firebird/include/firebird/impl/iberror_c.h && \
    rm -rf /tmp/native-src && \
    mkdir -p /root/install/lib /root/install/include && \
    cp ../temp/Release/libfbclient.a ../temp/Release/libtommath.a ../temp/Release/libtomcrypt.a \
       ../temp/Release/libre2.a /root/install/lib/ && \
    cp ../temp/Release/common.a /root/install/lib/libfbcommon.a && \
    cp -L ../temp/Release/libdecFloat.a /root/install/lib/ && \
    cp -r Release/firebird/include/. /root/install/include/

# libfbclient is C++ (and so is pdo_firebird_utils.cpp), but the core
# php.wasm is a C-only MAIN_MODULE=2 link: it has no libc++/libc++abi/
# libunwind, and so no __cpp_exception Wasm EH tag either (defined by
# libc++abi's __cpp_exception.S). A side module importing that tag can't
# even be instantiated ("Import #N env __cpp_exception: tag import requires
# a WebAssembly.Tag" -- the dynamic linker only has a JS stub to offer). So
# the C++ runtime is linked statically into the extension's own side
# module instead, as three more vendorLib archives: relocatable (--pic)
# builds, in the same Wasm EH flavour the rest of this repo compiles with
# (-fwasm-exceptions with Emscripten's default WASM_LEGACY_EXCEPTIONS=1,
# i.e. the *-legacyexcept variants). The side module then defines
# __cpp_exception itself. Exporting the C++ runtime from the core instead
# would grow php.wasm for every user, and need a core rebuild for each new
# libc++ symbol a future C++ extension happens to use.
RUN source /root/emsdk/emsdk_env.sh && \
    embuilder --pic build libc++-legacyexcept libc++abi-legacyexcept libunwind-legacyexcept && \
    PIC_DIR=/root/emsdk/upstream/emscripten/cache/sysroot/lib/wasm32-emscripten/pic && \
    cp $PIC_DIR/libc++-legacyexcept.a /root/install/lib/libc++.a && \
    cp $PIC_DIR/libc++abi-legacyexcept.a /root/install/lib/libc++abi.a && \
    cp $PIC_DIR/libunwind-legacyexcept.a /root/install/lib/libunwind.a
