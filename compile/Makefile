MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(dir $(MAKEFILE_PATH))

# Remove trailing slash if present
MAKEFILE_DIR := $(patsubst %/,%,$(MAKEFILE_DIR))

# Some upstream builds (zlib, OpenSSL) install both static (.a) and shared
# (.so*) artifacts. wasm-ld refuses to statically link against the latter
# ("attempted static link of dynamic object"), which breaks any consumer
# that finds a dependency via a generic -L/-l search (autoconf's
# AC_CHECK_LIB-style tests, e.g. libssh2's) rather than referencing the .a
# file by exact path. Call this right after populating a lib's dist dir.
STRIP_SO = rm -f $(1)/lib/*.so $(1)/lib/*.so.*

# Reads a library's pinned version straight out of matrix.json (its "latest"
# entry — see matrix.json's header comment and CLAUDE.md decision 27) instead
# of duplicating it as a separately hand-synced Makefile variable. Usage:
# SOME_VERSION := $(call MATRIX_VERSION,someLibraryKey)
MATRIX_VERSION = $(shell node $(MAKEFILE_DIR)/matrix-version.mjs $(1))

# JSPI only — no Asyncify (see CLAUDE.md decision 6). This project targets
# recent Node.js only, so Asyncify's broader-but-slower browser-compat
# tradeoff buys us nothing.

# The recipe used to never create this marker file, which made GNU Make
# treat "base-image" as freshly rebuilt on every single `make` invocation —
# and since every lib target depends on it, that cascaded into rebuilding
# every lib every time, defeating any attempt at a targeted/incremental
# build (see CLAUDE.md decision 25). `touch` it once the image is built;
# `rm -f` it (or `make clean-base-image`) to force a real rebuild later,
# e.g. after bumping the Ubuntu/Emscripten base.
base-image: base-image/.ready
base-image/.ready: base-image/Dockerfile
	cd base-image; docker build -f ./Dockerfile -t kirigami-php-wasm:base .
	touch base-image/.ready

clean-base-image:
	rm -f base-image/.ready

LIBZ_VERSION := $(call MATRIX_VERSION,libz)

libz_jspi: libz/jspi/dist/root/lib/lib/libz.a
libz/jspi/dist/root/lib/lib/libz.a: base-image
	docker build -f ./libz/Dockerfile -t kirigami-php-wasm:libz ./ --build-arg JSPI=1 --build-arg ZLIB_VERSION=$(LIBZ_VERSION)
	mkdir -p ./libz/jspi/dist/root/lib
	CID=$$(docker create kirigami-php-wasm:libz) && docker cp $$CID:/root/lib/lib ./libz/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libz) && docker cp $$CID:/root/lib/include ./libz/jspi/dist/root/lib && docker rm $$CID
	$(call STRIP_SO,./libz/jspi/dist/root/lib)

libzip_jspi: libzip-1.2.0_jspi libzip-1.9.2_jspi

libzip-1.2.0_jspi: libzip/jspi/dist/1.2.0/root/lib/lib/libzip.a
libzip/jspi/dist/1.2.0/root/lib/lib/libzip.a: base-image libz_jspi
	mkdir -p ./libzip/jspi/dist/1.2.0/root/lib
	docker build -f ./libzip/Dockerfile -t kirigami-php-wasm:libzip . --build-arg LIBZIP_VERSION=1.2.0 --build-arg JSPI=1 --progress=plain
	CID=$$(docker create kirigami-php-wasm:libzip) && docker cp $$CID:/root/install/lib ./libzip/jspi/dist/1.2.0/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libzip) && docker cp $$CID:/root/install/include ./libzip/jspi/dist/1.2.0/root/lib && docker rm $$CID

libzip-1.9.2_jspi: libzip/jspi/dist/1.9.2/root/lib/lib/libzip.a
libzip/jspi/dist/1.9.2/root/lib/lib/libzip.a: base-image libz_jspi
	mkdir -p ./libzip/jspi/dist/1.9.2/root/lib
	docker build -f ./libzip/Dockerfile -t kirigami-php-wasm:libzip . --build-arg LIBZIP_VERSION=1.9.2 --build-arg JSPI=1 --progress=plain
	CID=$$(docker create kirigami-php-wasm:libzip) && docker cp $$CID:/root/install/lib ./libzip/jspi/dist/1.9.2/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libzip) && docker cp $$CID:/root/install/include ./libzip/jspi/dist/1.9.2/root/lib && docker rm $$CID

LIBPNG_VERSION := $(call MATRIX_VERSION,libpng16)

libpng16_jspi: libpng16/jspi/dist/root/lib/lib/libpng16.a
libpng16/jspi/dist/root/lib/lib/libpng16.a: base-image libz_jspi
	mkdir -p ./libpng16/jspi/dist/root/lib
	docker build -f ./libpng16/Dockerfile -t kirigami-php-wasm:libpng . --build-arg JSPI=1 --build-arg LIBPNG_VERSION=$(LIBPNG_VERSION)
	CID=$$(docker create kirigami-php-wasm:libpng) && docker cp $$CID:/root/install/lib ./libpng16/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libpng) && docker cp $$CID:/root/install/include ./libpng16/jspi/dist/root/lib && docker rm $$CID

LIBJPEG_VERSION := $(call MATRIX_VERSION,libjpeg)

libjpeg_jspi: libjpeg/jspi/dist/root/lib/lib/libjpeg.a
libjpeg/jspi/dist/root/lib/lib/libjpeg.a: base-image libz_jspi
	mkdir -p ./libjpeg/jspi/dist/root/lib
	docker build -f ./libjpeg/Dockerfile -t kirigami-php-wasm:libpng . --build-arg JSPI=1 --build-arg LIBJPEG_VERSION=$(LIBJPEG_VERSION)
	CID=$$(docker create kirigami-php-wasm:libpng) && docker cp $$CID:/root/lib/lib32 ./libjpeg/jspi/dist/root/lib && docker rm $$CID
	# The directory might exist if another build step failed and we're retrying now
	mv ./libjpeg/jspi/dist/root/lib/lib32 ./libjpeg/jspi/dist/root/lib/lib || true
	CID=$$(docker create kirigami-php-wasm:libpng) && docker cp $$CID:/root/lib/include ./libjpeg/jspi/dist/root/lib && docker rm $$CID

LIBWEBP_VERSION := $(call MATRIX_VERSION,libwebp)

libwebp_jspi: libwebp/jspi/dist/root/lib/lib/libwebp.a
libwebp/jspi/dist/root/lib/lib/libwebp.a: base-image libz_jspi
	mkdir -p ./libwebp/jspi/dist/root/lib
	docker build -f ./libwebp/Dockerfile -t kirigami-php-wasm:libpng . --build-arg JSPI=1 --build-arg LIBWEBP_VERSION=$(LIBWEBP_VERSION)
	CID=$$(docker create kirigami-php-wasm:libpng) && docker cp $$CID:/root/lib/lib ./libwebp/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libpng) && docker cp $$CID:/root/lib/include ./libwebp/jspi/dist/root/lib && docker rm $$CID

LIBAOM_VERSION := $(call MATRIX_VERSION,libaom)

libaom_jspi: libaom/jspi/dist/root/lib/lib/libaom.a
libaom/jspi/dist/root/lib/lib/libaom.a: base-image
	mkdir -p ./libaom/jspi/dist/root/lib
	docker build -f ./libaom/Dockerfile -t kirigami-php-wasm:libaom . --build-arg JSPI=1 --build-arg LIBAOM_VERSION=$(LIBAOM_VERSION)
	CID=$$(docker create kirigami-php-wasm:libaom) && docker cp $$CID:/root/lib/lib ./libaom/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libaom) && docker cp $$CID:/root/lib/include ./libaom/jspi/dist/root/lib && docker rm $$CID

LIBAVIF_VERSION := $(call MATRIX_VERSION,libavif)

libavif_jspi: libavif/jspi/dist/root/lib/lib/libavif.a
libavif/jspi/dist/root/lib/lib/libavif.a: base-image
	mkdir -p ./libavif/jspi/dist/root/lib
	docker build -f ./libavif/Dockerfile -t kirigami-php-wasm:libavif . --build-arg JSPI=1 --build-arg LIBAVIF_VERSION=$(LIBAVIF_VERSION)
	CID=$$(docker create kirigami-php-wasm:libavif) && docker cp $$CID:/root/lib/lib ./libavif/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libavif) && docker cp $$CID:/root/lib/include ./libavif/jspi/dist/root/lib && docker rm $$CID

FREETYPE_VERSION := $(call MATRIX_VERSION,freetype)

libfreetype_jspi: libfreetype/jspi/dist/root/lib/lib/libfreetype.a
libfreetype/jspi/dist/root/lib/lib/libfreetype.a: base-image libz_jspi libpng16_jspi
	mkdir -p ./libfreetype/jspi/dist/root/lib
	docker build -f ./libfreetype/Dockerfile -t kirigami-php-wasm:libfreetype . --build-arg JSPI=1 --build-arg FREETYPE_VERSION=$(FREETYPE_VERSION) --progress=plain
	CID=$$(docker create kirigami-php-wasm:libfreetype) && docker cp $$CID:/root/install/lib ./libfreetype/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libfreetype) && docker cp $$CID:/root/install/include ./libfreetype/jspi/dist/root/lib && docker rm $$CID

GD_VERSION := $(call MATRIX_VERSION,libgd)

libgd_jspi: libgd/jspi/dist/root/lib/lib/libgd.a
libgd/jspi/dist/root/lib/lib/libgd.a: base-image libz_jspi libpng16_jspi libjpeg_jspi libwebp_jspi libavif_jspi libfreetype_jspi libiconv_jspi
	mkdir -p ./libgd/jspi/dist/root/lib
	docker build -f ./libgd/Dockerfile -t kirigami-php-wasm:libgd . --build-arg JSPI=1 --build-arg GD_VERSION=$(GD_VERSION)
	CID=$$(docker create kirigami-php-wasm:libgd) && docker cp $$CID:/root/install/lib ./libgd/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libgd) && docker cp $$CID:/root/install/include ./libgd/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libgd) && docker cp $$CID:/root/install/bin ./libgd/jspi/dist/root/lib && docker rm $$CID

LIBXML2_VERSION := $(call MATRIX_VERSION,libxml2)

libxml2_jspi: libxml2/jspi/dist/root/lib/lib/libxml2.a
libxml2/jspi/dist/root/lib/lib/libxml2.a: base-image libz_jspi
	mkdir -p ./libxml2/jspi/dist/root/lib
	docker build -f ./libxml2/Dockerfile -t kirigami-php-wasm:libxml2 . --build-arg JSPI=1 --build-arg LIBXML2_VERSION=$(LIBXML2_VERSION)
	CID=$$(docker create kirigami-php-wasm:libxml2) && docker cp $$CID:/root/lib/bin ./libxml2/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libxml2) && docker cp $$CID:/root/lib/lib ./libxml2/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libxml2) && docker cp $$CID:/root/lib/include ./libxml2/jspi/dist/root/lib && docker rm $$CID

# Single OpenSSL version, not the two (1.1.0h/1.1.1t) upstream Playground
# built in parallel — we only target one PHP version (8.5, see CLAUDE.md
# decision 16), so there is no need for two. Version comes from matrix.json
# (decision 27), not hand-synced here — see matrix.json's "libopenssl" note
# for what's been verified to actually cross-compile.
OPENSSL_VERSION := $(call MATRIX_VERSION,libopenssl)

libopenssl_jspi: libopenssl/jspi/dist/$(OPENSSL_VERSION)/root/lib/lib/libssl.a

libopenssl/jspi/dist/$(OPENSSL_VERSION)/root/lib/lib/libssl.a: base-image libz_jspi
	mkdir -p ./libopenssl/jspi/dist/root/lib
	docker build -f ./libopenssl/Dockerfile -t kirigami-php-wasm:libopenssl . --build-arg JSPI=1 --build-arg OPENSSL_VERSION=$(OPENSSL_VERSION)
	mkdir -p ./libopenssl/jspi/dist/$(OPENSSL_VERSION)/root/lib
	CID=$$(docker create kirigami-php-wasm:libopenssl) && docker cp $$CID:/root/install/lib ./libopenssl/jspi/dist/$(OPENSSL_VERSION)/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libopenssl) && docker cp $$CID:/root/install/include ./libopenssl/jspi/dist/$(OPENSSL_VERSION)/root/lib && docker rm $$CID
	# libcurl/Dockerfile, libssh2/Dockerfile, and compile/php/Dockerfile all
	# read a version-agnostic "dist/root/lib" for openssl (no OPENSSL_VERSION
	# selection on their end), but nothing else here ever populates it —
	# mirror our one built version into it.
	cp -r ./libopenssl/jspi/dist/$(OPENSSL_VERSION)/root/lib/. ./libopenssl/jspi/dist/root/lib/ || true
	$(call STRIP_SO,./libopenssl/jspi/dist/root/lib)

WITH_DEBUG ?= no

SQLITE_VERSION := $(call MATRIX_VERSION,libsqlite3)
# sqlite.org's own tarball naming encodes the dotted version as
# MAJOR*1000000 + MINOR*10000 + PATCH*100 + BUILD (e.g. 3.51.0 -> 3510000) —
# a Dockerfile ARG can't do this arithmetic itself, so it's precomputed here.
SQLITE_VERSION_COMPACT := $(shell node -e "const [maj,min,pat]=process.argv[1].split('.').map(Number); process.stdout.write(String(maj*1000000+(min||0)*10000+(pat||0)*100))" $(SQLITE_VERSION))

libsqlite3_jspi: libsqlite3/jspi/dist/root/lib/lib/libsqlite3.a
libsqlite3/jspi/dist/root/lib/lib/libsqlite3.a: base-image libz_jspi
	mkdir -p ./libsqlite3/jspi/dist/root/lib
	docker build -f ./libsqlite3/Dockerfile -t kirigami-php-wasm:libsqlite3 . \
		--build-arg JSPI=1 \
		--build-arg WITH_DEBUG=${WITH_DEBUG} \
		--build-arg DEBUG_DWARF_COMPILATION_DIR=${MAKEFILE_DIR}/libsqlite3 \
		--build-arg SQLITE_VERSION_COMPACT=$(SQLITE_VERSION_COMPACT) \
		--progress=plain
	CID=$$(docker create kirigami-php-wasm:libsqlite3) && docker cp $$CID:/root/lib/lib ./libsqlite3/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libsqlite3) && docker cp $$CID:/root/lib/include ./libsqlite3/jspi/dist/root/lib && docker rm $$CID
	@if [ "${WITH_DEBUG}" = "yes" ]; then \
		CID=$$(docker create kirigami-php-wasm:libsqlite3) && docker cp $$CID:/root/sqlite-src ./libsqlite3/ && docker rm $$CID; \
	fi

LIBICONV_VERSION := $(call MATRIX_VERSION,libiconv)

libiconv_jspi: libiconv/jspi/dist/root/lib/lib/libiconv.a
libiconv/jspi/dist/root/lib/lib/libiconv.a: base-image libz_jspi
	mkdir -p ./libiconv/jspi/dist/root/lib
	docker build -f ./libiconv/Dockerfile -t kirigami-php-wasm:libiconv . --build-arg JSPI=1 --build-arg LIBICONV_VERSION=$(LIBICONV_VERSION)
	CID=$$(docker create kirigami-php-wasm:libiconv) && docker cp $$CID:/root/install/lib ./libiconv/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libiconv) && docker cp $$CID:/root/install/include ./libiconv/jspi/dist/root/lib && docker rm $$CID

oniguruma_jspi: oniguruma/jspi/dist/root/lib/lib/libonig.a
oniguruma/jspi/dist/root/lib/lib/libonig.a: base-image
	mkdir -p ./oniguruma/jspi/dist/root/lib
	docker build -f ./oniguruma/Dockerfile -t kirigami-php-wasm:oniguruma . --progress=plain --build-arg JSPI=1
	CID=$$(docker create kirigami-php-wasm:oniguruma) && docker cp $$CID:/root/lib/lib ./oniguruma/jspi/dist/root/lib/ && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:oniguruma) && docker cp $$CID:/root/lib/include ./oniguruma/jspi/dist/root/lib/ && docker rm $$CID

LIBSSH2_VERSION := $(call MATRIX_VERSION,libssh2)

libssh2_jspi: libssh2/jspi/dist/root/lib/lib/libssh2.a
libssh2/jspi/dist/root/lib/lib/libssh2.a: base-image libz_jspi libopenssl_jspi
	mkdir -p ./libssh2/jspi/dist/root/lib
	docker build -f ./libssh2/Dockerfile -t kirigami-php-wasm:libssh2 . --build-arg JSPI=1 --build-arg LIBSSH2_VERSION=$(LIBSSH2_VERSION) --progress=plain
	CID=$$(docker create kirigami-php-wasm:libssh2) && docker cp $$CID:/root/install/lib ./libssh2/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libssh2) && docker cp $$CID:/root/install/include ./libssh2/jspi/dist/root/lib && docker rm $$CID
	$(call STRIP_SO,./libssh2/jspi/dist/root/lib)

NGHTTP2_VERSION := $(call MATRIX_VERSION,nghttp2)

nghttp2_jspi: nghttp2/jspi/dist/root/lib/lib/libnghttp2.a
nghttp2/jspi/dist/root/lib/lib/libnghttp2.a: base-image
	mkdir -p ./nghttp2/jspi/dist/root/lib
	docker build -f ./nghttp2/Dockerfile -t kirigami-php-wasm:nghttp2 . --build-arg JSPI=1 --build-arg NGHTTP2_VERSION=$(NGHTTP2_VERSION) --progress=plain
	CID=$$(docker create kirigami-php-wasm:nghttp2) && docker cp $$CID:/root/install/lib ./nghttp2/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:nghttp2) && docker cp $$CID:/root/install/include ./nghttp2/jspi/dist/root/lib && docker rm $$CID
	$(call STRIP_SO,./nghttp2/jspi/dist/root/lib)

# Version comes from matrix.json (decision 28), not hand-synced here.
LIBSODIUM_VERSION := $(call MATRIX_VERSION,libsodium)

LIBYAML_VERSION := $(call MATRIX_VERSION,libyaml)

libyaml_jspi: libyaml/jspi/dist/root/lib/lib/libyaml.a
libyaml/jspi/dist/root/lib/lib/libyaml.a: base-image
	mkdir -p ./libyaml/jspi/dist/root/lib
	docker build -f ./libyaml/Dockerfile -t kirigami-php-wasm:libyaml . --build-arg JSPI=1 --build-arg LIBYAML_VERSION=$(LIBYAML_VERSION) --progress=plain
	CID=$$(docker create kirigami-php-wasm:libyaml) && docker cp $$CID:/root/install/lib ./libyaml/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libyaml) && docker cp $$CID:/root/install/include ./libyaml/jspi/dist/root/lib && docker rm $$CID
	$(call STRIP_SO,./libyaml/jspi/dist/root/lib)

CMARK_VERSION := $(call MATRIX_VERSION,cmark)

libcmark_jspi: libcmark/jspi/dist/root/lib/lib/libcmark.a
libcmark/jspi/dist/root/lib/lib/libcmark.a: base-image
	mkdir -p ./libcmark/jspi/dist/root/lib
	docker build -f ./libcmark/Dockerfile -t kirigami-php-wasm:libcmark . --build-arg JSPI=1 --build-arg CMARK_VERSION=$(CMARK_VERSION) --progress=plain
	CID=$$(docker create kirigami-php-wasm:libcmark) && docker cp $$CID:/root/install/lib ./libcmark/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libcmark) && docker cp $$CID:/root/install/include ./libcmark/jspi/dist/root/lib && docker rm $$CID
	$(call STRIP_SO,./libcmark/jspi/dist/root/lib)

libsodium_jspi: libsodium/jspi/dist/root/lib/lib/libsodium.a
libsodium/jspi/dist/root/lib/lib/libsodium.a: base-image
	mkdir -p ./libsodium/jspi/dist/root/lib
	docker build -f ./libsodium/Dockerfile -t kirigami-php-wasm:libsodium . --build-arg JSPI=1 --build-arg LIBSODIUM_VERSION=$(LIBSODIUM_VERSION) --progress=plain
	CID=$$(docker create kirigami-php-wasm:libsodium) && docker cp $$CID:/root/install/lib ./libsodium/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libsodium) && docker cp $$CID:/root/install/include ./libsodium/jspi/dist/root/lib && docker rm $$CID
	$(call STRIP_SO,./libsodium/jspi/dist/root/lib)

# Version comes from matrix.json (decision 27), not hand-synced here — see
# matrix.json's "libcurl" note for what's been verified to actually
# cross-compile before bumping matrix.json's last "versions" entry.
CURL_VERSION := $(call MATRIX_VERSION,libcurl)

libcurl_jspi: libcurl/jspi/dist/root/lib/lib/libcurl.a
libcurl/jspi/dist/root/lib/lib/libcurl.a: base-image libz_jspi libopenssl_jspi libssh2_jspi nghttp2_jspi
	mkdir -p ./libcurl/jspi/dist/root/lib
	docker build -f ./libcurl/Dockerfile -t kirigami-php-wasm:libcurl . --progress=plain --build-arg JSPI=1 --build-arg CURL_VERSION=curl-$(CURL_VERSION)
	CID=$$(docker create kirigami-php-wasm:libcurl) && docker cp $$CID:/root/curl-$(CURL_VERSION)/lib/.libs ./libcurl/jspi/dist/root/lib/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libcurl) && docker cp $$CID:/root/curl-$(CURL_VERSION)/include/ ./libcurl/jspi/dist/root/lib && docker rm $$CID

IMAGEMAGICK_VERSION := $(call MATRIX_VERSION,libImageMagick)

libImageMagick_jspi: libImageMagick/jspi/dist/root/lib/lib/libMagickCore-7.Q16HDRI.a
libImageMagick/jspi/dist/root/lib/lib/libMagickCore-7.Q16HDRI.a: base-image libz_jspi libjpeg_jspi libpng16_jspi libwebp_jspi
	mkdir -p ./libImageMagick/jspi/dist/root/lib
	docker build -f ./libImageMagick/Dockerfile -t kirigami-php-wasm:libImageMagick . --progress=plain --build-arg JSPI=1 --build-arg IMAGEMAGICK_VERSION=$(IMAGEMAGICK_VERSION)
	CID=$$(docker create kirigami-php-wasm:libImageMagick) && docker cp $$CID:/root/install/lib ./libImageMagick/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libImageMagick) && docker cp $$CID:/root/install/include ./libImageMagick/jspi/dist/root/lib && docker rm $$CID
	CID=$$(docker create kirigami-php-wasm:libImageMagick) && docker cp $$CID:/root/install/bin ./libImageMagick/jspi/dist/root/lib && docker rm $$CID

all: all_jspi
all_jspi: libz_jspi libzip_jspi libpng16_jspi libjpeg_jspi libwebp_jspi libaom_jspi libavif_jspi libfreetype_jspi libgd_jspi libxml2_jspi libopenssl_jspi libsqlite3_jspi libiconv_jspi oniguruma_jspi libssh2_jspi nghttp2_jspi libcurl_jspi libImageMagick_jspi libsodium_jspi libyaml_jspi libcmark_jspi

clean:
	rm -rf ./libz/jspi/dist
	rm -rf ./libzip/jspi/dist
	rm -rf ./libpng16/jspi/dist
	rm -rf ./libjpeg/jspi/dist
	rm -rf ./libwebp/jspi/dist
	rm -rf ./libaom/jspi/dist
	rm -rf ./libavif/jspi/dist
	rm -rf ./libfreetype/jspi/dist
	rm -rf ./libgd/jspi/dist
	rm -rf ./libxml2/jspi/dist
	rm -rf ./libopenssl/jspi/dist
	rm -rf ./libsqlite3/jspi/dist
	rm -rf ./libiconv/jspi/dist
	rm -rf ./oniguruma/jspi/dist
	rm -rf ./libssh2/jspi/dist
	rm -rf ./nghttp2/jspi/dist
	rm -rf ./libcurl/jspi/dist
	rm -rf ./libImageMagick/jspi/dist
	rm -rf ./libsodium/jspi/dist
	rm -rf ./libyaml/jspi/dist
	rm -rf ./libcmark/jspi/dist

# Targeted cleanup of a single lib (e.g. `make clean-libzip && make libzip_jspi`),
# to force a rebuild without starting over from scratch like `clean` does.
clean-libz:
	rm -rf ./libz/jspi/dist
clean-libzip:
	rm -rf ./libzip/jspi/dist
clean-libpng16:
	rm -rf ./libpng16/jspi/dist
clean-libjpeg:
	rm -rf ./libjpeg/jspi/dist
clean-libwebp:
	rm -rf ./libwebp/jspi/dist
clean-libaom:
	rm -rf ./libaom/jspi/dist
clean-libavif:
	rm -rf ./libavif/jspi/dist
clean-libfreetype:
	rm -rf ./libfreetype/jspi/dist
clean-libgd:
	rm -rf ./libgd/jspi/dist
clean-libxml2:
	rm -rf ./libxml2/jspi/dist
clean-libopenssl:
	rm -rf ./libopenssl/jspi/dist
clean-libsqlite3:
	rm -rf ./libsqlite3/jspi/dist
clean-libiconv:
	rm -rf ./libiconv/jspi/dist
clean-oniguruma:
	rm -rf ./oniguruma/jspi/dist
clean-libssh2:
	rm -rf ./libssh2/jspi/dist
clean-nghttp2:
	rm -rf ./nghttp2/jspi/dist
clean-libcurl:
	rm -rf ./libcurl/jspi/dist
clean-libImageMagick:
	rm -rf ./libImageMagick/jspi/dist
clean-libsodium:
	rm -rf ./libsodium/jspi/dist
clean-libyaml:
	rm -rf ./libyaml/jspi/dist
clean-libcmark:
	rm -rf ./libcmark/jspi/dist
