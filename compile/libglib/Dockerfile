FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBGLIB_VERSION

RUN mkdir -p /root/lib/include /root/lib/lib
COPY ./libz/ /root/libz
RUN cp -r /root/libz/jspi/dist/root/lib/* /root/lib
COPY ./libffi/ /root/libffi_vendor
RUN cp -r /root/libffi_vendor/jspi/dist/root/lib/* /root/lib

# This release tarball ships only configure.ac (no pre-generated
# ./configure, unlike most autotools release tarballs) -- autoreconf
# regenerates it. gettext/autopoint (for glib's own i18n infrastructure)
# isn't in the base image.
RUN apt install -y gettext

RUN set -euxo pipefail && \
    source /root/emsdk/emsdk_env.sh && \
    wget https://download.gnome.org/sources/glib/2.58/glib-$LIBGLIB_VERSION.tar.xz && \
    tar xf glib-$LIBGLIB_VERSION.tar.xz

WORKDIR /root/glib-$LIBGLIB_VERSION

# configure.ac's own res_query detection (feeding GIO's DNS resolver) is a
# hard AC_MSG_ERROR with no config flag to skip and no AC_CACHE_CHECK
# wrapper to pre-seed via an env var (unlike glib_cv_stack_grows above) --
# real failure: "checking for res_query... configure: error: not found",
# genuinely missing from Emscripten's libc (confirmed: a real conftest.c
# calling res_query() fails to link, "undefined symbol: res_query"). We
# never build gio/ at all (only glib/gmodule, for ext/enchant's own
# glib-2.0 + gmodule-2.0 requirement -- see compile/extensions/enchant/
# PROVENANCE.md), so this dead code path is patched to warn instead of
# erroring, leaving NETWORK_LIBS empty, rather than trying to fake a real
# resolver.
RUN sed -i 's/\[AC_MSG_ERROR(not found)\])\])/[AC_MSG_WARN([res_query not found -- gio is not built for this target, so this is fine])])])/' configure.ac

# Not raw "autoreconf -fi": automake fails outright ("cannot open <
# gtk-doc.make") without gtk-doc installed, since Makefile.am references it
# unconditionally. glib's own autogen.sh handles this (stubs out an empty
# gtk-doc.make when gtkdocize isn't found, then runs autoreconf) --
# NOCONFIGURE=1 stops it from also running ./configure itself, since
# emconfigure runs it separately below with the cross-compile flags.
RUN NOCONFIGURE=1 ./autogen.sh

# g_list_free_full()/g_slist_free_full()/g_queue_free_full() (and the
# deprecated g_completion_clear_items()) call their one-argument
# GDestroyNotify through a cast to the two-argument GFunc:
# `g_list_foreach (list, (GFunc) free_func, NULL)`. Harmless on native ABIs,
# but WASM's call_indirect checks the callee's exact signature, so it traps
# with "function signature mismatch". Found via a real failure (2026-09-22):
# enchant_broker_init() -> g_slist_free_full() crashed on the first call.
# Newer glib releases replaced these casts with plain loops, and the patch
# below does the same. The grep fails the build if any of them didn't apply
# (e.g. after a version bump changed the source).
RUN sed -i 's/^  g_list_foreach (list, (GFunc) free_func, NULL);$/  { GList *l_; for (l_ = list; l_; l_ = l_->next) free_func (l_->data); }/' glib/glist.c && \
    sed -i 's/^  g_slist_foreach (list, (GFunc) free_func, NULL);$/  { GSList *l_; for (l_ = list; l_; l_ = l_->next) free_func (l_->data); }/' glib/gslist.c && \
    sed -i 's/^  g_queue_foreach (queue, (GFunc) free_func, NULL);$/  { GList *l_; for (l_ = queue->head; l_; l_ = l_->next) free_func (l_->data); }/' glib/gqueue.c && \
    sed -i 's/^  g_list_foreach (cmp->items, (GFunc) g_free, NULL);$/  { GList *l_; for (l_ = cmp->items; l_; l_ = l_->next) g_free (l_->data); }/' glib/deprecated/gcompletion.c && \
    ! grep -n '(GFunc) *\(free_func\|g_free\)' glib/glist.c glib/gslist.c glib/gqueue.c glib/deprecated/gcompletion.c

# 2.58.x is the last autotools-buildable glib series (2.60+ is meson-only,
# which this pipeline has no cross-compilation support for yet -- every
# other vendored lib here is autotools/CMake/plain-Makefile). ext/enchant's
# own config.m4 (older enchant releases, see compile/extensions/enchant/
# PROVENANCE.md) only needs glib-2.0 + gmodule-2.0, well within 2.58's
# feature set.
# -Wno-int-conversion: gatomic.c's own GNUC-atomics fallback path (its
# non-macro g_atomic_pointer_add()/g_atomic_pointer_and()/etc definitions,
# which call themselves as if invoking the macro form) passes an integer
# (gssize/gsize) where the macro expects a pointer -- real, deliberate glib
# code from 2018, predating recent Clang's default of treating implicit
# int-to-pointer conversion as a hard error rather than a warning. The
# generated code is correct regardless (both are pointer-sized on wasm32);
# only the strict type check needs relaxing.
# glib_cv_g_atomic_lock_free=no: real failure surfaced only later, at the
# final extension .so link (compile-extension's own pipeline, not this
# Dockerfile) -- "Atomic operations require threads [--enable-threads]"
# from Binaryen's wasm-validator, since the side-module link never passes
# --shared-memory/-pthread (none of this pipeline's other vendored libs
# emit real hardware atomic instructions, so this never came up before).
# gatomic.c's own g_atomic_lock_free detection (AC_CACHE_CHECK,
# glib_cv_g_atomic_lock_free) decides at configure time whether to inline
# real compiler atomic intrinsics or fall back to its own portable
# mutex-emulated path (glibconfig.h's G_ATOMIC_LOCK_FREE #define) --
# forcing the emulated path avoids emitting WASM atomic instructions at
# all. Fine for our purposes: ext/enchant's own usage here is effectively
# single-threaded (no real pthreads wired into this side-module build
# either), so the mutex-based fallback's slower-but-correct semantics cost
# nothing in practice.
RUN source /root/emsdk/emsdk_env.sh && \
    CPPFLAGS="-I/root/lib/include " \
    LDFLAGS="-L/root/lib/lib " \
    LIBFFI_CFLAGS="-I/root/lib/include " \
    LIBFFI_LIBS="-L/root/lib/lib -lffi " \
    CFLAGS="-Wno-int-conversion" \
    glib_cv_stack_grows=no \
    glib_cv_uscore=no \
    glib_cv_g_atomic_lock_free=no \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --prefix=/root/install \
        --disable-shared \
        --enable-static \
        --disable-dtrace \
        --disable-systemtap \
        --disable-fam \
        --disable-selinux \
        --disable-libmount \
        --disable-man \
        --with-pcre=internal

# Bare "make"/"make install" (the top-level SUBDIRS recursion: ".
# m4macros glib gmodule gthread gobject gio po docs tests subprojects")
# also builds gobject/gio, which we don't need (ext/enchant only requires
# glib-2.0 + gmodule-2.0) and which fails outright anyway: gobject's own
# "gobject-query" diagnostic tool links libglib-2.0.a in twice (once via a
# relative path, once absolute -- both the same real file), and
# Emscripten's SIDE_MODULE linking wraps every static archive in
# --whole-archive, so the same archive linked in twice is a duplicate-
# symbol error -- same root cause and same fix shape as openldap's own
# apitest/dtest (CLAUDE.md decision 56). Building only glib/ and gmodule/
# directly sidesteps gobject/gio entirely.
RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C glib && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C gmodule

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C glib install && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make -C gmodule install
