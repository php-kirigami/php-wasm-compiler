FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBFFI_VERSION

RUN set -euxo pipefail && \
    source /root/emsdk/emsdk_env.sh && \
    wget https://github.com/libffi/libffi/releases/download/v$LIBFFI_VERSION/libffi-$LIBFFI_VERSION.tar.gz && \
    tar xf libffi-$LIBFFI_VERSION.tar.gz

WORKDIR /root/libffi-$LIBFFI_VERSION

# src/wasm/ffi.c's EM_JS trampolines (ffi_call_js, the closure
# trampoline) call another EM_JS function, unbox_small_structs(), by its
# bare name. That works when libffi is linked into a main module (all its
# EM_JS functions end up in the same JS scope), but ext/ffi and enchant's
# GLib link it into side modules, where the dynamic linker (libdylink.js's
# addEmJs) evals each EM_JS function on its own and stores it in that
# module's `moduleExports`: the bare name is undefined ("unbox_small_structs
# is not defined", from ffi's smoke test). `moduleExports` is in scope of
# that eval and stays the same object, so the calls go through it. The
# definition itself ("unbox_small_structs, (") doesn't match the pattern.
RUN sed -i 's/\([^_A-Za-z0-9.]\)unbox_small_structs(/\1moduleExports.unbox_small_structs(/g' src/wasm/ffi.c && \
    test "$(grep -c 'moduleExports.unbox_small_structs(' src/wasm/ffi.c)" = 5

RUN source /root/emsdk/emsdk_env.sh && \
    emconfigure ./configure \
        --build i386-pc-linux-gnu \
        --host wasm32-unknown-emscripten \
        --prefix=/root/install \
        --disable-shared \
        --enable-static \
        --disable-docs

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make

RUN source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    EMCC_FLAGS=" -sSIDE_MODULE $JSPI_FLAGS" emmake make install
