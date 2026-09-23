# anydoc (hosmelq/ext-anydoc): a Rust PHP extension (ext-php-rs), built as
# a wasm32-unknown-emscripten *staticlib* and handed to compile-extension as
# a vendorLib archive, the recipe @php-wasm/compile-extension's README gives
# for Rust extensions. The final phpize build (compile/extensions/anydoc)
# only wraps it: get_module comes from the Rust archive.
#
# Built on top of the compile-extension image itself (prepared by any
# `node compile/cli.mjs compile-extension` run): it has the PHP 8.5 headers
# and php-config ext-php-rs's bindgen reads, and the same emsdk.
FROM kirigami-compile-extension:compile-extension-php8-5-jspi

ARG ANYDOC_VERSION
# A pinned nightly: -Zbuild-std needs one, and pinning keeps rebuilds
# reproducible. Bump deliberately.
ARG RUST_NIGHTLY=nightly-2026-09-20

SHELL ["/bin/bash", "-c"]

# bindgen (through ext-php-rs) needs libclang, which emsdk doesn't ship.
# Ubuntu's is only used to parse PHP's headers for the wasm32 target, so
# it doesn't have to match emcc's clang version.
RUN apt-get update && \
    apt-get install -y --no-install-recommends libclang-dev && \
    rm -rf /var/lib/apt/lists/*
ENV LIBCLANG_PATH=/usr/lib/llvm-18/lib

RUN curl -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal \
        --default-toolchain $RUST_NIGHTLY --component rust-src && \
    source /root/.cargo/env && \
    rustup target add wasm32-unknown-emscripten

RUN set -euxo pipefail && \
    wget -O ext-anydoc.tar.gz https://github.com/hosmelq/ext-anydoc/archive/refs/tags/$ANYDOC_VERSION.tar.gz && \
    mkdir /root/ext-anydoc && \
    tar xf ext-anydoc.tar.gz -C /root/ext-anydoc --strip-components=1

WORKDIR /root/ext-anydoc

# A static archive instead of the cdylib upstream builds for native PHP.
RUN sed -i 's/^crate-type = \["cdylib"\]$/crate-type = ["staticlib"]/' Cargo.toml && \
    grep -q '^crate-type = \["staticlib"\]$' Cargo.toml

# ext-php-rs's build script runs `php -i` for the Zend API version, thread
# safety and debug flags. The image's only host PHP CLI is Ubuntu's (8.3),
# so this answers for the PHP 8.5 build being targeted instead (its API
# number matches the core's extension directory, 20250925). The headers
# themselves come from php-config (PHP_CONFIG), which is PHP 8.5's.
RUN printf '%s\n' '#!/bin/sh' \
        'echo "PHP Version => 8.5.11"' \
        'echo "PHP API => 20250925"' \
        'echo "Thread Safety => disabled"' \
        'echo "Debug Build => no"' > /usr/local/bin/php-wasm-info && \
    chmod +x /usr/local/bin/php-wasm-info

# ext-php-rs 0.15 has a compile-time size guard, `size_of::<Property
# Descriptor<()>>() <= 12 * size_of::<usize>()` (src/internal/property.rs),
# sized with 64-bit targets in mind: on wasm32 usize is 4 bytes but the
# struct keeps its 64-bit fields, so it fails ("PropertyDescriptor grew
# beyond expected size"). It's a size budget, not a layout or ABI check, so
# the fetched crate's bound is doubled for this build.
RUN source /root/.cargo/env && \
    cargo fetch && \
    PROPERTY_RS=$(ls /root/.cargo/registry/src/*/ext-php-rs-0.15.15/src/internal/property.rs) && \
    sed -i 's/<= 12 \* std::mem::size_of::<usize>()/<= 24 * std::mem::size_of::<usize>()/' $PROPERTY_RS && \
    grep -q '<= 24 \* std::mem::size_of::<usize>()' $PROPERTY_RS

# -Zbuild-std rebuilds std/core with these RUSTFLAGS: relocatable code for
# a side module, and panic=abort (no unwinding tables to reconcile with the
# core's Wasm EH). The C that cc-rs builds, ext-php-rs's own wrapper.c
# included (zend_try/zend_catch, so setjmp), needs -sSUPPORT_LONGJMP=wasm:
# emcc otherwise lowers setjmp/longjmp the JavaScript way (invoke_*
# wrappers, emscripten_longjmp, __THREW__, getTempRet0), which the core
# doesn't provide (it uses Wasm SjLj, __c_longjmp). Current nightlies
# already build the emscripten target itself with Wasm EH; the old
# -Zemscripten-wasm-eh switch is gone. bindgen parses PHP's headers for the wasm32 target, the
# same flags compile-extension's own build script exports; cc-rs build
# scripts (zstd-sys) compile C with emcc.
RUN source /root/.cargo/env && \
    source /root/emsdk/emsdk_env.sh && \
    export PHP=/usr/local/bin/php-wasm-info && \
    export PHP_CONFIG=/usr/local/bin/php-config && \
    export BINDGEN_EXTRA_CLANG_ARGS="--target=wasm32-unknown-emscripten --sysroot=/root/emsdk/upstream/emscripten/cache/sysroot -DZEND_ENABLE_ZVAL_LONG64 -D__x86_64__" && \
    export CC_wasm32_unknown_emscripten=emcc && \
    export AR_wasm32_unknown_emscripten=emar && \
    export CFLAGS_wasm32_unknown_emscripten="-fPIC -fwasm-exceptions -sSUPPORT_LONGJMP=wasm" && \
    export RUSTFLAGS="-C panic=abort -C relocation-model=pic" && \
    cargo build --release --target wasm32-unknown-emscripten \
        -Zbuild-std=std,panic_abort && \
    mkdir -p /root/install/lib /root/install/include && \
    cp target/wasm32-unknown-emscripten/release/libanydoc.a /root/install/lib/
