FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBDLIB_VERSION

# davisking/dlib, the C++ machine-learning toolkit behind php_dlib (face
# detection, landmarks, recognition, clustering). Tag archives unpack to
# dlib-<version without the leading v>, so strip the top-level directory
# instead of guessing its name.
RUN mkdir -p /root/dlib && \
    wget https://github.com/davisking/dlib/archive/refs/tags/$LIBDLIB_VERSION.tar.gz -O dlib.tar.gz && \
    tar -xzf dlib.tar.gz -C /root/dlib --strip-components=1
WORKDIR /root/dlib

# Static archive only, with what a single-threaded wasm module can use:
#  - no GUI (the X11 code can't exist here). DLIB_ISO_CPP_ONLY would also
#    drop the sockets/threads code, but it switches off all of the image
#    loaders too (the JPEG/PNG sources sit in its else branch), and
#    php_dlib needs them; dlib's thread pools just can't start threads in
#    this VM;
#  - no BLAS/LAPACK/CUDA (plain C++ matrix code, slower but portable);
#  - JPEG and PNG support on, from dlib's own bundled libjpeg/libpng/zlib
#    (external/), so no other vendorLib is needed: php_dlib loads images
#    with load_image(), which dispatches on the file extension.
# The C++ flags need the same exception ABI as the core's (-fwasm-exceptions
# under JSPI, CLAUDE.md decision 6). CMAKE_INSTALL_LIBDIR=lib pins the
# archive to install/lib. "install" also writes the generated dlib/config.h
# (carrying the DLIB_*_SUPPORT defines) and dlib-1.pc.
RUN mkdir -p build && cd build && \
    source /root/emsdk/emsdk_env.sh && \
    export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
    emcmake cmake -G"Unix Makefiles" ../dlib \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/root/install \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DBUILD_SHARED_LIBS=OFF \
        -DDLIB_NO_GUI_SUPPORT=ON \
        -DDLIB_USE_BLAS=OFF \
        -DDLIB_USE_LAPACK=OFF \
        -DDLIB_USE_CUDA=OFF \
        -DDLIB_USE_MKL_FFT=OFF \
        -DDLIB_JPEG_SUPPORT=ON \
        -DDLIB_PNG_SUPPORT=ON \
        -DDLIB_GIF_SUPPORT=OFF \
        -DDLIB_LINK_WITH_SQLITE3=OFF \
        -DCMAKE_C_FLAGS="-sSIDE_MODULE $JSPI_FLAGS" \
        -DCMAKE_CXX_FLAGS="-sSIDE_MODULE $JSPI_FLAGS"

RUN source /root/emsdk/emsdk_env.sh && \
    emmake make -C build -j2 && \
    emmake make -C build install && \
    test -f /root/install/lib/libdlib.a && test -f /root/install/include/dlib/config.h
