FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBAURA_VERSION

# libaura/aura is hosted on GitLab, whose tag archives live under
# /-/archive/<tag>/<name>-<tag>.tar.gz (the top-level directory inside is
# aura-<tag>).
RUN wget https://gitlab.com/libaura/aura/-/archive/$LIBAURA_VERSION/aura-$LIBAURA_VERSION.tar.gz -O aura.tar.gz
RUN tar -xzf aura.tar.gz
WORKDIR /root/aura-$LIBAURA_VERSION

# Static archive only (AURA_BUILD_STATIC is the default; tests, examples,
# the upstream PHP/WASM subprojects stay off). libaura is C++20, so unlike
# libcmark-gfm this needs the C++ flags too: -fwasm-exceptions/-sSUPPORT_LONGJMP
# under JSPI (CLAUDE.md decision 6) must match the core's, or the archive
# would link against a different exception ABI. CMAKE_INSTALL_LIBDIR=lib
# pins the archive to install/lib (GNUInstallDirs would otherwise pick a
# distro-specific multiarch directory). "install" also drops libaura.pc,
# which ext-aura's config.m4 reads to fill in PHP_AURA_VERSION.
RUN mkdir -p build && cd build && \
    source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    emcmake cmake -G"Unix Makefiles" .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/root/install \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DAURA_BUILD_STATIC=ON \
        -DAURA_BUILD_SHARED=OFF \
        -DAURA_BUILD_TESTS=OFF \
        -DAURA_BUILD_EXAMPLES=OFF \
        -DAURA_BUILD_PHP=OFF \
        -DAURA_BUILD_WASM=OFF \
        -DCMAKE_C_FLAGS="-sSIDE_MODULE $JSPI_FLAGS" \
        -DCMAKE_CXX_FLAGS="-sSIDE_MODULE $JSPI_FLAGS"

RUN source /root/emsdk/emsdk_env.sh && \
    emmake make -C build && \
    emmake make -C build install && \
    test -f /root/install/lib/libaura.a && test -f /root/install/include/aura/aura.h
