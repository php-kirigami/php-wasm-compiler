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
		# Be sure to disable OPcache if not needed.
		echo -n ' --disable-opcache ' >> /root/.php-configure-flags; \
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

# Get and patch PHP
COPY ./compile/php/php*.patch /root/
COPY ./compile/php/apply-mysqlnd-patch.sh /root/
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
	touch php-src/patched

# Install Mysql support if needed
RUN if [ "$WITH_MYSQL" = "yes" ]; then \
		echo -n ' --enable-mysql --enable-pdo --with-mysql=mysqlnd --with-mysqli=mysqlnd --with-pdo-mysql=mysqlnd ' >> /root/.php-configure-flags; \
		echo -n ' -DMYSQLND_FORCE_TCP_LOCALHOST ' >> /root/.emcc-php-wasm-flags; \
	fi

# Download and prepare imagick extension if needed (PHP 7.4+)
# Use version 3.7.0 for PHP 7.x, development branch for PHP 8.x
RUN if [ "$WITH_IMAGICK" = "yes" ]; then \
		if [ "${PHP_VERSION:0:1}" -eq "7" ]; then \
			IMAGICK_BRANCH="3.7.0"; \
		else \
			IMAGICK_BRANCH="master"; \
		fi && \
		git clone https://github.com/Imagick/imagick.git /root/imagick \
			--branch "$IMAGICK_BRANCH" \
			--single-branch \
			--depth 1 && \
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
	export ASYNCIFY_FLAGS=" -s ASYNCIFY=2 -sSUPPORT_LONGJMP=wasm -fwasm-exceptions -sJSPI_IMPORTS=js_open_process,js_fd_read,js_waitpid,js_process_status,js_create_input_device,wasm_setsockopt,wasm_shutdown,wasm_close,wasm_recv,wasm_connect,__syscall_fcntl64,js_flock,js_release_file_locks,js_waitpid -sJSPI_EXPORTS=php_wasm_init,wasm_sleep,wasm_read,emscripten_sleep,wasm_sapi_handle_request,wasm_sapi_request_shutdown,wasm_poll_socket,wrap_select,__wrap_select,select,php_pollfd_for,fflush,wasm_popen,wasm_pclose,__wrap_popen,__wrap_pclose,wasm_read,wasm_php_exec,run_cli,wasm_recv,wasm_connect,__wasm_call_ctors,__errno_location,__funcs_on_exit -s EXPORTED_RUNTIME_METHODS=HEAPU32,HEAPU8,ccall,PROXYFS,wasmExports "; \
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
	-Wl,--export=__stack_pointer \
	-Wl,--wrap=usleep \
	-Wl,--wrap=select \
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
