FROM kirigami-php-wasm:base

ARG JSPI
ARG LIBTIDY_VERSION

RUN wget https://github.com/htacg/tidy-html5/archive/refs/tags/$LIBTIDY_VERSION.tar.gz -O tidy-html5.tar.gz
RUN mkdir -p /root/tidy-html5 && tar -xzf tidy-html5.tar.gz -C /root/tidy-html5 --strip-components=1
WORKDIR /root/tidy-html5

# BUILD_SHARED_LIB/SUPPORT_CONSOLE_APP/BUILD_SAMPLE_CODE/BUILD_TAB2SPACE off:
# only the static library (which "always builds", see CMakeLists.txt) is
# needed -- the console `tidy` binary and sample code don't cross-compile
# cleanly and aren't consumed by ext/tidy anyway. SUPPORT_LOCALIZATIONS off:
# skips vendoring the localize/ message catalogs, ext/tidy only surfaces
# English diagnostic strings regardless.
#
# -DHAS_FUTIME=0: tidyplatform.h picks between <sys/utime.h> (Windows-style
# _futime) and <utime.h> (POSIX utime(), what tidylib.c actually calls) by
# checking for known-OS macros (__GLIBC__, LINUX_OS, BSD_BASED_OS, ...) --
# none of which Emscripten's musl libc defines, so it silently falls through
# to the Windows branch and fails with "'sys/utime.h' file not found"
# (confirmed as a real build error, not a guess). musl does provide a
# POSIX-compatible <utime.h>/utime() (confirmed with a standalone test
# compile), so forcing HAS_FUTIME=0 picks the right header for this target.
RUN mkdir -p build && cd build && \
	source /root/emsdk/emsdk_env.sh && \
	export JSPI_FLAGS=$(if [ "$JSPI" = "1" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
	emcmake cmake .. \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SHARED_LIB=OFF \
		-DSUPPORT_CONSOLE_APP=OFF \
		-DBUILD_SAMPLE_CODE=OFF \
		-DBUILD_TAB2SPACE=OFF \
		-DSUPPORT_LOCALIZATIONS=OFF \
		-DCMAKE_C_FLAGS="-sSIDE_MODULE -DHAS_FUTIME=0 $JSPI_FLAGS" \
		-DCMAKE_INSTALL_PREFIX=/root/install

RUN cd build && \
	source /root/emsdk/emsdk_env.sh && \
	emmake make tidy-static

RUN cd build && \
	source /root/emsdk/emsdk_env.sh && \
	emmake make install
