FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBICONV_VERSION

RUN mkdir -p /root/lib/include /root/lib/lib
COPY ./libz/ /root/libz
RUN if [ "$JSPI" = "1" ]; then \
        cp -r /root/libz/jspi/dist/root/lib/* /root/lib; \
    else \
        cp -r /root/libz/asyncify/dist/root/lib/* /root/lib; \
    fi

RUN set -euxo pipefail; \
	wget https://ftp.gnu.org/pub/gnu/libiconv/libiconv-$LIBICONV_VERSION.tar.gz; \
	tar -xvf libiconv-$LIBICONV_VERSION.tar.gz; \
	rm libiconv-$LIBICONV_VERSION.tar.gz;

WORKDIR /root/libiconv-$LIBICONV_VERSION

RUN set -euxo pipefail; \
	source /root/emsdk/emsdk_env.sh; \
	CPPFLAGS="-I/root/lib/include " \
	LDFLAGS="-L/root/lib/lib " \
	emconfigure ./configure \
		PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
		--build i386-pc-linux-gnu \
		--target wasm32-unknown-emscripten \
		--prefix=/root/install/ \
		--enable-shared=no \
		--enable-static=yes;

RUN set -euxo pipefail; \
	source /root/emsdk/emsdk_env.sh; \
	export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
	EMCC_FLAGS=" -D__x86_64__ -fPIC $JSPI_FLAGS" emmake make; \
	emmake make install;

RUN ls -R /root/install
