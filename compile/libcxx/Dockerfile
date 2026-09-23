FROM kirigami-php-wasm:base

# The C++ runtime (libc++, libc++abi, libunwind) as a vendorLib of its own,
# for C++ mode:shared extensions that have no other vendored library to
# carry it (rar, whose unrar code is bundled in the extension source).
# The core php.wasm is a C-only MAIN_MODULE=2 link with no C++ runtime and
# no __cpp_exception Wasm EH tag, so each C++ side module links its own:
# see compile/firebird/Dockerfile for the full reasoning (firebird and icu
# stage the same three archives next to their own libraries).
#
# Relocatable (--pic) builds, in the Wasm EH flavour the rest of this repo
# compiles with (-fwasm-exceptions with Emscripten's default
# WASM_LEGACY_EXCEPTIONS=1, the *-legacyexcept variants). No version of its
# own: it's whatever the pinned emsdk ships.
RUN source /root/emsdk/emsdk_env.sh && \
    embuilder --pic build libc++-legacyexcept libc++abi-legacyexcept libunwind-legacyexcept && \
    PIC_DIR=/root/emsdk/upstream/emscripten/cache/sysroot/lib/wasm32-emscripten/pic && \
    mkdir -p /root/install/lib /root/install/include && \
    cp $PIC_DIR/libc++-legacyexcept.a /root/install/lib/libc++.a && \
    cp $PIC_DIR/libc++abi-legacyexcept.a /root/install/lib/libc++abi.a && \
    cp $PIC_DIR/libunwind-legacyexcept.a /root/install/lib/libunwind.a
