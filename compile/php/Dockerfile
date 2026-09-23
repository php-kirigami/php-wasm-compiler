FROM kirigami-php-wasm:base

ARG WITH_JSPI="no"
ENV PKG_CONFIG_PATH=/root/lib/lib/pkgconfig
ENV PATH="/root/lib/bin:${PATH}"

# The PHP version to build.
# This controls version-specific build patches and output filenames.
ARG PHP_VERSION

# The git ref to clone from php-src. Release builds use php-$PHP_VERSION tags;
# next builds use php-src development refs.
ARG PHP_REF

# Whether to build the opcache extension. The .m4 file is patched before the main
# ./buildconf script is generated.
ARG WITH_OPCACHE

RUN PHP_REF="${PHP_REF:-php-$PHP_VERSION}" && \
	git clone https://github.com/php/php-src.git php-src \
		--branch $PHP_REF \
		--single-branch \
		--depth 1;

# Empty out the JIT IR sources incompatible with WASM
RUN find /root/php-src/ext/opcache/jit -name "*.c" \
        ! -path "*/dynasm/minilua.c" \
        ! -path "*/gen_ir_fold_hash.c" \
        ! -path "*/ir_strtab.c" \
        -exec sh -c 'echo "" > "$1"' _ {} \; && \
    find /root/php-src/ext/opcache/jit -name "*.dasc" \
        ! -name "ir_x86.dasc" \
        -exec sh -c 'echo "" > "$1"' _ {} \;

# Work around memory leak due to PHP using Emscripten's incomplete mmap/munmap support
COPY ./compile/php-wasm-memory-storage /root/php-src/ext/wasm_memory_storage

# Polyfill for dns functions
COPY ./compile/php-wasm-dns-polyfill/ /root/php-src/ext/dns_polyfill

# Polyfill for post_message_to_js functions
COPY ./compile/php-post-message-to-js/ /root/php-src/ext/post_message_to_js

# Copy the opcache_module.c and php_opcache.h files.
COPY ./compile/opcache/ /root/php-src/ext/opcache

# PECL extensions downloaded fresh from their own GitHub repos (CLAUDE.md
# decision 34) — sourced there rather than pecl.php.net's package archive,
# which lags behind. Versions come from matrix.json's "extensions" entries
# (decision 28's pattern), passed in as build-args by cli.mjs. Must land in
# ext/ before buildconf, same as the extensions above.
ARG YAML_EXT_VERSION
ARG MDHTML_EXT_VERSION
ARG JSONK_EXT_VERSION
ARG APCU_EXT_VERSION
ARG NAVICAT_EXT_VERSION
ARG IGBINARY_EXT_VERSION
ARG NORM_EXT_VERSION
ARG SIMDJSON_VERSION
ARG YYJSON_VERSION
ARG UTF8PROC_VERSION

RUN mkdir -p /root/php-src/ext/yaml && \
	wget -O /root/yaml-ext.tar.gz https://github.com/php/pecl-file_formats-yaml/archive/refs/tags/${YAML_EXT_VERSION}.tar.gz && \
	tar -xzf /root/yaml-ext.tar.gz -C /root/php-src/ext/yaml --strip-components=1 && \
	rm /root/yaml-ext.tar.gz

# php-kirigami/php-mdhtml (CLAUDE.md decision 37) — our own CommonMark+GFM
# extension, replacing ext/cmark as the core, static-mode markdown
# renderer. No patches: unlike ext/cmark, we own this source directly.
RUN mkdir -p /root/php-src/ext/mdhtml && \
	wget -O /root/mdhtml-ext.tar.gz https://github.com/php-kirigami/php-mdhtml/archive/refs/tags/${MDHTML_EXT_VERSION}.tar.gz && \
	tar -xzf /root/mdhtml-ext.tar.gz -C /root/php-src/ext/mdhtml --strip-components=1 && \
	rm /root/mdhtml-ext.tar.gz

# php-kirigami/php-jsonk (the user's own project) — fast JSON encode/decode +
# JSON Schema validation. Vendors simdjson (decode) + yyjson (encode/schema
# reads) as flat amalgamated files, same layout as its own
# vendor/build/stage.sh recipe — fetched directly here rather than via a
# separate compile/lib*/Dockerfile, since neither needs a real build step
# (single-header/source releases). "json" is a hard module dependency of
# jsonk (see its zend_module_dep) — it can't be disabled, and php-src's
# ext/json/config.m4 has no --disable-json flag at all in PHP 8.5 anyway.
# "jsonk replaces json" is instead done by flipping the upstream ini default
# for "jsonk.replace_json_functions" from "0" to "1" for this build.
# simdjson.cpp's own runtime CPU-feature detection is gated by a raw
# `#elif defined(__x86_64__)` (not overridable via -D, unlike every other
# SIMDJSON_* knob jsonk's own config.m4 sets) and emits real x86 cpuid/
# xgetbv inline asm with no wasm equivalent — patched to also require
# `!defined(__EMSCRIPTEN__)`, falling through to simdjson's own existing
# portable instruction_set::DEFAULT branch instead.
COPY ./patches/simdjson/*.patch /root/patches-simdjson/
RUN mkdir -p /root/php-src/ext/jsonk && \
	wget -O /root/jsonk-ext.tar.gz https://github.com/php-kirigami/php-jsonk/archive/refs/tags/${JSONK_EXT_VERSION}.tar.gz && \
	tar -xzf /root/jsonk-ext.tar.gz -C /root/php-src/ext/jsonk --strip-components=1 && \
	rm /root/jsonk-ext.tar.gz && \
	mkdir -p /root/php-src/ext/jsonk/vendor/simdjson /root/php-src/ext/jsonk/vendor/yyjson && \
	wget -O /root/php-src/ext/jsonk/vendor/simdjson/simdjson.h "https://github.com/simdjson/simdjson/releases/download/v${SIMDJSON_VERSION}/simdjson.h" && \
	wget -O /root/php-src/ext/jsonk/vendor/simdjson/simdjson.cpp "https://github.com/simdjson/simdjson/releases/download/v${SIMDJSON_VERSION}/simdjson.cpp" && \
	wget -O /root/php-src/ext/jsonk/vendor/yyjson/yyjson.h "https://raw.githubusercontent.com/ibireme/yyjson/${YYJSON_VERSION}/src/yyjson.h" && \
	wget -O /root/php-src/ext/jsonk/vendor/yyjson/yyjson.c "https://raw.githubusercontent.com/ibireme/yyjson/${YYJSON_VERSION}/src/yyjson.c" && \
	cd /root/php-src/ext/jsonk/vendor/simdjson && \
	git apply --no-index -v /root/patches-simdjson/*.patch && \
	cd /root && \
	# config.m4's own "vendor/simdjson/simdjson.h" existence check (and its
	# PHP_NEW_EXTENSION source list) is a plain relative path, correct for
	# jsonk's own documented standalone build (phpize from ext/jsonk itself,
	# so cwd == ext/jsonk when configure runs) but NOT for this static,
	# whole-php-src-tree build: the generated top-level ./configure script
	# runs config.m4's shell body with cwd == /root/php-src, so the bare
	# relative path resolves there, not under ext/jsonk/. Mirroring the
	# vendored files at the php-src root too (real build confirmed this is
	# needed: configure failed with "simdjson not found" otherwise) resolves
	# both that guard check and any other relative reference the same way.
	mkdir -p /root/php-src/vendor && \
	cp -r /root/php-src/ext/jsonk/vendor/simdjson /root/php-src/vendor/simdjson && \
	cp -r /root/php-src/ext/jsonk/vendor/yyjson /root/php-src/vendor/yyjson && \
	# Default "jsonk.replace_json_functions" to on for this build (upstream
	# default is off) so the native json_encode()/json_decode() are
	# jsonk-backed without any consumer-side ini configuration.
	/root/replace.sh 's/"jsonk\.replace_json_functions", "0",/"jsonk.replace_json_functions", "1",/g' /root/php-src/ext/jsonk/jsonk.c

# krakjoe/apcu — shared user/opcode cache. Pure php-src-bundled-style
# sources like exif/mbstring/sockets above: no external lib, no vendoring.
# Added as an open experiment (not yet build-tested): does its cache
# actually persist across separate PHP.run() calls within the same
# php-wasm runtime instance? See matrix.json's own "apcu" note.
RUN mkdir -p /root/php-src/ext/apcu && \
	wget -O /root/apcu-ext.tar.gz https://github.com/krakjoe/apcu/archive/refs/tags/${APCU_EXT_VERSION}.tar.gz && \
	tar -xzf /root/apcu-ext.tar.gz -C /root/php-src/ext/apcu --strip-components=1 && \
	rm /root/apcu-ext.tar.gz

# igbinary/igbinary -- faster binary serializer, no external lib. Fetched
# after apcu (above) since its own config.m4 auto-detects apcu by checking
# for ext/apcu/apc_serializer.h at configure time (real path, not a guess --
# verified against igbinary's own config.m4) and, if found, registers
# itself as an APCu serializer at its own MINIT. The apc.serializer default
# is only flipped to "igbinary" further below, guarded on $WITH_IGBINARY —
# see the "Add igbinary if needed" block — since defaulting to a serializer
# that was never compiled in would break apcu whenever igbinary is off.
RUN mkdir -p /root/php-src/ext/igbinary && \
	wget -O /root/igbinary-ext.tar.gz https://github.com/igbinary/igbinary/archive/refs/tags/${IGBINARY_EXT_VERSION}.tar.gz && \
	tar -xzf /root/igbinary-ext.tar.gz -C /root/php-src/ext/igbinary --strip-components=1 && \
	rm /root/igbinary-ext.tar.gz && \
	# igbinary's own php_igbinary.h #includes "ext/standard/php_smart_string.h",
	# a compat header real PHP 8.x no longer ships (superseded by
	# Zend/zend_smart_string.h) -- confirmed as a real, known gap, not a
	# guess: @php-wasm/compile-extension's own build environment
	# (compile-extension/docker/Dockerfile.ext) synthesizes this exact same
	# one-line shim for this exact same reason. Mirrored here rather than
	# patching igbinary's own source, since it's a pure compat forwarding
	# header, not a real behavior difference.
	mkdir -p /root/php-src/ext/standard && \
	echo '#include "Zend/zend_smart_string.h"' > /root/php-src/ext/standard/php_smart_string.h

# php-kirigami/php-navicat (the user's own project) -- native client for
# Navicat Premium's own HTTP-tunnel protocol (ntunnel_mysql.php). Reuses the
# libcurl already linked in via the WITH_CURL block below for the HTTP
# transport; no separate library to vendor here.
RUN mkdir -p /root/php-src/ext/navicat && \
	wget -O /root/navicat-ext.tar.gz https://github.com/php-kirigami/php-navicat/archive/refs/tags/${NAVICAT_EXT_VERSION}.tar.gz && \
	tar -xzf /root/navicat-ext.tar.gz -C /root/php-src/ext/navicat --strip-components=1 && \
	rm /root/navicat-ext.tar.gz

# php-kirigami/php-norm (the user's own project) -- native Normalizer class +
# normalizer_normalize()/normalizer_is_normalized() functions, wrapping
# vendored utf8proc as flat source files compiled directly into ext/norm's
# own object files (same shape as ext/jsonk's simdjson/yyjson above -- no
# separate compile/lib*/Dockerfile, no .a, no .emcc-php-wasm-sources entry).
# Only registers its class/functions if intl (or anything else) hasn't
# already provided them.
RUN mkdir -p /root/php-src/ext/norm && \
	wget -O /root/norm-ext.tar.gz https://github.com/php-kirigami/php-norm/archive/refs/tags/${NORM_EXT_VERSION}.tar.gz && \
	tar -xzf /root/norm-ext.tar.gz -C /root/php-src/ext/norm --strip-components=1 && \
	rm /root/norm-ext.tar.gz && \
	mkdir -p /root/php-src/ext/norm/vendor/utf8proc && \
	wget -O /root/php-src/ext/norm/vendor/utf8proc/utf8proc.h "https://raw.githubusercontent.com/JuliaStrings/utf8proc/v${UTF8PROC_VERSION}/utf8proc.h" && \
	wget -O /root/php-src/ext/norm/vendor/utf8proc/utf8proc.c "https://raw.githubusercontent.com/JuliaStrings/utf8proc/v${UTF8PROC_VERSION}/utf8proc.c" && \
	wget -O /root/php-src/ext/norm/vendor/utf8proc/utf8proc_data.c "https://raw.githubusercontent.com/JuliaStrings/utf8proc/v${UTF8PROC_VERSION}/utf8proc_data.c" && \
	# config.m4's own "vendor/utf8proc/utf8proc.h" existence check (and its
	# PHP_NEW_EXTENSION source list) is a plain relative path -- same
	# "correct for a standalone phpize build, not for this whole-php-src-tree
	# build" gap as ext/jsonk's own vendor/simdjson check above. Mirroring
	# the vendored files at the php-src root too resolves it the same way.
	mkdir -p /root/php-src/vendor && \
	cp -r /root/php-src/ext/norm/vendor/utf8proc /root/php-src/vendor/utf8proc

# Bring in the libraries
RUN mkdir -p /root/lib/include /root/lib/lib /libs
COPY ./compile/libiconv/ /root/builds/libiconv
COPY ./compile/libcurl/ /root/builds/libcurl
COPY ./compile/libssh2/ /root/builds/libssh2
COPY ./compile/nghttp2/ /root/builds/nghttp2
COPY ./compile/libopenssl/ /root/builds/libopenssl
COPY ./compile/libpng16/ /root/builds/libpng16
COPY ./compile/libjpeg/ /root/builds/libjpeg
COPY ./compile/libwebp/ /root/builds/libwebp
COPY ./compile/libaom/ /root/builds/libaom
COPY ./compile/libavif/ /root/builds/libavif
COPY ./compile/libsqlite3/ /root/builds/libsqlite3
COPY ./compile/libxml2/ /root/builds/libxml2
COPY ./compile/libz/ /root/builds/libz
COPY ./compile/oniguruma/ /root/builds/oniguruma
COPY ./compile/libImageMagick/ /root/builds/libImageMagick
COPY ./compile/libfreetype/ /root/builds/libfreetype
COPY ./compile/libgd/ /root/builds/libgd
COPY ./compile/libyaml/ /root/builds/libyaml
COPY ./compile/libcmark-gfm/ /root/builds/libcmark-gfm
COPY ./compile/libbz2/ /root/builds/libbz2

RUN if [ "$WITH_JSPI" = "yes" ]; then \
	find /root/builds -path '*/jspi/dist/root/lib' -type d -exec cp -r {}/. /root/lib \; ; \
else \
	find /root/builds -path '*/asyncify/dist/root/lib' -type d -exec cp -r {}/. /root/lib \; ; \
fi

RUN find /root/lib /root/builds /libs -name "*.la" -exec sed -i 's/\r//g' {} \;


COPY ./compile/libzip/ /root/builds-libzip/
RUN mkdir -p /libs/libzip
RUN if [ "$WITH_JSPI" = "yes" ]; then \
	cp -r /root/builds-libzip/jspi/dist/1.9.2/root/lib /libs/libzip/1.9.2; \
else \
	cp -r /root/builds-libzip/asyncify/dist/1.9.2/root/lib /libs/libzip/1.9.2; \
fi

COPY ./compile/libopenssl/ /root/builds-libopenssl/
RUN mkdir -p /libs/libopenssl
RUN if [ "$WITH_JSPI" = "yes" ]; then \
	cp -r /root/builds-libopenssl/jspi/dist/root/lib /libs/libopenssl; \
else \
	cp -r /root/builds-libopenssl/asyncify/dist/root/lib /libs/libopenssl; \
fi

# Patch OPcache config.m4 file if needed
RUN if [ "$WITH_OPCACHE" = "yes" ]; \
	then \
		# Ensure mmap() shared memory is recognized, even if not true.
		# This will #declare HAVE_SHM_MMAP_ANON 1 in php_config.h
		if [[ "${PHP_VERSION:0:1}" -eq "8" && "${PHP_VERSION:2:1}" -ge "4" ]]; then \
			# Name changed in 8.4. See https://github.com/php/php-src/commit/ca55603d8a27b4e1f9c12cf72acd0fd39a6ac059
			/root/replace.sh 's/php_cv_shm_mmap_anon=no/php_cv_shm_mmap_anon=yes/g' /root/php-src/ext/opcache/config.m4; \
		else \
			/root/replace.sh 's/have_shm_mmap_anon=no/have_shm_mmap_anon=yes/g' /root/php-src/ext/opcache/config.m4; \
			# PHP bug: PHP_NEW_EXTENSION do not read $ext_shared from the config.m4 file.
			# https://github.com/php/php-src/blob/f35a22adba8a7ddf709c6cc98ecb57ba4dd8fe8c/ext/opcache/config.m4#L313
			/root/replace.sh 's/shared,,/no,,/g' /root/php-src/ext/opcache/config.m4; \
		fi; \
		# Force OPcache to be built as a static extension. OPcache is always built as a shared extension.
		/root/replace.sh 's/ext_shared=yes/ext_shared=no/g' /root/php-src/ext/opcache/config.m4; \
		# Add the opcache_module.c to the list of files to build for PHP < 8.5.0.
		# PHP 8.5 made OPcache a non-optional part of PHP https://wiki.php.net/rfc/make_opcache_required
		if [[ "${PHP_VERSION:0:1}" -lt "8" || ("${PHP_VERSION:0:1}" -eq "8" && "${PHP_VERSION:2:1}" -le "4") ]]; then \
			/root/replace.sh 's/shared_alloc_mmap.c/shared_alloc_mmap.c opcache_module.c/g' /root/php-src/ext/opcache/config.m4; \
		fi; \
		# Enable OPcache.
		echo -n ' --enable-opcache --disable-opcache-jit --disable-huge-code-pages ' >> /root/.php-configure-flags; \
	else \
		# Be sure to disable OPcache if not needed. --disable-opcache-jit is
		# also required here, not just in the "yes" branch above: PHP 8.5
		# made OPcache "non-optional" (https://wiki.php.net/rfc/make_opcache_required)
		# -- opcache/config.m4's own PHP_ARG_ENABLE([opcache-jit]) block runs
		# unconditionally, independent of --enable/--disable-opcache, and
		# defaults PHP_OPCACHE_JIT to "yes". Its $host_cpu-based arch check
		# (real x86_64 vs. supported) incorrectly concludes "yes, x86_64" for
		# this build, because the main PHP ./configure invocation is never
		# given an explicit --host wasm32-unknown-emscripten (unlike every
		# third-party lib Dockerfile), so autoconf's config.guess reports the
		# real Docker host's actual x86_64 CPU. That flips on HAVE_JIT and
		# real ext/opcache/jit/*.c sources, whose zend_jit.h then requires
		# the *compiler* to also define __x86_64__ (only true via this
		# project's own EMCC_FLAGS injection at `make` time, not during
		# `./configure`) -- found as a real "error validating input"/
		# "JIT not supported on this platform" build failure. Passing
		# --disable-opcache-jit skips that whole detection path outright,
		# matching what the "yes" branch already does.
		echo -n ' --disable-opcache --disable-opcache-jit --disable-huge-code-pages ' >> /root/.php-configure-flags; \
	fi

RUN cd php-src && ./buildconf --force

# Build PHP

# The PHP extensions to build:
ARG WITH_CURL
ARG WITH_FILEINFO
ARG WITH_LIBXML
ARG WITH_SOAP
ARG WITH_LIBZIP
ARG WITH_EXIF
ARG WITH_GD
ARG WITH_MBSTRING
ARG WITH_MBREGEX
ARG WITH_SOCKETS
ARG WITH_CLI_SAPI
ARG WITH_SQLITE
ARG WITH_MYSQL
ARG WITH_OPENSSL
ARG WITH_ICONV
ARG WITH_SOURCEMAPS
ARG OUTPUT_DIR_ON_HOST
ARG WITH_DEBUG
ARG DEBUG_DWARF_COMPILATION_DIR
ARG WITH_WS_NETWORKING_PROXY
ARG WITH_IMAGICK
ARG WITH_YAML
ARG WITH_MDHTML
ARG WITH_JSONK
ARG WITH_APCU
ARG WITH_NAVICAT
ARG WITH_IGBINARY
ARG WITH_NORM
ARG WITH_BZ2

# The platform to build for: web or node
ARG EMSCRIPTEN_ENVIRONMENT=web

RUN touch /root/.configure-flags && \
	touch /root/.emcc-php-wasm-flags && \
	touch /root/.emcc-php-wasm-sources && \
	touch /root/.JS_ABI_EXPORTS && \
	touch /root/.WASM_ABI_EXPORTS

RUN apt install -y bison;

# Add libzip and zlib files if needed
RUN if [ "$WITH_LIBZIP" = "yes" ]; then \
		cp -r /libs/libzip/1.9.2/* /root/lib; \
		# https://www.php.net/manual/en/zlib.installation.php
		echo -n ' --with-zlib --with-zlib-dir=/root/lib --with-zip ' >> /root/.php-configure-flags; \
		echo -n ' /root/lib/lib/libzip.a /root/lib/lib/libz.a ' >> /root/.emcc-php-wasm-sources; \
	fi;

# Add ncurses if needed and libedit if needed
RUN if [ "$WITH_CLI_SAPI" = "yes" ]; \
	then \
		# Configure build flags
		echo -n ' --enable-phar --enable-cli=static ' >> /root/.php-configure-flags && \
		echo -n ' /root/php-src/sapi/cli/php_cli_process_title.c /root/php-src/sapi/cli/ps_title.c ' \
			# Use absolute paths to help path mapping for DWARF debug info
			'/root/php-src/sapi/cli/php_http_parser.c /root/php-src/sapi/cli/php_cli_server.c ' \
			'/root/php-src/sapi/cli/php_cli.c ' \
			>> /root/.emcc-php-wasm-sources && \
		echo "_run_cli" >> /root/.JS_ABI_EXPORTS && echo "_wasm_add_cli_arg" >> /root/.JS_ABI_EXPORTS && \
		echo -n ' -DWITH_CLI_SAPI=1 ' >> /root/.emcc-php-wasm-flags && \
		/root/replace.sh 's/GET_SHELL_CB\(cb\);/(cb) = php_cli_get_shell_callbacks();/g' /root/php-src/ext/readline/readline_cli.c; \
		# Avoid a symbol conflict between php_cli.c and php_embed.c
		/root/replace.sh 's/\bHARDCODED_INI\b/HARDCODED_EMBED_INI/g' /root/php-src/sapi/embed/php_embed.c \
	else \
		echo -n ' --disable-cli ' >> /root/.php-configure-flags; \
	fi;

# Add Libxml2 if needed
RUN if [ "$WITH_LIBXML" = "yes" ]; \
	then \
		set -euxo pipefail;\
		echo -n ' --enable-libxml --with-libxml --with-libxml-dir=/root/lib --enable-dom --enable-xml --enable-simplexml --enable-xmlreader --enable-xmlwriter ' >> /root/.php-configure-flags && \
		echo -n ' /root/lib/lib/libxml2.a ' >> /root/.emcc-php-wasm-sources && \
		# Libxml check is wrong in PHP < 7.4.0.
		# In the regular cc it's just a warning, but in the emscripten's emcc that's an error:
		perl -pi.bak -e 's/char xmlInitParser/void xmlInitParser/g' /root/php-src/configure; \
		# On PHP < 7.1.0, the dom_iterators.c file implicitly converts *char to const *char causing emcc error
		if [[ "${PHP_VERSION:0:1}" -le "7" && "${PHP_VERSION:2:1}" -le "0" ]]; then \
			/root/replace.sh 's/xmlHashScan\(ht, itemHashScanner, iter\);/xmlHashScan(ht, (xmlHashScanner)itemHashScanner, iter);/g' /root/php-src/ext/dom/dom_iterators.c; \
		fi; \
	else \
		echo -n ' --disable-libxml --without-libxml --disable-dom --disable-xml --disable-simplexml --disable-xmlreader --disable-xmlwriter' >> /root/.php-configure-flags; \
	fi

# Add SOAP if needed (requires libxml)
RUN if [ "$WITH_SOAP" = "yes" ]; \
	then \
		echo -n ' --enable-soap' >> /root/.php-configure-flags; \
	else \
		echo -n ' --disable-soap' >> /root/.php-configure-flags; \
	fi

# Add sqlite3 if needed
RUN if [ "$WITH_SQLITE" = "yes" ]; \
	then \
		echo -n ' --with-sqlite3 --enable-pdo --with-pdo-sqlite=/root/lib ' >> /root/.php-configure-flags; \
		echo -n ' /root/lib/lib/libsqlite3.a ' >> /root/.emcc-php-wasm-sources; \
	else \
		echo -n ' --without-sqlite3 --disable-pdo ' >> /root/.php-configure-flags; \
	fi

# Add GD if needed
RUN if [ "$WITH_GD" = "yes" ]; \
	then \
		# Use external libgd for PHP 8.1+; fall back to bundled gd for <= 8.0 (and 7.x) to avoid missing symbols
		if [[ "${PHP_VERSION:0:1}" -gt "8" || ( "${PHP_VERSION:0:1}" -eq "8" && "${PHP_VERSION:2:1}" -ge "1" ) ]]; then \
			echo -n ' --with-external-gd=/root/lib --enable-gd  ' >> /root/.php-configure-flags; \
			echo -n ' /root/lib/lib/libgd.a /root/lib/lib/libjpeg.a /root/lib/lib/libpng16.a /root/lib/lib/libwebp.a /root/lib/lib/libsharpyuv.a /root/lib/lib/libavif.a /root/lib/lib/libfreetype.a ' >> /root/.emcc-php-wasm-sources; \
			if [[ "${PHP_VERSION:0:1}" -eq "8" && "${PHP_VERSION:2:1}" -ge "1" ]]; then \
				echo -n ' --with-avif ' >> /root/.php-configure-flags; \
			fi; \
		else \
			# Bundled gd for PHP 7.4 and 8.0
			echo -n ' --enable-gd ' >> /root/.php-configure-flags; \
			if [[ "${PHP_VERSION:0:1}" -le "7" && "${PHP_VERSION:2:1}" -le "4" ]]; then \
				/root/replace.sh 's/static long php_jpeg_emit_message/static void php_jpeg_emit_message/g' /root/php-src/ext/gd/libgd/gd_jpeg.c; \
				sed -i '84s/.*return 1;.*//' /root/php-src/ext/gd/libgd/gd_jpeg.c; \
				/root/replace.sh 's/cinfo.err->emit_message = \(void \(*\)\(j_common_ptr,int\)\) php_jpeg_emit_message;/cinfo.err->emit_message = php_jpeg_emit_message;/g' /root/php-src/ext/gd/libgd/gd_jpeg.c; \
			fi; \
			echo -n ' /root/lib/lib/libjpeg.a /root/lib/lib/libpng16.a /root/lib/lib/libwebp.a /root/lib/lib/libsharpyuv.a ' >> /root/.emcc-php-wasm-sources; \
		fi; \
		echo -n ' --with-png-dir=/root/lib --with-jpeg --with-webp ' >> /root/.php-configure-flags; \
	fi;

# Add openssl if needed
RUN if [ "$WITH_OPENSSL" = "yes" ]; \
	then \
		cp -r /libs/libopenssl/* /root/lib; \
		echo -n ' --with-openssl --with-openssl-dir=/root/lib ' >> /root/.php-configure-flags; \
		echo -n ' /root/lib/lib/libcrypto.a /root/lib/lib/libssl.a ' >> /root/.emcc-php-wasm-sources; \
	fi;

# Remove fileinfo if needed
RUN if [ "$WITH_FILEINFO" = "yes" ]; \
    then \
		echo -n ' --enable-fileinfo' >> /root/.php-configure-flags; \
	else \
        # light bundle should compile without fileinfo and libmagic
        echo -n ' --disable-fileinfo' >> /root/.php-configure-flags; \
    fi;

# Add iconv if needed
RUN if [ "$WITH_ICONV" = "yes" ]; \
	then \
		echo -n ' --with-iconv=/root/lib ' >> /root/.php-configure-flags; \
		echo -n ' /root/lib/lib/libiconv.a ' >> /root/.emcc-php-wasm-sources; \
		# PHP >= 8.0 posix is no longer supported.
		# PHP ≤= 7.4 posix is supported.
	else \
		echo -n ' --without-iconv ' >> /root/.php-configure-flags; \
	fi;

# Add curl if needed
RUN if [ "$WITH_CURL" = "yes" ]; \
	then \
		echo -n ' --with-curl=/root/lib ' >> /root/.php-configure-flags; \
		echo -n ' /root/lib/lib/libcurl.a /root/lib/lib/libssh2.a /root/lib/lib/libnghttp2.a ' >> /root/.emcc-php-wasm-sources; \
	fi;

# Add mbstring if needed
# mbstring is required for the exif extension
RUN if [ "$WITH_MBSTRING" = "yes" ] || [ "$WITH_EXIF" = "yes" ]; \
	then echo -n ' --enable-mbstring ' >> /root/.php-configure-flags; \
	else echo -n ' --disable-mbstring ' >> /root/.php-configure-flags; \
	fi;

# Add exif if needed
RUN if [ "$WITH_EXIF" = "yes" ]; \
	then \
		echo -n ' --enable-exif ' >> /root/.php-configure-flags; \
	else \
		echo -n ' --disable-exif ' >> /root/.php-configure-flags; \
	fi;

# Add mbregex if needed
RUN if [ "$WITH_MBREGEX" = "yes" ] && [ "${PHP_VERSION:0:3}" != "7.0" ]; \
	then \
		echo -n ' --enable-mbregex --with-onig=/root/lib ' >> /root/.php-configure-flags; \
		echo -n ' /root/lib/lib/libonig.a ' >> /root/.emcc-php-wasm-sources; \
	else \
		echo -n ' --disable-mbregex ' >> /root/.php-configure-flags; \
	fi;

# Add sockets if needed (no external lib — pure php-src bundled code, like
# exif/mbstring above; CLAUDE.md decision 41)
RUN if [ "$WITH_SOCKETS" = "yes" ]; \
	then \
		echo -n ' --enable-sockets ' >> /root/.php-configure-flags; \
	else \
		echo -n ' --disable-sockets ' >> /root/.php-configure-flags; \
	fi;

# Add yaml if needed (PECL, vendored from GitHub — CLAUDE.md decision 34)
RUN if [ "$WITH_YAML" = "yes" ]; \
	then \
		echo -n ' --with-yaml=/root/lib ' >> /root/.php-configure-flags; \
		echo -n ' /root/lib/lib/libyaml.a ' >> /root/.emcc-php-wasm-sources; \
	else \
		echo -n ' --without-yaml ' >> /root/.php-configure-flags; \
	fi;

# Add mdhtml if needed (php-kirigami/php-mdhtml, our own CommonMark+GFM
# extension built on cmark-gfm — CLAUDE.md decision 37, replaces cmark as
# the core static-mode markdown extension)
RUN if [ "$WITH_MDHTML" = "yes" ]; \
	then \
		echo -n ' --with-mdhtml=/root/lib ' >> /root/.php-configure-flags; \
		echo -n ' /root/lib/lib/libcmark-gfm.a /root/lib/lib/libcmark-gfm-extensions.a ' >> /root/.emcc-php-wasm-sources; \
	else \
		echo -n ' --without-mdhtml ' >> /root/.php-configure-flags; \
	fi;

# Add jsonk if needed (php-kirigami/php-jsonk — see the "Get and patch PHP"
# comment above for why ext/json itself can't be disabled). No external .a
# to append to .emcc-php-wasm-sources: simdjson/yyjson are vendored as
# sources compiled straight into ext/jsonk's own object files, not a
# separately-built static library.
RUN if [ "$WITH_JSONK" = "yes" ]; \
	then \
		echo -n ' --enable-jsonk ' >> /root/.php-configure-flags; \
	else \
		echo -n ' --disable-jsonk ' >> /root/.php-configure-flags; \
	fi;

# Add apcu if needed (krakjoe/apcu — experiment, see matrix.json's "apcu"
# note: does its shared cache persist across PHP.run() calls in the same
# php-wasm runtime instance?)
RUN if [ "$WITH_APCU" = "yes" ]; \
	then \
		echo -n ' --enable-apcu ' >> /root/.php-configure-flags; \
	else \
		echo -n ' --disable-apcu ' >> /root/.php-configure-flags; \
	fi;

# Add navicat if needed (php-kirigami/php-navicat). No separate
# .emcc-php-wasm-sources entry: it reuses libcurl.a, already appended there
# by the WITH_CURL block above.
RUN if [ "$WITH_NAVICAT" = "yes" ]; \
	then \
		echo -n ' --with-navicat=/root/lib ' >> /root/.php-configure-flags; \
	else \
		echo -n ' --without-navicat ' >> /root/.php-configure-flags; \
	fi;

# Add igbinary if needed (igbinary/igbinary). Only flips apcu's own
# apc.serializer default to "igbinary" when igbinary is actually being
# built -- defaulting to a serializer that was never compiled in would
# break apcu outright (see the "igbinary/igbinary" fetch comment above).
RUN if [ "$WITH_IGBINARY" = "yes" ]; \
	then \
		echo -n ' --enable-igbinary ' >> /root/.php-configure-flags; \
		/root/replace.sh 's/STD_PHP_INI_ENTRY\("apc\.serializer", "php",/STD_PHP_INI_ENTRY("apc.serializer", "igbinary",/g' /root/php-src/ext/apcu/php_apc.c; \
	else \
		echo -n ' --disable-igbinary ' >> /root/.php-configure-flags; \
	fi;

# Add norm if needed (php-kirigami/php-norm). No separate
# .emcc-php-wasm-sources entry: utf8proc.c is compiled straight into
# ext/norm's own object files, not a separately-built static library.
RUN if [ "$WITH_NORM" = "yes" ]; \
	then \
		echo -n ' --enable-norm ' >> /root/.php-configure-flags; \
	else \
		echo -n ' --disable-norm ' >> /root/.php-configure-flags; \
	fi;

# Add bz2 if needed (php-src's own ext/bz2 -- gives phar its optional
# .tar.bz2 archive support, gated by phar's own `#if HAVE_BZ2` checks, in
# addition to registering bz2's own bzcompress()/bzdecompress() functions).
# Same "--with-x=DIR + explicit .a on the link line" shape as yaml/mdhtml
# above -- ext/bz2's config.m4 is PHP_ARG_WITH (DIR-based bzlib.h lookup),
# not PHP_ARG_ENABLE.
RUN if [ "$WITH_BZ2" = "yes" ]; \
	then \
		echo -n ' --with-bz2=/root/lib ' >> /root/.php-configure-flags; \
		echo -n ' /root/lib/lib/libbz2.a ' >> /root/.emcc-php-wasm-sources; \
	else \
		echo -n ' --without-bz2 ' >> /root/.php-configure-flags; \
	fi;

# Get and patch PHP
COPY ./compile/php/php*.patch /root/
COPY ./compile/php/apply-mysqlnd-patch.sh /root/
# ext/sockets (CLAUDE.md decision 42): php-src assumes that if AF_PACKET /
# SO_ATTACH_REUSEPORT_CBPF are defined, the associated Linux kernel raw-
# socket/BPF-filter structs and macros (struct sockaddr_ll, SKF_AD_OFF,
# BPF_RET, ...) are too — true on real Linux, not true under Emscripten's
# partial POSIX emulation, which defines the constants but not the rest.
# Disables those two feature blocks entirely under __EMSCRIPTEN__ rather
# than chasing individual missing macros one at a time.
COPY ./patches/sockets/*.patch /root/patches-sockets/
# ext/apcu (experiment, see the "Add apcu if needed" block above): apc_shm.c
# unconditionally compiles real SysV shm syscalls (shmget/shmat/shmctl/
# shmdt) that Emscripten declares (musl-derived headers) but never
# implements — a real "undefined symbol" link failure at the final
# php.wasm link step, found on the first real build attempt. The default
# build (--enable-apcu-mmap, on by default) never actually calls this
# backend at runtime; the patch stubs it with a fatal error instead of
# leaving it silently reachable.
COPY ./patches/apcu/*.patch /root/patches-apcu/
RUN cd /root && \
	PHP_PATCH_VERSION="${PHP_VERSION:0:3}" && \
	if ! compgen -G "/root/php${PHP_PATCH_VERSION}*.patch" > /dev/null; then \
		PHP_PATCH_VERSION="8.5"; \
	fi && \
	git apply --no-index /root/php${PHP_PATCH_VERSION}*.patch -v && \
	chmod +x /root/apply-mysqlnd-patch.sh && \
	/root/apply-mysqlnd-patch.sh && \
	if [[ "$PHP_PATCH_VERSION" == '8.5' ]]; then \
		git apply --no-index /root/php-chunk-alloc-zend-assert-8.5.patch -v; \
	fi && \
	cd php-src && \
	git apply --no-index -v /root/patches-sockets/*.patch && \
	git apply --no-index -v /root/patches-apcu/*.patch && \
	cd /root && \
	touch php-src/patched

# Install Mysql support if needed
RUN if [ "$WITH_MYSQL" = "yes" ]; then \
		echo -n ' --enable-mysql --enable-pdo --with-mysql=mysqlnd --with-mysqli=mysqlnd --with-pdo-mysql=mysqlnd ' >> /root/.php-configure-flags; \
		echo -n ' -DMYSQLND_FORCE_TCP_LOCALHOST ' >> /root/.emcc-php-wasm-flags; \
	fi

# Download and prepare imagick extension if needed (PHP 7.4+)
# Use version 3.7.0 for PHP 7.x, a pinned real tag (IMAGICK_EXT_VERSION) for
# PHP 8.x -- was tracking the unpinned "master" branch directly, which is
# both non-reproducible (decision 11's same oniguruma gap) and how a real,
# user-noticed bug went unfixed: php_imagick.h ships
# `#define PHP_IMAGICK_VERSION "@PACKAGE_VERSION@"` literally (its own
# comment says this is deliberate -- meant to be substituted by PECL's own
# `pecl package` tool at release time, which this pipeline's plain git
# clone never runs), so phpinfo() displayed the raw placeholder string
# instead of a real version. Substituted below with the same pinned
# version actually being built, now that there is one.
ARG IMAGICK_EXT_VERSION
RUN if [ "$WITH_IMAGICK" = "yes" ]; then \
		if [ "${PHP_VERSION:0:1}" -eq "7" ]; then \
			IMAGICK_BRANCH="3.7.0"; \
		else \
			IMAGICK_BRANCH="$IMAGICK_EXT_VERSION"; \
		fi && \
		git clone https://github.com/Imagick/imagick.git /root/imagick \
			--branch "$IMAGICK_BRANCH" \
			--single-branch \
			--depth 1 && \
		# replace.sh shells out to `perl -pi.bak -e`, and Perl treats a bare
		# "@" inside an -e pattern string as array interpolation (silently
		# expanding to empty), not a literal character -- confirmed by a
		# real test: the unescaped version left "PACKAGE_VERSION3.8.1...
		# 3.8.1PACKAGE_VERSION..."-style garbage (each lone "@" replaced
		# independently, the word between them untouched). Escaping both
		# "@" as "\@" fixes it.
		/root/replace.sh "s/\@PACKAGE_VERSION\@/$IMAGICK_BRANCH/g" /root/imagick/php_imagick.h && \
		cp -r /root/imagick /root/php-src/ext/imagick && \
		cd /root/php-src && \
		if [ "${PHP_VERSION:0:1}" -eq "7" ]; then \
			cp configure configure.backup && \
			./buildconf --force && \
			(/root/replace.sh 's/char xmlInitParser/void xmlInitParser/g' configure || true) && \
			(/root/replace.sh 's/\$PKG_CONFIG --atleast-version 7.\d+.\d+ \$PKNAME/test "1" = "1"/g' configure || true) && \
			(/root/replace.sh 's/test "\$curl_version" -ge 7010005/test "1" = "1"/g' configure || true) && \
			perl -pi.bak -e 's/ret = read/ret = wasm_read/g' configure; \
		else \
			./buildconf --force; \
		fi && \
		echo -n ' --with-imagick=/root/lib --with-imagick-config=/root/lib/bin/wasm32-unknown-emscripten-MagickWand-config ' >> /root/.php-configure-flags && \
		echo -n ' /root/lib/lib/libMagickWand-7.Q16HDRI.a /root/lib/lib/libMagickCore-7.Q16HDRI.a ' >> /root/.emcc-php-wasm-sources; \
	fi

# Disable phar.php generation for all builds (WASM binaries can't execute during build)
# This must run after buildconf but before configure
RUN /root/replace.sh 's/pharcmd=pharcmd/pharcmd=/g' /root/php-src/configure && \
	/root/replace.sh 's/pharcmd_install=install-pharcmd/pharcmd_install=/g' /root/php-src/configure

# Build the patched PHP
WORKDIR /root/php-src
RUN source /root/emsdk/emsdk_env.sh && \
	CURL_CFLAGS="-I/root/lib/include" \
	WEBP_LIBS="-L/root/lib/lib -lwebp -lsharpyuv" \
	WEBP_CFLAGS="-I/root/lib/include" \
	CURL_LIBS="-I/root/lib/lib -L/root/lib/lib" \
	JPEG_CFLAGS="-I/root/lib/include" \
	JPEG_LIBS="-L/root/lib/lib -ljpeg" \
	PNG_CFLAGS="-I/root/lib/include" \
	PNG_LIBS="-L/root/lib/lib -lpng16 -lz" \
	AVIF_CFLAGS="-I/root/lib/include" \
	AVIF_LIBS="-L/root/lib/lib -lavif" \
	GDLIB_CFLAGS="-I/root/lib/include" \
	GDLIB_LIBS="-L/root/lib/lib -lgd -lpng16 -lz -ljpeg -lwebp -lsharpyuv -lavif" \
	PKG_CONFIG_PATH="/root/lib/lib/pkgconfig:${PKG_CONFIG_PATH}" \
		emconfigure ./configure \
		PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
	# Fibers are a PHP 8.1+ feature. They are compiled as
	# a custom assembly implementation by default. However,
	# that implementation does not work with emscripten.
	#
	# The line below disables it, forcing PHP to use the
	# C implementation instead.
	#
	# See https://github.com/WordPress/wordpress-playground/issues/92
	# for more context.
	--disable-fiber-asm \
	--disable-phar \
	# --enable-json for PHP < 8.0:
	--enable-json      \
	--enable-embed=static \
	--with-layout=GNU  \
	--disable-cgi      \
	--disable-opcache  \
	--disable-posix    \
	--enable-hash      \
	--enable-static    \
	--enable-shared    \
	--disable-session   \
	--enable-filter    \
	--enable-calendar  \
	--disable-rpath    \
	--disable-phpdbg   \
	--without-pear     \
	--with-valgrind=no \
	--without-pcre-jit \
	--enable-bcmath    \
	--enable-ctype     \
	--enable-tokenizer \
	--enable-wasm_memory_storage \
	--enable-dns_polyfill \
	--enable-post_message_to_js \
	$(cat /root/.php-configure-flags)

# Silence the errors "munmap() failed: [28] Invalid argument"
# @TODO: Identify the root cause behind these errors and fix them properly
RUN echo '#define ZEND_MM_ERROR 0' >> /root/php-src/main/php_config.h;

# Force GD JPEG/PNG/WebP/AVIF support for external GD builds (PHP 8.1+)
# Configure checks may fail during cross-compilation, so we explicitly enable them
# after verifying the external libgd was built with these features.
RUN if [ "$WITH_GD" = "yes" ]; then \
	if [[ "${PHP_VERSION:0:1}" -gt "8" || ( "${PHP_VERSION:0:1}" -eq "8" && "${PHP_VERSION:2:1}" -ge "1" ) ]]; then \
		echo '/* Force GD format support for external libgd */' >> /root/php-src/main/php_config.h; \
		echo '#ifndef HAVE_GD_JPG' >> /root/php-src/main/php_config.h; \
		echo '#define HAVE_GD_JPG 1' >> /root/php-src/main/php_config.h; \
		echo '#endif' >> /root/php-src/main/php_config.h; \
		echo '#ifndef HAVE_GD_PNG' >> /root/php-src/main/php_config.h; \
		echo '#define HAVE_GD_PNG 1' >> /root/php-src/main/php_config.h; \
		echo '#endif' >> /root/php-src/main/php_config.h; \
		echo '#ifndef HAVE_GD_WEBP' >> /root/php-src/main/php_config.h; \
		echo '#define HAVE_GD_WEBP 1' >> /root/php-src/main/php_config.h; \
		echo '#endif' >> /root/php-src/main/php_config.h; \
		echo '#ifndef HAVE_GD_AVIF' >> /root/php-src/main/php_config.h; \
		echo '#define HAVE_GD_AVIF 1' >> /root/php-src/main/php_config.h; \
		echo '#endif' >> /root/php-src/main/php_config.h; \
	fi; \
fi;

# 64 bit long support {{{
# PHP uses custom typedefs for 64 bit integers in many places, e.g.
# timelib_long is int64_t when __x86_64__ is defined and int32_t otherwise.
#
# We're defining __x86_64__ in all emcc calls in this project, which
# works well, but also has some side effects in the PHP codebase.
#
# Specifically, PHP has a lot of inlined asm for 64bit processors that is used
# when __x86_64__ is defined. Inline asm won't build to WebAssembly so we need to
# disable every instance of inlined asm.
RUN echo '#undef HAVE_ASM_GOTO' >> /root/php-src/main/php_config.h;
RUN echo '#define HAVE_ASM_GOTO 0' >> /root/php-src/main/php_config.h;
# Disable asm arithmetic.
RUN /root/replace.sh 's/ZEND_USE_ASM_ARITHMETIC 1/ZEND_USE_ASM_ARITHMETIC 0/g' /root/php-src/Zend/zend_operators.h;
RUN /root/replace.sh 's/defined\(__GNUC__\)/0/g' /root/php-src/Zend/zend_multiply.h
RUN /root/replace.sh 's/defined\(__GNUC__\)/0/g' /root/php-src/Zend/zend_cpuinfo.c
RUN /root/replace.sh 's/defined\(__clang__\)/0/g' /root/php-src/Zend/zend_cpuinfo.c
# }}}

# With HAVE_UNISTD_H=1 PHP complains about the missing getdtablesize() function
RUN /root/replace.sh 's/define php_sleep sleep/define php_sleep wasm_sleep/g' /root/php-src/main/php.h
RUN echo 'extern unsigned int wasm_sleep(unsigned int time);' >> /root/php-src/main/php.h;

RUN /root/replace.sh 's/define HAVE_UNISTD_H 1/define HAVE_UNISTD_H 0/g' /root/php-src/main/php_config.h

# Rename the original php_pollfd_for() implementation so that we can link our own version.
RUN /root/replace.sh 's/static inline int php_pollfd_for\(/int php_pollfd_for(php_socket_t fd, int events, struct timeval *timeouttv); static inline int __real_php_pollfd_for(/g' /root/php-src/main/php_network.h

RUN echo 'extern ssize_t wasm_read(int fd, void *buf, size_t count);' >> /root/php-src/main/php.h;
RUN /root/replace.sh 's/ret = read/ret = wasm_read/g' /root/php-src/main/streams/plain_wrapper.c

# Provide a custom implementation of the php_exec() function that handles spawning
# the process inside exec(), passthru(), system(), etc.
# We effectively remove the php_exec() implementation from the build by renaming it
# to an unused identifier "php_exec_old", and then we mark php_exec as extern.
RUN /root/replace.sh 's/PHPAPI int php_exec(.+)$/PHPAPI extern int php_exec\1; int php_exec_old\1/g' /root/php-src/ext/standard/exec.c

# Provide a custom implementation of the shutdown() function.
RUN perl -pi.bak -e $'s/(\s+)shutdown\(/$1 wasm_shutdown(/g' /root/php-src/sapi/cli/php_cli_server.c
RUN perl -pi.bak -e $'s/(\s+)closesocket\(/$1 wasm_close(/g' /root/php-src/sapi/cli/php_cli_server.c

# Provide custom implementation fo the shutdown() function.
# This is used by intl ( which links C++ code ) and need C linkage.
RUN cat >> /root/php-src/main/php_config.h <<EOF
#ifdef __cplusplus
extern "C" {
#endif

extern int wasm_shutdown(int fd, int how);
extern int wasm_close(int fd);

#ifdef __cplusplus
}
#endif
EOF

# This is used by intl ( which links C++ code ) and need C linkage. Needed in PHP < 7.4.
RUN sed -i '/^extern char \*\*environ;/d' /root/php-src/main/php.h \
 && cat <<EOF >> /root/php-src/main/php.h
#ifdef __cplusplus
extern "C" {
#endif

extern char **environ;

#ifdef __cplusplus
}
#endif
EOF

# Don't ship PHP_FUNCTION(proc_open) with the PHP build
# so that we can ship a patched version with php_wasm.c
RUN echo '' > /root/php-src/ext/standard/proc_open.h;
RUN echo '' > /root/php-src/ext/standard/proc_open.c;

ARG OUTPUT_DIR_ON_HOST
ARG DEBUG_DWARF_COMPILATION_DIR

# Compile minilua and gen_ir_fold_hash with the native GCC
RUN cd /root/php-src && \
    gcc -o ext/opcache/jit/ir/minilua \
        ext/opcache/jit/ir/dynasm/minilua.c -lm && \
    gcc -o ext/opcache/jit/ir/gen_ir_fold_hash \
        ext/opcache/jit/ir/gen_ir_fold_hash.c -lm && \
    touch ext/opcache/jit/ir/ir_emit_x86.h && \
    touch ext/opcache/jit/ir/ir_fold_hash.h


# PHP 8.5 forces the CLI — remove the CLI build rule from the Makefile
RUN grep -n "sapi/cli/php" /root/php-src/Makefile | head -5 || true && \
    sed -i 's|^\(sapi/cli/php\b.*\)|#\1|g' /root/php-src/Makefile || true && \
    sed -i '/^all:.*sapi\/cli\/php/s/ sapi\/cli\/php//' /root/php-src/Makefile || true

RUN grep -n "cli" /root/php-src/Makefile | grep -i "php$\|cli/php" | head -20 || true

RUN source /root/emsdk/emsdk_env.sh && \
	# We're compiling PHP as emscripten's side module...
	export JSPI_FLAGS=$(if [ "$WITH_JSPI" = "yes" ]; then echo "-sSUPPORT_LONGJMP=wasm -fwasm-exceptions"; else echo ""; fi) && \
	export PHP_VERSION_ESCAPED="${PHP_VERSION//./_}" && \
	EMCC_FLAGS=" -D__x86_64__ -sSIDE_MODULE -Dsetsockopt=wasm_setsockopt -Dphp_exec=wasm_php_exec \
		$JSPI_FLAGS \
		-fdebug-compilation-dir=${DEBUG_DWARF_COMPILATION_DIR}/ \
		-fdebug-prefix-map=/root/php-src/=${OUTPUT_DIR_ON_HOST}/${PHP_VERSION_ESCAPED}/php-src/ " \
	# ...which means we must skip all the libraries - they will be provided in the final linking step.
	emmake make -j14 libphp.la || emmake make -j14

RUN cp -v /root/php-src/.libs/libphp*.la /root/lib/libphp.la
RUN cp -v /root/php-src/.libs/libphp*.a /root/lib/libphp.a

COPY ./compile/php/php_wasm.c /root/
COPY ./compile/php/proc_open* /root/

ARG WITH_SOURCEMAPS
ARG OUTPUT_DIR_ON_HOST
ARG DEBUG_DWARF_COMPILATION_DIR
ARG WITH_DEBUG
RUN set -euxo pipefail; \
	if [ "$EMSCRIPTEN_ENVIRONMENT" = "node" ]; then \
		# Add nodefs when building for node.js
		echo -n ' -lnodefs.js ' >> /root/.emcc-php-wasm-flags; \
	fi; \
	if [ "${WITH_SOURCEMAPS}" = "yes" ] || [ "${WITH_DEBUG}" = "yes" ]; then \
		echo -n ' -g3 ' >> /root/.emcc-php-wasm-flags; \
		# Make source file paths relative to the location of the .wasm file
		# so we can debug without additional config in host OS.
		export PHP_VERSION_ESCAPED="${PHP_VERSION//./_}"; \
		echo -n " -fdebug-compilation-dir=${DEBUG_DWARF_COMPILATION_DIR}/ " \
			"-fdebug-prefix-map=/root/php_wasm.c=${DEBUG_DWARF_COMPILATION_DIR}/compile/php/php_wasm.c " \
			"-fdebug-prefix-map=/root/php-src/=${OUTPUT_DIR_ON_HOST}/${PHP_VERSION_ESCAPED}/php-src/ " \
			"-fdebug-prefix-map=./=${OUTPUT_DIR_ON_HOST}/${PHP_VERSION_ESCAPED}/php-src/ " \
			"-fdebug-prefix-map=/emsdk/emscripten/==${OUTPUT_DIR_ON_HOST}/${PHP_VERSION_ESCAPED}/emscripten/ " \
			"-fdebug-prefix-map=/root/emsdk/upstream/emscripten/=${OUTPUT_DIR_ON_HOST}/${PHP_VERSION_ESCAPED}/emscripten/ " \
			>> /root/.emcc-php-wasm-flags; \
		if [ "${WITH_SOURCEMAPS}" = "yes" ]; then \
			echo -n ' -gsource-map' >> /root/.emcc-php-wasm-flags; \
			# Ensure source maps are loaded from the correct URL on the web.
			# Without this, browsers will try the wasm:// protocol and never load the source map.
			if [ "$EMSCRIPTEN_ENVIRONMENT" = "web" ]; then \
				echo -n " --source-map-base http://127.0.0.1:5400/@fs${OUTPUT_DIR_ON_HOST}/${PHP_VERSION_ESCAPED}/" >> /root/.emcc-php-wasm-flags; \
			fi; \
		fi; \
	else \
		# Preserve symbol names in node.js build – the bundle size doesn't matter as much
		# as on the web, and this makes debugging **much** easier.
		if [ "$EMSCRIPTEN_ENVIRONMENT" = "node" ]; then \
			echo -n ' -g2 ' >> /root/.emcc-php-wasm-flags; \
		fi; \
	fi;

# Add ws networking proxy support if needed
RUN if [ "$WITH_WS_NETWORKING_PROXY" = "yes" ]; \
	then \
		echo -n ' -lwebsocket.js ' >> /root/.emcc-php-wasm-flags; \
	fi

# List global and exports symbols
COPY ./compile/php/exported-functions.list /root/exported-functions.list
COPY ./compile/php/zend-side-module-exports.c /root/zend-side-module-exports.c
RUN cat /root/exported-functions.list >> /root/.JS_ABI_EXPORTS

# External phpize extensions are not available while this main module is linked.
# Export the complete public symbol surface of the matching libphp archive so
# MAIN_MODULE=2 retains the PHP extension ABI for externally built side modules
# to dynamically link against.
#
# Add the ABI to the wasm export list (`-Wl,--export=`) ONLY, not to
# EXPORTED_FUNCTIONS. A side module resolves imports against the main module's
# `wasmExports` (which `--export` populates), so that is all linking needs.
# Emscripten still emits a `_sym = Module['_sym'] = wasmExports['sym']` JS alias
# for every wasm export regardless of EXPORTED_FUNCTIONS; those ABI aliases are
# dead weight (nothing in our JS/TS reads them) and are removed after linking by
# strip-unused-abi-export-aliases.mjs. Keeping .JS_ABI_EXPORTS free of the ABI
# also lets that step derive the strip set cleanly as (ABI) minus (JS exports).
#
# zif_*/zim_* (PHP_FUNCTION/PHP_METHOD handlers) are left out. They aren't
# an API: each extension registers its own through its function tables. But
# exported, a static extension's handler takes precedence over a side
# module's own definition of the same C name, since the dynamic linker
# resolves a side module's GOT imports against the main module's exports
# first. Found with php-norm (static) and intl (shared), which both define
# zif_normalizer_normalize: intl's Normalizer::normalize() ran php-norm's
# code, with php-norm's own normalization-form values ("must be a valid
# normalization form" for Normalizer::FORM_C). No side module imports a
# core zif_/zim_ symbol (checked over every packages/phpext-*).
#
# External extensions are JSPI-only (see compile-extension's ExtensionAsyncMode),
# so skip this for asyncify builds, which can never load one and would otherwise
# pay the binary-size cost of retaining the whole libphp ABI as dead weight.
RUN set -eux; \
	if [ "$WITH_JSPI" != "yes" ]; then \
		exit 0; \
	fi; \
	/root/emsdk/upstream/bin/llvm-nm \
		--defined-only \
		--extern-only \
		--format=just-symbols \
		/root/lib/libphp.a \
		| grep -v '^[[:space:]]*$' \
		| grep -v ':$' \
		| grep -v -E '^zi[fm]_' \
		| LC_ALL=C sort -u \
		> /root/.PHP_EXTENSION_ABI_EXPORTS; \
	cat /root/.PHP_EXTENSION_ABI_EXPORTS \
		>> /root/.WASM_ABI_EXPORTS

RUN set -eux; \
	if [ "$WITH_JSPI" != "yes" ]; then \
		exit 0; \
	fi; \
	PHP_VERSION_MAJOR="${PHP_VERSION%%.*}"; \
	PHP_VERSION_REST="${PHP_VERSION#*.}"; \
	PHP_VERSION_MINOR="${PHP_VERSION_REST%%.*}"; \
	{ \
		if [ "$PHP_VERSION_MAJOR" -ge 7 ]; then \
			echo 'zend_ce_exception'; \
		fi; \
		echo 'zend_ce_traversable'; \
		echo 'zend_ce_iterator'; \
		echo 'zend_ce_aggregate'; \
		echo 'zend_ce_arrayaccess'; \
		if [ "$PHP_VERSION_MAJOR" -gt 7 ] || \
			{ [ "$PHP_VERSION_MAJOR" -eq 7 ] && [ "$PHP_VERSION_MINOR" -ge 2 ]; }; then \
			echo 'zend_ce_countable'; \
		fi; \
		if [ "$PHP_VERSION_MAJOR" -ge 7 ]; then \
			echo 'zend_ce_throwable'; \
		fi; \
		if [ "$PHP_VERSION_MAJOR" -lt 9 ]; then \
			echo 'zend_ce_serializable'; \
		fi; \
		if [ "$PHP_VERSION_MAJOR" -ge 8 ]; then \
			echo 'zend_ce_stringable'; \
			echo 'zend_ce_unhandled_match_error'; \
			echo 'zend_ce_division_by_zero_error'; \
		fi; \
		if [ "$PHP_VERSION_MAJOR" -gt 8 ] || \
			{ [ "$PHP_VERSION_MAJOR" -eq 8 ] && [ "$PHP_VERSION_MINOR" -ge 1 ]; }; then \
			echo 'zend_ce_unit_enum'; \
			echo 'zend_ce_backed_enum'; \
		fi; \
		if [ "$PHP_VERSION_MAJOR" -gt 7 ] || \
			{ [ "$PHP_VERSION_MAJOR" -eq 7 ] && [ "$PHP_VERSION_MINOR" -ge 3 ]; }; then \
			echo 'zend_string_init_interned'; \
		fi; \
		if [ "$PHP_VERSION_MAJOR" -ge 7 ]; then \
			echo 'zend_empty_string'; \
			echo 'zend_one_char_string'; \
		fi; \
		echo 'std_object_handlers'; \
		echo 'executor_globals'; \
		echo 'compiler_globals'; \
		echo 'sapi_globals'; \
		echo 'file_globals'; \
		echo 'sapi_module'; \
		echo 'zend_compile_string'; \
		if [ "$PHP_VERSION_MAJOR" -ge 7 ]; then \
			echo 'zend_get_executed_scope'; \
		fi; \
		echo 'zend_execute'; \
		if [ "$PHP_VERSION_MAJOR" -ge 7 ]; then \
			echo 'zval_ptr_dtor'; \
		fi; \
		if [ "$PHP_VERSION_MAJOR" -gt 8 ] || \
			{ [ "$PHP_VERSION_MAJOR" -eq 8 ] && [ "$PHP_VERSION_MINOR" -ge 1 ]; }; then \
			echo 'zend_destroy_static_vars'; \
		fi; \
		if [ "$PHP_VERSION_MAJOR" -lt 7 ]; then \
			echo '_zval_ptr_dtor'; \
		fi; \
		echo 'destroy_op_array'; \
		echo '_efree'; \
		echo '_zend_bailout'; \
		echo 'getentropy'; \
		echo 'posix_memalign'; \
		echo 'pthread_cond_signal'; \
		echo 'pthread_cond_wait'; \
		echo 'pthread_condattr_destroy'; \
		echo 'pthread_condattr_init'; \
		echo 'pthread_condattr_setclock'; \
		echo 'pthread_mutex_trylock'; \
		echo 'pthread_mutexattr_destroy'; \
		echo 'pthread_mutexattr_init'; \
		echo 'pthread_mutexattr_settype'; \
		echo 'sched_yield'; \
		echo 'writev'; \
		if [ "$PHP_VERSION_MAJOR" -ge 8 ]; then \
			echo '_call_user_function_impl'; \
			echo 'zend_array_count'; \
			echo 'zend_declare_class_constant_ex'; \
			echo 'zend_declare_property'; \
			echo 'zend_do_implement_interface'; \
			echo 'zend_hash_get_current_key_zval_ex'; \
			echo 'zend_is_callable'; \
			echo 'zend_is_iterable'; \
			echo 'zend_lookup_class_ex'; \
			echo 'zend_objects_new'; \
			echo 'zend_register_internal_class_ex'; \
			echo 'zend_register_internal_interface'; \
			echo 'zend_std_get_properties'; \
			echo 'zend_std_has_property'; \
			echo 'zend_std_read_property'; \
			echo 'zend_std_write_property'; \
			echo 'zend_throw_exception_object'; \
		fi; \
	} >> /root/.WASM_ABI_EXPORTS

# WORKAROUND: JSPI side modules may import wasm longjmp symbols. Asyncify
# builds do not provide those symbols, so including them in the shared
# exported-functions.list would break Asyncify builds. Add them only for JSPI.
RUN if [ "$WITH_JSPI" = "yes" ]; then \
		echo '___wasm_setjmp' >> /root/.JS_ABI_EXPORTS; \
		echo '___wasm_setjmp_test' >> /root/.JS_ABI_EXPORTS; \
		echo '___wasm_longjmp' >> /root/.JS_ABI_EXPORTS; \
		echo '___c_longjmp' >> /root/.JS_ABI_EXPORTS; \
	fi;

# A non-trivial MAIN_MODULE=2 side module (e.g. sodium/mysqlnd, CLAUDE.md
# decision 32) imports the wasm-ld-synthesized dylink globals __stack_pointer
# and __table_base, which this main module must export as real
# WebAssembly.Global objects. A raw `-Wl,--export=__stack_pointer` link flag
# does NOT survive Binaryen's post-link wasm-metadce/wasm-opt DCE pass at
# -O3 -- confirmed by Emscripten's own maintainers (emscripten-core/
# emscripten#25952, filed against our exact pinned 4.0.19): -sMAIN_MODULE=2
# only keeps alive what -s EXPORTED_FUNCTIONS asks for, not arbitrary -Wl,
# flags. The fix is the same one used for JSPI's own longjmp symbols above:
# feed it through .JS_ABI_EXPORTS -> EXPORTED_FUNCTIONS, with the mandatory
# extra "_" prefix EXPORTED_FUNCTIONS requires for C symbols.
RUN echo '___stack_pointer' >> /root/.JS_ABI_EXPORTS; \
	echo '___table_base' >> /root/.JS_ABI_EXPORTS;

# Two more real, concretely-observed side-module symbol gaps, same shape as
# __stack_pointer/__table_base above but for different reasons -- found by
# actually loading sodium.so/mysqlnd.so against this main module and reading
# the exact "could not resolve symbol" failure:
# - atoll: mysqlnd.c calls it, but this main module's own code never does,
#   so wasm-ld's DCE drops it entirely (a real, ordinary libc function --
#   forcing it into EXPORTED_FUNCTIONS is enough, same mechanism as above;
#   mergeLibSymbols() in the generated JS then copies it into wasmImports
#   automatically since it becomes a real wasm export).
RUN echo '_atoll' >> /root/.JS_ABI_EXPORTS;

# - emscripten_asm_const_int: libsodium's own randombytes_js.c uses EM_ASM_INT
#   to reach real JS-side randomness in the browser/Node. This is NOT a real C
#   symbol -- it's a pure JS runtime helper Emscripten only generates into the
#   output when SOME compiled code in this exact link uses EM_ASM, which this
#   main module's own code never does. Forcing it into EXPORTED_FUNCTIONS (like
#   atoll above) makes Emscripten generate the `_emscripten_asm_const_int` JS
#   function, but -- unlike atoll -- it produces no real wasm export for
#   mergeLibSymbols() to pick up automatically (it has no wasm code at all), so
#   it never reaches the JS-side wasmImports object side modules resolve
#   against. Verified directly (throwaway emcc test against this same base
#   image, MAIN_MODULE=2, before touching this Dockerfile): the JS function is
#   generated correctly, but wasmImports stays untouched. Fixed with the
#   companion JS patch below ("Make emscripten_asm_const_int resolvable...").
RUN echo '_emscripten_asm_const_int' >> /root/.JS_ABI_EXPORTS;

# - free/strlen: same shape as atoll -- real libc functions this main module
#   only ever calls internally (direct wasm calls, no export needed for that),
#   so wasm-ld's DCE drops the export even though the function itself is
#   still compiled in. sodium.so calls free(), mysqlnd.so calls strlen();
#   found the same way as atoll, one real runtime failure at a time.
RUN echo '_free' >> /root/.JS_ABI_EXPORTS; \
	echo '_strlen' >> /root/.JS_ABI_EXPORTS;

# - stderr/stdin: same shape, same reason (openssl/zlib linked into
#   mysqlnd.so).
# - stdout: same shape, observed for real 2026-09-17 -- bz2.so's vendored
#   libbz2.a (bzlib.c's BZ2_bzWriteOpen/BZ2_bzReadOpen file-stream API,
#   compiled in even though the PHP extension itself never calls those
#   entry points, ext/bz2 only using the in-memory
#   BZ2_bzBuffToBuffCompress/Decompress functions) references the global
#   `stdout` FILE* -- confirmed by check-shared-extension-symbols.mjs's own
#   "Found 1 missing ABI export(s): - stdout" report, not guessed.
RUN echo '_stderr' >> /root/.JS_ABI_EXPORTS; \
	echo '_stdin' >> /root/.JS_ABI_EXPORTS; \
	echo '_stdout' >> /root/.JS_ABI_EXPORTS;

# - strcmp/memset: same shape as atoll/free/strlen above -- mysqlnd.so/
#   mysqli.so call them directly, this main module never does, so DCE drops
#   the export even though the functions are still compiled in. Found via
#   compile/check-shared-extension-symbols.mjs (a throwaway-patched copy of
#   the already-built php_*.js that logs every currently-missing GOT symbol
#   in one run instead of aborting on the first one), which is why both
#   landed in the same commit despite being two distinct symbols.
RUN echo '_strcmp' >> /root/.JS_ABI_EXPORTS; \
	echo '_memset' >> /root/.JS_ABI_EXPORTS;

# - strcasecmp: dba.so calls it (its handler-name lookup table does a
#   case-insensitive compare) -- same shape as strcmp/memset above, but
#   this one is resolved LAZILY (only when actually CALLED, via the dynamic
#   linker's JS proxy stub), not eagerly via the GOT -- found by extending
#   check-shared-extension-symbols.mjs to also patch that lazy-resolution
#   site, not just the eager one, after it slipped past the first pass.
RUN echo '_strcasecmp' >> /root/.JS_ABI_EXPORTS;

# - atoi/memcmp: libflatfile/flatfile.c calls both (atoi parsing a numeric
#   header field, memcmp comparing key bytes on lookup) -- also lazily
#   resolved, only reachable by actually exercising a real dba_open()/
#   dba_insert()/dba_fetch() round-trip, which is why both slipped past
#   check-shared-extension-symbols.mjs's default smoke test (just
#   get_loaded_extensions()) and needed the fuller functional test to
#   surface, one at a time.
RUN echo '_atoi' >> /root/.JS_ABI_EXPORTS; \
	echo '_memcmp' >> /root/.JS_ABI_EXPORTS;

# - vfprintf/fscanf/fgetc/ungetc: gmp.so calls these (GMP's own I/O
#   functions -- gmp_fprintf()/gmp_fscanf() and friends -- reach them, same
#   DCE-drops-the-export shape as strcmp/memset above). realloc/__cxa_atexit:
#   soap.so calls both directly. memchr/strstr/strncmp/strdup/strchrnul/
#   strcpy: pgsql.so/pdo_pgsql.so (real fe-connect.c connection-
#   establishment code, exercised via each extension's own smoke-test.php
#   connecting to a closed local port rather than a real PostgreSQL server --
#   same "exercise the real lazy code path" reasoning as dba's own
#   atoi/memcmp above). All ten found in the same check-shared-extension-
#   symbols.mjs run, once gmp/soap/pgsql/pdo_pgsql all had packages built.
RUN echo '_vfprintf' >> /root/.JS_ABI_EXPORTS; \
	echo '_fscanf' >> /root/.JS_ABI_EXPORTS; \
	echo '_fgetc' >> /root/.JS_ABI_EXPORTS; \
	echo '_ungetc' >> /root/.JS_ABI_EXPORTS; \
	echo '_realloc' >> /root/.JS_ABI_EXPORTS; \
	echo '___cxa_atexit' >> /root/.JS_ABI_EXPORTS; \
	echo '_memchr' >> /root/.JS_ABI_EXPORTS; \
	echo '_strstr' >> /root/.JS_ABI_EXPORTS; \
	echo '_strncmp' >> /root/.JS_ABI_EXPORTS; \
	echo '_strdup' >> /root/.JS_ABI_EXPORTS; \
	echo '_strchrnul' >> /root/.JS_ABI_EXPORTS; \
	echo '_strcpy' >> /root/.JS_ABI_EXPORTS;

# - getenv/geteuid/getpwuid_r: pgsql.so/pdo_pgsql.so again, one layer
#   further into the same fe-connect.c connection path than the batch just
#   above (PGPASSWORD/PGUSER env lookups, then falling back to the calling
#   OS user's name via getpwuid_r when no user is given) -- only surfaced
#   once the previous batch's missing symbols stopped aborting the attempt
#   before reaching this point, same "found one layer at a time" shape as
#   the SysV IPC discovery process described below.
RUN echo '_getenv' >> /root/.JS_ABI_EXPORTS; \
	echo '_geteuid' >> /root/.JS_ABI_EXPORTS; \
	echo '_getpwuid_r' >> /root/.JS_ABI_EXPORTS;

# - vsnprintf: tidy.so (libtidy's own message-formatting code) calls it
#   directly -- same DCE-drops-the-export shape as strcmp/memset above.
#   Found via a real tidy_parse_string()/cleanRepair() smoke-test.php round
#   trip, not just loading the module.
RUN echo '_vsnprintf' >> /root/.JS_ABI_EXPORTS;

# - wctomb/wcstombs/mbtowc/mbstowcs/strchr/fopen/getuid/siprintf/snprintf/
#   gai_strerror: ldap.so (libldap's own connection/charset-conversion code)
#   calls all ten directly -- same DCE-drops-the-export shape as
#   strcmp/memset above. Found via a real ldap_bind() smoke-test.php
#   attempt against a closed local port (ldap_connect() itself is lazy and
#   never opens a socket on its own), not just loading the module.
RUN echo '_wctomb' >> /root/.JS_ABI_EXPORTS; \
	echo '_wcstombs' >> /root/.JS_ABI_EXPORTS; \
	echo '_mbtowc' >> /root/.JS_ABI_EXPORTS; \
	echo '_mbstowcs' >> /root/.JS_ABI_EXPORTS; \
	echo '_strchr' >> /root/.JS_ABI_EXPORTS; \
	echo '_fopen' >> /root/.JS_ABI_EXPORTS; \
	echo '_getuid' >> /root/.JS_ABI_EXPORTS; \
	echo '_siprintf' >> /root/.JS_ABI_EXPORTS; \
	echo '_snprintf' >> /root/.JS_ABI_EXPORTS; \
	echo '_gai_strerror' >> /root/.JS_ABI_EXPORTS;

# - strtol/socket/fcntl/setsockopt/inet_ntop/connect/freeaddrinfo/time:
#   ldap.so again, one layer further into the same connection path than the
#   batch just above (the real socket()/connect() syscalls, only reached
#   once the previous batch's missing symbols stopped aborting the attempt
#   before getting there) -- same "found one layer at a time" shape as
#   pgsql's own getenv/geteuid/getpwuid_r batch.
RUN echo '_strtol' >> /root/.JS_ABI_EXPORTS; \
	echo '_socket' >> /root/.JS_ABI_EXPORTS; \
	echo '_fcntl' >> /root/.JS_ABI_EXPORTS; \
	echo '_setsockopt' >> /root/.JS_ABI_EXPORTS; \
	echo '_inet_ntop' >> /root/.JS_ABI_EXPORTS; \
	echo '_connect' >> /root/.JS_ABI_EXPORTS; \
	echo '_freeaddrinfo' >> /root/.JS_ABI_EXPORTS; \
	echo '_time' >> /root/.JS_ABI_EXPORTS;

# - poll/shutdown/close: ldap.so again, one more layer into the same
#   connection path (closing/cleaning up the socket after the closed-port
#   connection attempt fails) -- same "found one layer at a time" shape.
#   poll itself is no longer listed: the core is linked with
#   -Wl,--wrap=poll, so like getpid (below) there is no plain "_poll"
#   export; side modules get __wrap_poll through a wasmImports alias.
RUN echo '_shutdown' >> /root/.JS_ABI_EXPORTS; \
	echo '_close' >> /root/.JS_ABI_EXPORTS;

# - environ: enchant.so (glib's own g_get_environ()/getenv-adjacent code)
#   references the `environ` global directly (a data symbol, not a
#   function -- exported the same way as stdin/stdout/stderr above).
RUN echo '_environ' >> /root/.JS_ABI_EXPORTS;

# - pthread_key_create/pthread_getspecific/sysconf/fputs/sprintf/fwrite/
#   abort: enchant.so again -- glib's own static initializers (thread-local
#   storage setup, standard I/O) call these directly during
#   __wasm_call_ctors, before any PHP code runs. Same DCE-drops-the-export
#   shape as strcmp/memset above; found via load-only checking since this
#   extension has no smoke-test.php yet (no spell-checking provider vendored
#   to actually exercise -- see compile/extensions/enchant/PROVENANCE.md).
RUN echo '_pthread_key_create' >> /root/.JS_ABI_EXPORTS; \
	echo '_pthread_getspecific' >> /root/.JS_ABI_EXPORTS; \
	echo '_sysconf' >> /root/.JS_ABI_EXPORTS; \
	echo '_fputs' >> /root/.JS_ABI_EXPORTS; \
	echo '_sprintf' >> /root/.JS_ABI_EXPORTS; \
	echo '_fwrite' >> /root/.JS_ABI_EXPORTS; \
	echo '_abort' >> /root/.JS_ABI_EXPORTS;

# - gettimeofday/pthread_setspecific: enchant.so again, same
#   __wasm_call_ctors static-init path as the pthread/stdio batch above --
#   found once those stopped the module from crashing before reaching them
#   (the "RuntimeError: unreachable" trap this run's predecessor hit turned
#   out to be a direct consequence of the missing pthread_key_create/
#   pthread_getspecific/etc, not a separate bug).
RUN echo '_gettimeofday' >> /root/.JS_ABI_EXPORTS; \
	echo '_pthread_setspecific' >> /root/.JS_ABI_EXPORTS;

# - bindtextdomain/textdomain/gettext/dgettext/dcgettext/ngettext/dngettext/
#   dcngettext/bind_textdomain_codeset: gettext.so calls musl's own libintl
#   functions (no vendored libintl), and enchant.so's glib calls the same
#   ones for its own message translation. Lazy symbols, only reached by
#   actually calling them -- invisible to load-only checking, found by real
#   calls (2026-09-22) once gettext/enchant got a smoke-test.php.
RUN echo '_bindtextdomain' >> /root/.JS_ABI_EXPORTS; \
	echo '_textdomain' >> /root/.JS_ABI_EXPORTS; \
	echo '_gettext' >> /root/.JS_ABI_EXPORTS; \
	echo '_dgettext' >> /root/.JS_ABI_EXPORTS; \
	echo '_dcgettext' >> /root/.JS_ABI_EXPORTS; \
	echo '_ngettext' >> /root/.JS_ABI_EXPORTS; \
	echo '_dngettext' >> /root/.JS_ABI_EXPORTS; \
	echo '_dcngettext' >> /root/.JS_ABI_EXPORTS; \
	echo '_bind_textdomain_codeset' >> /root/.JS_ABI_EXPORTS;

# - bsearch/iconv_open/iconv/iconv_close: soap.so links its own copy of
#   libxml2 (vendorLib: libxml2 -- this main module exports none of its
#   own), and that copy's encoding layer calls musl's iconv (not the
#   vendored GNU libiconv the core's ext/iconv uses, whose symbols are
#   renamed libiconv_*). Without them SoapServer::handle() fails with
#   "Dump memory failed". glib (enchant.so) calls the same iconv trio.
RUN echo '_bsearch' >> /root/.JS_ABI_EXPORTS; \
	echo '_iconv_open' >> /root/.JS_ABI_EXPORTS; \
	echo '_iconv' >> /root/.JS_ABI_EXPORTS; \
	echo '_iconv_close' >> /root/.JS_ABI_EXPORTS;

# - nl_langinfo/strncpy/pthread_cond_broadcast/setlocale/open/fstat/read/
#   fileno/isatty/strcat/localtime/strftime: enchant.so again -- glib's own
#   charset/locale detection, file I/O and time formatting, reached by
#   enchant_broker_init()/enchant_broker_describe(), not by loading alone.
RUN echo '_nl_langinfo' >> /root/.JS_ABI_EXPORTS; \
	echo '_strncpy' >> /root/.JS_ABI_EXPORTS; \
	echo '_pthread_cond_broadcast' >> /root/.JS_ABI_EXPORTS; \
	echo '_setlocale' >> /root/.JS_ABI_EXPORTS; \
	echo '_open' >> /root/.JS_ABI_EXPORTS; \
	echo '_fstat' >> /root/.JS_ABI_EXPORTS; \
	echo '_read' >> /root/.JS_ABI_EXPORTS; \
	echo '_fileno' >> /root/.JS_ABI_EXPORTS; \
	echo '_isatty' >> /root/.JS_ABI_EXPORTS; \
	echo '_strcat' >> /root/.JS_ABI_EXPORTS; \
	echo '_localtime' >> /root/.JS_ABI_EXPORTS; \
	echo '_strftime' >> /root/.JS_ABI_EXPORTS;

# - fclose/stat/fgets: enchant.so again, one layer further -- libenchant's
#   personal word list (enchant_broker_request_pwl_dict()) reading its file,
#   only reached once glib's GFunc-cast crash (compile/libglib/Dockerfile)
#   stopped enchant_broker_init() from trapping first.
RUN echo '_fclose' >> /root/.JS_ABI_EXPORTS; \
	echo '_stat' >> /root/.JS_ABI_EXPORTS; \
	echo '_fgets' >> /root/.JS_ABI_EXPORTS;

# - toupper/getpwuid/strncat: odbc.so (unixODBC's driver manager -- its ini
#   parser upper-cases section/key names, and odbcinst resolves the user's
#   ~/.odbc.ini through getpwuid()), reached by odbc_connect()'s DSN lookup
#   in its smoke-test.php, not by loading alone. enchant.so imports
#   toupper/getpwuid too.
RUN echo '_toupper' >> /root/.JS_ABI_EXPORTS; \
	echo '_getpwuid' >> /root/.JS_ABI_EXPORTS; \
	echo '_strncat' >> /root/.JS_ABI_EXPORTS;

# - strrchr/access: odbc.so/pdo_odbc.so again, one layer further -- the
#   driver manager's driver-loading path (splitting and probing a DRIVER=
#   library path before lt_dlopen), only reached once the batch above
#   stopped the DSN lookup from failing first.
RUN echo '_strrchr' >> /root/.JS_ABI_EXPORTS; \
	echo '_access' >> /root/.JS_ABI_EXPORTS;

# - strlcpy/dlopen/dlerror: odbc.so/pdo_odbc.so again, the last layer --
#   libltdl's own dlopen loader actually trying to load the driver library
#   (and reading back why it failed). dlopen/dlerror are the main module's
#   own dynamic-linking entry points (it already loads every mode:shared
#   extension); exporting them lets a side module call them directly too,
#   which is also what a future ODBC driver built as its own side module
#   would load through.
RUN echo '_strlcpy' >> /root/.JS_ABI_EXPORTS; \
	echo '_dlopen' >> /root/.JS_ABI_EXPORTS; \
	echo '_dlerror' >> /root/.JS_ABI_EXPORTS;

# - socketpair/gethostname/strerror/asprintf: pdo_dblib.so (FreeTDS's
#   db-lib login + TDS connection code: gethostname() for the login
#   packet's client host name, socketpair() for each connection's wakeup
#   channel, strerror()/asprintf() for its error messages). Found via a
#   real new PDO('dblib:...') against a closed local port in its
#   smoke-test.php, not just loading the module.
RUN echo '_socketpair' >> /root/.JS_ABI_EXPORTS; \
	echo '_gethostname' >> /root/.JS_ABI_EXPORTS; \
	echo '_strerror' >> /root/.JS_ABI_EXPORTS; \
	echo '_asprintf' >> /root/.JS_ABI_EXPORTS;

# - pdo_firebird.so (libfbclient, the first C++ side module; its C++
#   runtime is linked into the .so itself, see compile/firebird/Dockerfile):
#   newlocale/uselocale (libc++'s locale support), __small_sprintf (musl's
#   integer-only sprintf variant clang emits for some sprintf calls), and
#   what libfbclient's own startup and attach path touches: pthread_atfork/
#   pthread_create/pthread_setcanceltype/pthread_rwlock_init (its init and
#   timer thread code), sem_init, mmap, fegetenv/fesetenv (FpeControl),
#   sigemptyset/sigaddset/sigaction (its signal handler setup), setenv.
#   Found by check-shared-extension-symbols.mjs running its smoke-test.php
#   (a real new PDO('firebird:...') against a closed local port). A second
#   layer, only reached once the first batch was exported:
#   __ctype_get_mb_cur_max/aligned_alloc (libc++), sem_destroy,
#   pthread_once.
RUN echo '___ctype_get_mb_cur_max' >> /root/.JS_ABI_EXPORTS; \
	echo '_aligned_alloc' >> /root/.JS_ABI_EXPORTS; \
	echo '_sem_destroy' >> /root/.JS_ABI_EXPORTS; \
	echo '_pthread_once' >> /root/.JS_ABI_EXPORTS; \
	echo '_newlocale' >> /root/.JS_ABI_EXPORTS; \
	echo '_uselocale' >> /root/.JS_ABI_EXPORTS; \
	echo '___small_sprintf' >> /root/.JS_ABI_EXPORTS; \
	echo '_pthread_atfork' >> /root/.JS_ABI_EXPORTS; \
	echo '_pthread_create' >> /root/.JS_ABI_EXPORTS; \
	echo '_pthread_setcanceltype' >> /root/.JS_ABI_EXPORTS; \
	echo '_pthread_rwlock_init' >> /root/.JS_ABI_EXPORTS; \
	echo '_sem_init' >> /root/.JS_ABI_EXPORTS; \
	echo '_mmap' >> /root/.JS_ABI_EXPORTS; \
	echo '_fegetenv' >> /root/.JS_ABI_EXPORTS; \
	echo '_fesetenv' >> /root/.JS_ABI_EXPORTS; \
	echo '_sigemptyset' >> /root/.JS_ABI_EXPORTS; \
	echo '_sigaddset' >> /root/.JS_ABI_EXPORTS; \
	echo '_sigaction' >> /root/.JS_ABI_EXPORTS; \
	echo '_setenv' >> /root/.JS_ABI_EXPORTS;

# - dlsym/dlclose: ffi.so. FFI::cdef() with no library resolves every C
#   function it declares with dlsym(RTLD_DEFAULT, ...), and FFI::load()/
#   FFI::cdef($code, $lib) pair dlopen() (already exported above, for odbc)
#   with dlclose(). Found by check-shared-extension-symbols.mjs running
#   ffi's smoke-test.php. qsort is exported too: that smoke test calls it
#   through FFI (with a PHP callback, to exercise libffi's closures), and
#   the core's own code never does, so without this FFI couldn't resolve it
#   (a C function FFI is asked to call must be a real export of the core,
#   or of a loaded side module).
# The bulk list for the 2026-09-23 batch of side modules (ffi, fileinfo,
# xsl, snmp, rar, intl, scanmeqr), computed statically instead of one smoke-test
# layer at a time: see the header of compile/php/side-module-abi-exports.txt.
COPY ./compile/php/side-module-abi-exports.txt /root/side-module-abi-exports.txt
RUN grep -v '^#' /root/side-module-abi-exports.txt | grep -v '^[[:space:]]*$' | sed 's/^/_/' >> /root/.JS_ABI_EXPORTS

RUN echo '_dlsym' >> /root/.JS_ABI_EXPORTS; \
	echo '_dlclose' >> /root/.JS_ABI_EXPORTS; \
	echo '_qsort' >> /root/.JS_ABI_EXPORTS;

# - getpid: posix.so's posix_getpid() calls it directly -- found via
#   check-shared-extension-symbols.mjs's smoke-test.php support (CLAUDE.md
#   decision 48's batch of no-external-lib extensions: posix/shmop/sysvshm/
#   sysvmsg/sysvsem). NOT the same fix shape as atoi/memcmp above: this
#   main module already -Wl,--wrap=getpid (see the emcc invocation below,
#   PLATFORM_SPECIFIC_ARGS for EMSCRIPTEN_ENVIRONMENT=node), so there is no
#   real "_getpid" export at all -- the linker splits it into __real_getpid
#   (unused here) and __wrap_getpid, and a bare `echo '_getpid'` here fails
#   the link outright ("undefined exported symbol: _getpid"). __wrap_getpid
#   is already generated as Module["___wrap_getpid"] regardless (the wrap
#   mechanism itself, no .JS_ABI_EXPORTS entry needed for that part) -- it
#   just never reaches wasmImports for side modules to resolve against,
#   same gap as emscripten_asm_const_int below. Aliased with the matching
#   JS patch instead of an export line, see "Make getpid resolvable..." below.

# - shmget/shmat/shmctl/shmdt (shmop.so/sysvshm.so), msgget/msgsnd/msgrcv
#   (sysvmsg.so), semget/semop/semctl (sysvsem.so): all real SysV IPC libc
#   calls these extensions call directly -- same DCE-drops-the-export shape
#   as atoll/strcmp/memset/etc above (this main module's own code never
#   calls any of them), found all in one check-shared-extension-symbols.mjs
#   run (CLAUDE.md decision 48's batch). Exporting them does NOT mean they
#   actually work -- CLAUDE.md decision 43 already found Emscripten's libc
#   declares these SysV syscalls without a real implementation (broke
#   apcu's own optional shm backend, needed a source patch to stub it out)
#   -- this export just lets the *real* (likely failing) call happen,
#   instead of an artificial "undefined symbol" crash; each extension's own
#   smoke-test.php tolerates a graceful PHP-level failure here as an
#   acceptable, expected-until-proven-otherwise outcome.
#   None of them exist as exportable symbols under this toolchain --
#   real, concretely observed build failures, found one at a time as each
#   was removed in turn (msgctl, then msgget, then semctl, then shmat --
#   at which point the process was stopped rather than continuing the same
#   one-symbol-per-rebuild cycle for the remaining ones under time
#   pressure). This confirms CLAUDE.md decision 43's own finding for
#   apcu's optional shm backend generalizes to the whole SysV IPC family,
#   not just shmget/shmat/shmctl/shmdt: Emscripten's libc does not
#   implement any of it at the symbol-table level, only declares it in
#   headers. shmop.so/sysvshm.so/sysvmsg.so/sysvsem.so's own calls into all
#   of these are left as unresolved lazy stubs -- fine, since all four
#   extensions' own smoke-test.php already tolerate a graceful failure
#   here. Left un-exported entirely rather than re-attempting individually;
#   revisit only with a real reason to (e.g. a patched Emscripten libc, or
#   accepting the cost of confirming each one true/false one rebuild at a
#   time when there's no time pressure).

RUN if [ "$(echo "$PHP_VERSION" | cut -d. -f1)" -ge 8 ] && [ "$WITH_JSPI" = "yes" ]; then \
		{ \
			echo '_clock_gettime'; \
			echo '_getcwd'; \
			echo '_pthread_cond_destroy'; \
			echo '_pthread_cond_init'; \
			echo '_pthread_cond_timedwait'; \
			echo '_pthread_mutex_destroy'; \
			echo '_pthread_mutex_init'; \
			echo '_pthread_mutex_lock'; \
			echo '_pthread_mutex_unlock'; \
			echo '_strerror_r'; \
			echo '_write'; \
			echo '_zend_hash_get_current_data_ex'; \
			echo '_zend_hash_get_current_key_type_ex'; \
			echo '_zend_hash_move_forward_ex'; \
			echo '_zend_is_true'; \
		} >> /root/.JS_ABI_EXPORTS; \
	fi;

COPY ./${EMSCRIPTEN_ENVIRONMENT}-builds/ /root/${EMSCRIPTEN_ENVIRONMENT}-builds
COPY ./compile/shared/ /root/shared/
RUN	set -eux; \
	PHP_VERSION_DIR="$(echo "$PHP_VERSION" | cut -d. -f1,2 | tr . -)"; \
	ASYNCIFY_OR_JSPI_DIR=$( [ "$WITH_JSPI" = "yes" ] && echo "jspi" || echo "asyncify" ); \
	EXTENSIONS_DIR_PATH="/root/${EMSCRIPTEN_ENVIRONMENT}-builds/${PHP_VERSION_DIR}/${ASYNCIFY_OR_JSPI_DIR}/extensions"; \
	for path in "$EXTENSIONS_DIR_PATH"/*; do \
		extension="${path##*/}"; \
		# List globals symbols [ env func and globals without invoke_ and libc++(_Z) prefixed ones ]
		/root/emsdk/upstream/bin/wasm-dis "$EXTENSIONS_DIR_PATH/${extension}/${extension}.so" \
		| sed -n 's/^[[:space:]]*(import "env" "\([^"]*\)"[[:space:]]*(\(func\|global\).*/\1/p' \
		| sed '/^invoke_/d' \
		| sed '/^_Z/d' \
		| sed 's/^/_/' \
		| sort \
		>> /root/.JS_ABI_EXPORTS; \
		# List exports symbols [ GOT.mem and GOT.func globals ]
		/root/emsdk/upstream/bin/wasm-dis "$EXTENSIONS_DIR_PATH/${extension}/${extension}.so" \
		| sed -n 's/^[[:space:]]*(import "\(GOT\.mem\|GOT\.func\)" "\([^"]*\)"[[:space:]]*(global.*/\2/p' \
		| sort \
		>> /root/.WASM_ABI_EXPORTS; \
		# Inject extension-specific exported functions
		if [ -f /root/shared/${extension}/exported-functions.list ]; then \
			cat /root/shared/${extension}/exported-functions.list >> /root/.JS_ABI_EXPORTS; \
		fi; \
	done

RUN set -eux; \
	sort -u /root/.JS_ABI_EXPORTS | \
	grep -v '^[[:space:]]*$' | \
	sed 's/^/  "/; s/$/",/' | \
	sed '$ s/,$//' | \
	{ echo '['; cat; echo ']'; } \
	> /root/.exported-functions

RUN set -eux; \
	sort -u /root/.WASM_ABI_EXPORTS | \
	grep -v '^[[:space:]]*$' | \
	sed 's/^/-Wl,--export=/' \
	> /root/.wasm-exports

# Derive the JS aliases to strip after linking: the libphp ABI symbols that are
# exported to wasm for side-module linking but are NOT part of the JS-facing
# export set (.JS_ABI_EXPORTS). These get a dead `Module['_sym']` alias in the
# generated loader that nothing reads. Names use Emscripten's JS leading
# underscore so they match the loader's alias declarations directly.
RUN set -eux; \
	: > /root/.PHP_EXTENSION_ABI_JS_STRIP; \
	if [ -s /root/.PHP_EXTENSION_ABI_EXPORTS ]; then \
		sed 's/^/_/' /root/.PHP_EXTENSION_ABI_EXPORTS \
			| grep -v '^[[:space:]]*$' \
			| LC_ALL=C sort -u > /root/.abi-js-names; \
		LC_ALL=C sort -u /root/.JS_ABI_EXPORTS \
			| grep -v '^[[:space:]]*$' > /root/.js-keep-names; \
		LC_ALL=C comm -23 /root/.abi-js-names /root/.js-keep-names \
			> /root/.PHP_EXTENSION_ABI_JS_STRIP; \
	fi


# Build the final .wasm file
RUN mkdir /root/output
COPY ./compile/php/phpwasm-emscripten-library.js /root/phpwasm-emscripten-library.js
COPY ./compile/php/phpwasm-emscripten-library-dynamic-linking.js /root/phpwasm-emscripten-library-dynamic-linking.js
COPY ./compile/php/phpwasm-emscripten-library-file-locking-for-node.js /root/phpwasm-emscripten-library-file-locking-for-node.js
COPY ./compile/php/phpwasm-emscripten-library-dns-lookup-for-node.js /root/phpwasm-emscripten-library-dns-lookup-for-node.js
COPY ./compile/php/phpwasm-emscripten-library-known-undefined-functions.js /root/phpwasm-emscripten-library-known-undefined-functions.js
ARG WITH_SOURCEMAPS
ARG WITH_DEBUG
ARG STACK_SIZE

RUN cat /root/.exported-functions | grep -n "FS" || true

# Nettoyer les exports corrompus (retours de ligne dans les noms)
RUN sed -i '/^$/d' /root/.exported-functions && \
    sed -i 's/\r//g' /root/.exported-functions && \
    tr -d '\r' < /root/.JS_ABI_EXPORTS > /tmp/exports_clean && \
    mv /tmp/exports_clean /root/.JS_ABI_EXPORTS


RUN cat > /root/jit_stubs.c << 'EOF'
#include <stddef.h>
#include <stdint.h>

int zend_jit_startup_ok = 0;
void* jit_globals = NULL;

void zend_jit_activate(void) {}
void zend_jit_blacklist_function(void* op_array) {}
int  zend_jit_check_support(void) { return 0; }
int  zend_jit_config(void* jit_config, int stage) { return 0; }
void zend_jit_deactivate(void) {}
int  zend_jit_debug_config(long long old_val, long long new_val, int stage) { return 0; }
void zend_jit_init(void) {}
int  zend_jit_op_array(void* op_array, void* script) { return 0; }
void zend_jit_protect(void) {}
void zend_jit_restart(void) {}
int  zend_jit_script(void* script) { return 0; }
void zend_jit_shutdown(void) {}
void zend_jit_startup(void* buf, size_t size, int reattached) {}
void zend_jit_status(void* zv) {}
void zend_jit_unprotect(void) {}
EOF


RUN set -euxo pipefail; \
	mkdir -p /build/output; \
	source /root/emsdk/emsdk_env.sh; \
	ZEND_SIDE_MODULE_EXPORT_SOURCE=""; \
	# Both imports and exports are required for inter-module communication with wrapped methods, e.g., wasm_recv.
	# JSPI only (see CLAUDE.md decision 6) — the real-Asyncify branch this
	# used to fall back to (ASYNCIFY=1, unreachable since WITH_JSPI is
	# always "yes" in this pipeline) was removed along with the dead
	# ASYNCIFY_IMPORTS/ASYNCIFY_ONLY block that fed it.
	export ASYNCIFY_FLAGS=" -s ASYNCIFY=2 -sSUPPORT_LONGJMP=wasm -fwasm-exceptions -sJSPI_IMPORTS=js_open_process,js_fd_read,js_waitpid,js_process_status,js_create_input_device,wasm_shutdown,wasm_close,wasm_recv,wasm_recvfrom,__syscall_recvfrom,wasm_connect,__syscall_connect,__syscall_fcntl64,js_flock,js_release_file_locks,js_waitpid -sJSPI_EXPORTS=php_wasm_init,wasm_sleep,wasm_read,emscripten_sleep,wasm_sapi_handle_request,wasm_sapi_request_shutdown,wasm_poll_socket,wrap_select,__wrap_select,select,__wrap_poll,php_pollfd_for,fflush,wasm_popen,wasm_pclose,__wrap_popen,__wrap_pclose,wasm_read,wasm_php_exec,run_cli,wasm_recv,wasm_connect,__wasm_call_ctors,__errno_location,__funcs_on_exit -s EXPORTED_RUNTIME_METHODS=HEAPU32,HEAPU8,ccall,PROXYFS,wasmExports "; \
	echo '#define PLAYGROUND_JSPI 1' > /root/php_wasm_asyncify.h; \
	ZEND_SIDE_MODULE_EXPORT_SOURCE="/root/zend-side-module-exports.c"; \
	export OPTIMIZATION_FLAGS="-O3"; \
	if [ "${WITH_SOURCEMAPS}" = "yes" ] || [ "${WITH_DEBUG}" = "yes" ]; then \
		export OPTIMIZATION_FLAGS="-O0"; \
	fi; \
	PLATFORM_SPECIFIC_ARGS=''; \
	ENVIRONMENT="$EMSCRIPTEN_ENVIRONMENT"; \
	if [ "$EMSCRIPTEN_ENVIRONMENT" = "node" ]; then \
		PLATFORM_SPECIFIC_ARGS="$PLATFORM_SPECIFIC_ARGS -DPHP_WASM_FILE_LOCKING_SUPPORT=1"; \
		PLATFORM_SPECIFIC_ARGS="$PLATFORM_SPECIFIC_ARGS --js-library /root/phpwasm-emscripten-library-file-locking-for-node.js --js-library /root/phpwasm-emscripten-library-dns-lookup-for-node.js -Wl,--wrap=getpid "; \
	else \
		# Prevents "document is not defined" errors in Web Workers when using MAIN_MODULE.
		# By default, Emscripten assumes MAIN_MODULE runs only in a web environment,
		# so it injects WebGL and other browser-only APIs. Explicitly adding "worker"
		# ensures the runtime is compatible with Web Workers as well.
		ENVIRONMENT="$EMSCRIPTEN_ENVIRONMENT,worker"; \
	fi; \
	emcc $OPTIMIZATION_FLAGS \
	--js-library /root/phpwasm-emscripten-library.js \
	--js-library /root/phpwasm-emscripten-library-dynamic-linking.js \
	--js-library /root/phpwasm-emscripten-library-known-undefined-functions.js \
	$PLATFORM_SPECIFIC_ARGS \
	-I .  \
	-I ext   \
	-I ext/json   \
	-I Zend  \
	-I main  \
	-I TSRM/ \
	-I/root/lib -I/root/lib/include \
	-L/root/lib -L/root/lib/lib/ \
	-D__x86_64__\
	-lproxyfs.js \
	$ASYNCIFY_FLAGS \
	$(cat /root/.emcc-php-wasm-flags) \
	-s MAIN_MODULE=2 \
	-s EXPORTED_FUNCTIONS="@/root/.exported-functions" \
	-s WASM_BIGINT=1 \
	-s MIN_NODE_VERSION=200900 \
	-s INITIAL_MEMORY=64MB \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s STACK_SIZE="$STACK_SIZE" \
	-s ASSERTIONS=0 \
	-s ERROR_ON_UNDEFINED_SYMBOLS=1 \
	-s NODEJS_CATCH_EXIT=0 \
	-s NODEJS_CATCH_REJECTION=0 \
	-s INVOKE_RUN=0 \
	-o /build/output/php.js \
	-s EXIT_RUNTIME=1 \
	-Wl,@/root/.wasm-exports \
	-Wl,--wrap=usleep \
	-Wl,--wrap=select \
	-Wl,--wrap=poll \
	-Wl,--wrap=popen \
	-Wl,--wrap=pclose \
		/root/lib/libphp.a \
		/root/proc_open.c \
		/root/php_wasm.c \
		/root/jit_stubs.c \
		$ZEND_SIDE_MODULE_EXPORT_SOURCE \
		$(cat /root/.emcc-php-wasm-sources) \
	-s ENVIRONMENT=$ENVIRONMENT \
	-s FORCE_FILESYSTEM=1 \
	-s EXPORT_NAME="'PHPLoader'"

	# Emscripten complains it can't find some Asyncify functions
	# listed in ASYNCIFY_IMPORTS. The culprit is those functions
	# are actually used and delisting them breaks php_pollfd_for().
	# Sooo... just ignore Emscripten's warnings.

# # At the moment terminfo is baked into the repository in
# # src/php-cli/terminfo, but if we ever need to update it, we can
# # use the terminfo database produced by the ncurses make script:
# RUN if [ "$EMSCRIPTEN_ENVIRONMENT" = "node" ]; then \
#         mkdir -p /root/output/terminfo/x && \
#         cp -v /root/lib/share/terminfo/x/xterm* /root/output/terminfo/x/; \
#     fi

# Strip the dead JS aliases Emscripten emits for the externally-linkable libphp
# ABI (see strip-unused-abi-export-aliases.mjs). The wasm exports stay; only the
# unused `Module['_sym']` aliases are removed. No-op when the strip list is empty
# (asyncify builds, or builds without the external-extension ABI).
#
# A stronger, Emscripten-native alternative — `-sDECLARE_ASM_MODULE_EXPORTS=0` —
# would avoid emitting these aliases entirely, but it is global and would break
# `ccall` (getCFunc reads `Module['_x']`), our custom js-library glue, and the
# `PHPLoader['malloc']`/`['free']` patches. That trade-off is documented in
# strip-unused-abi-export-aliases.mjs; this targeted strip is the lower-risk path.
COPY ./compile/php/strip-unused-abi-export-aliases.mjs /root/strip-unused-abi-export-aliases.mjs
RUN set -eux; \
	source /root/emsdk/emsdk_env.sh; \
	node /root/strip-unused-abi-export-aliases.mjs \
		/build/output/php.js \
		/root/.PHP_EXTENSION_ABI_JS_STRIP

# Postprocess the build php.js module:
COPY ./compile/php/esm-node-boilerplate.js /root/esm-node-boilerplate.js
COPY ./compile/php/esm-prefix.js /root/esm-prefix.js
COPY ./compile/php/esm-suffix.js /root/esm-suffix.js
RUN set -euxo pipefail; \
	cp -rfv /build/output/* /root/output/; \
	# Figure out the target file names and URLs
		# The .js and .wasm filenames should reflect the build configuration, e.g.:
		# * `php-7.4.node.js` and `php-7.4.node.wasm`
		# * `php-8.0.js` and `php-8.0.wasm`
		# In addition, the `.wasm` file URL should have a "cache busting" query string on the
		# web, so that web browsers will reload it when the file contents change.
	# Precompute export variables:
		export FILE_SIZE=$(stat -c%s "/root/output/php.wasm") && \
		export PHP_VERSION_ESCAPED="${PHP_VERSION//./_}" && \
		export FILENAME_PREFIX="php_${PHP_VERSION_ESCAPED:0:3}"; \
		export EXT_PREFIX=""; \
		export JS_FILENAME="${FILENAME_PREFIX}${EXT_PREFIX}.js"; \
		export WASM_FILENAME="${FILENAME_PREFIX}${EXT_PREFIX}.wasm"; \
	# Make the php.wasm URL configurable via the dependencyFilename loader argument:
		/root/replace.sh $'s/["\']php\.wasm[\'"]/dependencyFilename/g' /root/output/php.js; \
	# Make emscripten_asm_const_int resolvable by side modules (e.g. sodium.so,
	# see the ".JS_ABI_EXPORTS" comment above for the full "why"). Forcing it
	# into EXPORTED_FUNCTIONS only generates the JS function; it never reaches
	# the wasmImports object side modules resolve symbols against (it has no
	# real wasm export for mergeLibSymbols() to copy automatically), so it must
	# be seeded manually. Inserted right before Emscripten's own stable
	# "// include: postamble.js" marker, which always immediately follows the
	# generated `wasmImports = {...}` object literal.
		/root/replace.sh 's/^\/\/ include: postamble\.js/if (typeof _emscripten_asm_const_int === "function") { wasmImports["emscripten_asm_const_int"] = _emscripten_asm_const_int; }\n\/\/ include: postamble.js/' /root/output/php.js; \
	# Make getpid resolvable by side modules (e.g. posix.so's posix_getpid(),
	# CLAUDE.md decision 48) -- this main module is built with -Wl,--wrap=getpid
	# for EMSCRIPTEN_ENVIRONMENT=node, so plain "_getpid" doesn't exist as an
	# export at all, only the wrapped "___wrap_getpid" (already generated
	# regardless of .JS_ABI_EXPORTS, see the comment above it). A side module
	# built independently (never wrapped) imports the plain name "getpid",
	# so it needs the same manual wasmImports seeding as emscripten_asm_const_int
	# -- but NOT at the postamble.js marker like that one: unlike
	# emscripten_asm_const_int (a plain JS helper, available synchronously),
	# ___wrap_getpid is `wasmExports["__wrap_getpid"]`, a REAL wasm export only
	# assigned once the main module actually finishes instantiating -- at the
	# postamble.js insertion point (which runs before instantiation, since it's
	# what kicks it off) it is still undefined, so that insertion point silently
	# never sets the alias (found empirically: the build succeeded, but
	# posix_getpid() still returned the wrong value at runtime). Fixed by
	# inserting immediately after Emscripten's own real assignment line
	# instead, which is unconditionally generated (deterministic, extension-
	# set-independent) the moment ___wrap_getpid is in EXPORTED_FUNCTIONS.
		/root/replace.sh 's/(___wrap_getpid = Module\["___wrap_getpid"\] = wasmExports\["__wrap_getpid"\];)/$1\n  if (typeof ___wrap_getpid === "function") { wasmImports["getpid"] = ___wrap_getpid; }/' /root/output/php.js; \
	# Same for select (-Wl,--wrap=select, php_wasm.c's __wrap_select): net-snmp
		# in snmp.so waits for replies with select(), and must get the wrapped,
		# socket-aware version, not the plain libc one. __wrap_select is a JSPI
		# export (it can suspend); the dynamic linker already looks through the
		# JSPI wrapper (`.orig`) when resolving a side module's import.
		/root/replace.sh 's/(___wrap_select = Module\["___wrap_select"\] = wasmExports\["__wrap_select"\];)/$1\n  if (typeof ___wrap_select === "function") { wasmImports["select"] = ___wrap_select; }/' /root/output/php.js; \
	# Same for poll, usleep, popen and pclose (-Wl,--wrap too): side modules
		# waiting on sockets with poll() (libpq, mysqlnd, ...) must get the
		# version that yields to the event loop, and pgsql's libpq imports
		# the other three, which must be the JSPI-aware sleep and the
		# process-spawning popen, not musl's.
		/root/replace.sh 's/(___wrap_poll = Module\["___wrap_poll"\] = wasmExports\["__wrap_poll"\];)/$1\n  if (typeof ___wrap_poll === "function") { wasmImports["poll"] = ___wrap_poll; }/' /root/output/php.js; \
		/root/replace.sh 's/(___wrap_usleep = Module\["___wrap_usleep"\] = wasmExports\["__wrap_usleep"\];)/$1\n  if (typeof ___wrap_usleep === "function") { wasmImports["usleep"] = ___wrap_usleep; }/' /root/output/php.js; \
		/root/replace.sh 's/(___wrap_popen = Module\["___wrap_popen"\] = wasmExports\["__wrap_popen"\];)/$1\n  if (typeof ___wrap_popen === "function") { wasmImports["popen"] = ___wrap_popen; }/' /root/output/php.js; \
		/root/replace.sh 's/(___wrap_pclose = Module\["___wrap_pclose"\] = wasmExports\["__wrap_pclose"\];)/$1\n  if (typeof ___wrap_pclose === "function") { wasmImports["pclose"] = ___wrap_pclose; }/' /root/output/php.js; \
	# Patch a "property undefined" error
		# Emscripten produces an if that checks a stream.stream_ops.poll property. However,
		# stream.stream_ops is sometimes undefined and the check fails. Let's adjust it to
		# tolerate a null stream.stream_ops value.
		/root/replace.sh "s/if\s*\(stream\.stream_ops\.poll\)/if (stream.stream_ops?.poll)/g" /root/output/php.js; \
	# Emscripten allows only directories to be mounted, but in Playground we support mounting files, directories, and symlinks.
		# For file mounting to work, we need to remove the directory check.
		/root/replace-across-lines.sh 's/(\s*)if\s*\(\s*!FS\.isDir\(node\.mode\)\s*\)\s*\{\s*throw\s*new\s*FS\.ErrnoError\(54\)\s*;?\s*\}(\s*)/$1$2/s' /root/output/php.js; \
	# Make Emscripten websockets configurable
		# Emscripten makes the Websocket proxy connect to a fixed URL.
		# This assumes the traffic is always forwarded to the same target.
		# However, we want to support arbitrary targets, so we need to
		# replace the hardcoded websocket target URL with a dynamic callback.
		/root/replace.sh $'s/if\s*\(\s*SOCKFS\.websocketArgs\[[\'"]url[\'"]\]\s*\)/if("function"===typeof SOCKFS.websocketArgs["url"]) {\nurl = SOCKFS.websocketArgs["url"](...arguments);\n}else if ("string" === typeof SOCKFS.websocketArgs["url"])/g' \
			/root/output/php.js; \
		# Enable custom WebSocket constructors to support socket options.
		/root/replace.sh "s/ws\s*=\s*new WebSocketConstructor/if (Module['websocket']['decorator']) {WebSocketConstructor = Module['websocket']['decorator'](WebSocketConstructor);}ws = new WebSocketConstructor/g" /root/output/php.js && \
		if [ "$EMSCRIPTEN_ENVIRONMENT" = "node" ]; then \
			if [ "$WITH_WS_NETWORKING_PROXY" = "yes" ]; then \
				/root/replace.sh "s/sock\.server\s*=\s*new WebSocketServer/if (Module['websocket']['serverDecorator']) {WebSocketServer = Module['websocket']['serverDecorator'](WebSocketServer);}sock.server = new WebSocketServer/g" /root/output/php.js; \
			fi; \
		fi; \
		# PIPEFS: return 0 (success) instead of 6 (EWOULDBLOCK) when reached end of data
		# and the other end of the pipe is closed.
		/root/replace-across-lines.sh 's/if\s*\(\s*currentLength\s*==\s*0\s*\)\s*\{.{0,200}throw new FS.ErrnoError\(\s*6\s*\)/if(currentLength==0){if(pipe.refcnt<2){return 0;}throw new FS.ErrnoError(6)/sg' /root/output/php.js; \
	# Add MSG_PEEK flag support in recvfrom
		#
		# Emscripten ignores the flags argument to ___syscall_recvfrom.
		# However, PHP relies on passing the MSG_PEEK (== integer 2) flag when polling
		# the stream for data.
		# MSG_PEEK enables reading bytes without moving the stream pointer forward.
		# Without the patch below, PHP consumes the first byte from the top of the
		# response stream, typically "H" in "HTTP/1.1 200 OK", and then fails after
		# reading the remaining "TTP/1.1 200 OK" and not recognizing it as a valid
		# status line.
		# We need to patch the syscall to support the MSG_PEEK flag.
		if [ "$WITH_WS_NETWORKING_PROXY" = "yes" ]; then \
			/root/replace.sh 's/sock\.sock_ops\.recvmsg\(sock,\s*len\);/sock.sock_ops.recvmsg(sock, len, typeof flags !== "undefined" ? flags : 0);/g' /root/output/php.js; \
			/root/replace.sh 's/recvmsg\(sock,\s*length\)\s*{/recvmsg(sock, length, flags) {/g' /root/output/php.js; \
			/root/replace.sh 's/if\s*\(sock\.type\s*===\s*1\s*&&\s*bytesRead\s*<\s*queuedLength\)/if (flags&2) {bytesRead = 0;} if (sock.type === 1 && bytesRead < queuedLength)/g' /root/output/php.js; \
		fi ; \
	# Replace the hardcoded ENVIRONMENT variable with a dynamic computation
		#
		# The JavaScript code of the web loader and web worker loader is identical,
		# but Emscripten forces running different code paths by setting
		#
		# ENVIRONMENT_IS_WEB = true; ENVIRONMENT_IS_WORKER = false
		#
		# This project supports both environments and would have to maintain two
		# separate copies of the code. Instead, we use a dynamic computation of the
		# environment, based on the `RuntimeName` variable, which is an argument to the
		# wrapper function.
		/root/replace.sh $'s/ENVIRONMENT_IS_([A-Z]+)\s*=[^;]*;/ENVIRONMENT_IS_$1=RuntimeName==="$1";/g' /root/output/php.js; \
		/root/replace.sh 's/var ENV\s*=\s*\{/var ENV = PHPLoader.ENV || {/g' /root/output/php.js; \
	# Provide the JSPI longjmp tag for web side modules.
		# Existing web JSPI binaries do not export __c_longjmp, so the dynamic
		# linker would otherwise synthesize a JS function stub for a tag import.
		if [ "$EMSCRIPTEN_ENVIRONMENT" = "web" ] && [ "$WITH_JSPI" = "yes" ]; then \
			/root/replace.sh $'s/Asyncify\\.instrumentWasmImports\\(wasmImports\\);/Asyncify.instrumentWasmImports(wasmImports);\\n\\t\\twasmImports["__c_longjmp"] ??= new WebAssembly.Tag({\\n\\t\\t\\tparameters: ["i32"],\\n\\t\\t});/g' /root/output/php.js; \
		fi; \
	# Expose required functions to the module.
		# Emscripten 4.0.10 removed the runtime exposure of `_malloc` and `_free`,
		# so we rebind them from `wasmExports` and attach them to PHPLoader.
		/root/replace.sh "s/_wasm_free\s*=\s*Module\[[\"']_wasm_free[\"']\]/_wasm_free = PHPLoader['free'] = Module['_wasm_free']/g" /root/output/php.js; \
		/root/replace.sh "s/_malloc\s*=\s*Module\[[\"']_malloc[\"']\]/_malloc = PHPLoader['malloc'] = Module['_malloc']/g" /root/output/php.js; \
	# NOTE: upstream Playground adds a wasmImports alias here
	# (`wasm_recv: _wasm_recv`) so the memcached shared extension can resolve
	# that import. We don't vendor memcached (see NOTICE.md/CLAUDE.md — out
	# of scope), and networking in this project is handled Node-side by the
	# consuming package's own runtime (SOCKFS proxy), not by a WASM-side
	# wasm_recv. Removed: `_wasm_recv` is never actually generated by emcc
	# for our extension set, so the unconditional reference crashed every
	# Node build with "ReferenceError: _wasm_recv is not defined" the moment
	# php_8_5.js's init() ran. Re-add (correctly guarded) if memcached ever
	# gets vendored.
	# Patch Emscripten exit status to include a stack trace
		# Override Emscripten's default ExitStatus class which gets
		# thrown on failure. Unfortunately, the default object is not
		# a subclass of Error and does not provide any stack trace.
		#
		# This is a deliberate behavior on Emscripten's end to prevent
		# memory leaks after the program exits. See:
		#
		# https://github.com/emscripten-core/emscripten/pull/9108
		#
		# In case of WordPress Playground, the worker in which the PHP
		# runs will typically exit after the PHP program finishes, so
		# we don't have to worry about memory leaks.
		/root/replace-across-lines.sh "s/(class\\s+ExitStatus\\s*{\\s*name\\s*=\\s*['\"]ExitStatus['\"];\\s*constructor\\s*\(\\s*status\\s*\)\\s*{\\s*this\\.message\\s*=\\s*\`Program terminated with exit\(\\\${status}\)\`;\s*this\\.status\\s*=\\s*status;?\\s*}\\s*})/\$1\nExitStatus = class PHPExitStatus extends Error {\n	constructor\(status\) {\n		super\(status\);\n		this.name = 'ExitStatus';\n		this.message = 'Program terminated with exit\(' + status + '\)';\n		this.status = status;\n	}\n};/" /root/output/php.js; \
	# Restore quit handling to Emscripten
		# Quit callback customization was removed from Emscripten by:
		# https://github.com/emscripten-core/emscripten/pull/22371
		# But the alternative `onExit()` handling is conditional and has different semantics.
		# To avoid bugs due to this change, let's just restore support for quit callback customization.
		/root/replace.sh "s/(if\s*\(Module\[['\"]thisProgram['\"]\]\)\s*thisProgram\s*=\s*Module\[['\"]thisProgram['\"]\];)/\$1\nif (Module[\"quit\"]) quit_=Module[\"quit\"];/" /root/output/php.js; \
	# Cache Asyncify sleep data allocation to prevent memory growth corruption
		#
		# Emscripten's handleSleep() calls _malloc() on every async operation and
		# _free() on every rewind. If _malloc triggers memory.grow(), the ArrayBuffer
		# detaches and corrupts state. Fix: allocate once, reuse on subsequent sleeps.
		# See: https://github.com/emscripten-core/emscripten/issues/21108
		if [ "$WITH_JSPI" != "yes" ]; then \
			/root/replace.sh \
				's/Asyncify\.currData\s*=\s*Asyncify\.allocateData\(\)/if (!Asyncify._cachedData) { Asyncify._cachedData = Asyncify.allocateData(); } else { Asyncify.setDataHeader(Asyncify._cachedData, Asyncify._cachedData + 12, Asyncify.StackSize); Asyncify.setDataRewindFunc(Asyncify._cachedData); } Asyncify.currData = Asyncify._cachedData/' \
				/root/output/php.js; \
			/root/replace-across-lines.sh \
				's/runAndAbortIfError\(_asyncify_stop_rewind\);\s*_free\(Asyncify\.currData\);/runAndAbortIfError(_asyncify_stop_rewind);/s' \
				/root/output/php.js; \
		fi; \
	# Node.js: Create a require() function and add 'currentDirPath' declaration
	# because Emscripten generates Node.js code that requires them.
		if [ "$EMSCRIPTEN_ENVIRONMENT" = "node" ]; then \
			cat /root/esm-node-boilerplate.js >> /root/output/php-module.js; \
		fi; \
	# Turn the php.js file into an ES module
		# Manually turn the output into a esm module instead of relying on -s MODULARIZE=1.
		# which pollutes the global namespace and does not play well with import() mechanics.
		if [ "$EMSCRIPTEN_ENVIRONMENT" = "node" ]; then \
			echo "const dependencyFilename = path.join(currentDirPath, '${PHP_VERSION_ESCAPED}', '$WASM_FILENAME');" >> /root/output/php-module.js;\
			/root/replace.sh "s/scriptDirectory\s*=\s*__dirname/scriptDirectory = currentDirPath/" /root/output/php.js; \
		else \
			echo "import dependencyFilename from './${PHP_VERSION_ESCAPED}/$WASM_FILENAME'; " >> /root/output/php-module.js; \
		fi; \
		echo "export { dependencyFilename }; " >> /root/output/php-module.js && \
		echo "export const dependenciesTotalSize = $FILE_SIZE; " >> /root/output/php-module.js && \
		echo "const phpVersionString = '$PHP_VERSION';" >> /root/output/php-module.js && \
		cat /root/esm-prefix.js >> /root/output/php-module.js && \
		cat /root/output/php.js >> /root/output/php-module.js && \
		cat /root/esm-suffix.js >> /root/output/php-module.js && \
		\
		# Remove the old php.js file
		rm /root/output/php.js && \
		\
		# Rename the build files to their final names
		mv /root/output/php-module.js "/root/output/$JS_FILENAME"; \
		mkdir -p /root/output/${PHP_VERSION_ESCAPED}/; \
		mv /root/output/php.wasm "/root/output/${PHP_VERSION_ESCAPED}/$WASM_FILENAME"; \
		if [ "${WITH_SOURCEMAPS}" = "yes" ]; then \
			# Make the source map paths relative to the .wasm file, not the build directory
			sed -i 's#"\.\./\.\./root/#"#g' /root/output/php.wasm.map; \
			mv /root/output/php.wasm.map "/root/output/${PHP_VERSION_ESCAPED}/php.wasm.map"; \
		fi; \
		if [ "${WITH_SOURCEMAPS}" = "yes" ] || [ "${WITH_DEBUG}" = "yes" ]; then \
			# Make PHP source available for use with step debugger
			rm -rf /root/php-src/.git; \
			cp -r /root/php-src /root/output/"${PHP_VERSION_ESCAPED}"/php-src; \
			# Copy emscripten source files
			mkdir -p /root/output/"${PHP_VERSION_ESCAPED}"/emscripten; \
			cp -r /root/emsdk/upstream/emscripten/cache /root/output/"${PHP_VERSION_ESCAPED}"/emscripten/; \
			cp -r /root/emsdk/upstream/emscripten/system /root/output/"${PHP_VERSION_ESCAPED}"/emscripten/; \
		fi;
