dnl Build configuration for the scanmeqr extension.
dnl
dnl   phpize && ./configure && make
dnl
dnl The C++ core is compiled straight into the extension instead of being linked
dnl as a shared library. PIE only ever runs phpize/configure/make inside the
dnl package directory, so there is no opportunity to run CMake first, and an
dnl extension that needs a prebuilt libscanme_qr could not be installed with it.
dnl
dnl The same file drives two layouts. In the generated crazy-goat/qrcode-ext
dnl package the core sources sit flat next to this file; inside ScanMePHP they
dnl live in ../clib and are symlinked in here at configure time, which keeps
dnl clib/ the single place they are edited while still putting every object file
dnl the build produces inside php-ext/.

dnl Default yes, which is the convention for an extension living outside
dnl php-src: PIE runs ./configure with no arguments beyond the declared options,
dnl so a default of `no` would make `pie install` build nothing and still exit 0.
PHP_ARG_ENABLE([scanmeqr],
  [whether to enable scanmeqr support],
  [AS_HELP_STRING([--enable-scanmeqr],
    [Enable scanmeqr support (default: yes)])],
  [yes])

PHP_ARG_WITH([scanmeqr-clib],
  [where to find the ScanMePHP C++ core],
  [AS_HELP_STRING([--with-scanmeqr-clib=DIR],
    [Directory holding the C++ core as src/ and include/. Only meaningful inside
     a ScanMePHP checkout, where it defaults to ../clib; the standalone package
     carries the sources and ignores this.])],
  [yes],
  [no])

if test "$PHP_SCANMEQR" != "no"; then

  PHP_REQUIRE_CXX()

  dnl Tested directly rather than through PHP_CXX_COMPILE_STDCXX, whose accepted
  dnl arguments depend on the PHP being built against: 8.1's copy knows nothing
  dnl about 20 and fails at autoconf time with "invalid first argument `20'".
  dnl The core needs C++20 (std::span, <bit>), so there is no lower fallback.
  AC_MSG_CHECKING([whether $CXX accepts -std=c++20])
  AC_LANG_PUSH([C++])
  scanmeqr_saved_cxxflags="$CXXFLAGS"
  CXXFLAGS="$scanmeqr_saved_cxxflags -std=c++20"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <span>]], [[]])],
    [SCANMEQR_STDCXX="-std=c++20"], [SCANMEQR_STDCXX=""])
  CXXFLAGS="$scanmeqr_saved_cxxflags"
  AC_LANG_POP([C++])
  if test -z "$SCANMEQR_STDCXX"; then
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([the C++ core needs C++20; GCC 10+ or Clang 12+ is required.])
  fi
  AC_MSG_RESULT([yes])

  dnl Every kernel is built from the same header with a different
  dnl SCANME_KERNEL_NS and a different -m flag set; mask.cpp picks between them
  dnl at runtime from __builtin_cpu_supports.
  SCANMEQR_CORE_SOURCES="encoder.cpp reed_solomon.cpp matrix.cpp mask.cpp mask_kernel_generic.cpp"
  SCANMEQR_CORE_OPTIONAL="mask_kernel_avx2.cpp mask_kernel_avx512.cpp"

  dnl -- locate the C++ core ------------------------------------------------

  AC_MSG_CHECKING([for the ScanMePHP C++ core])

  dnl clib wins when it is there, and the bundled layout is recognised by a
  dnl *header* rather than a source: once configure has run once in a checkout,
  dnl $srcdir/encoder.cpp exists as a symlink, and keying off it would make the
  dnl second run believe it was looking at the standalone package and drop the
  dnl include paths the headers still live behind.
  SCANMEQR_CLIB=""
  if test "$PHP_SCANMEQR_CLIB" != "yes" && test "$PHP_SCANMEQR_CLIB" != "no"; then
    SCANMEQR_CLIB="$PHP_SCANMEQR_CLIB"
    if test ! -r "$SCANMEQR_CLIB/src/encoder.cpp"; then
      AC_MSG_RESULT([not found])
      AC_MSG_ERROR([encoder.cpp not found in $SCANMEQR_CLIB/src/.])
    fi
  elif test -r "$srcdir/../clib/src/encoder.cpp"; then
    SCANMEQR_CLIB="$srcdir/../clib"
  elif test ! -r "$srcdir/encoder.hpp"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([the C++ core is neither bundled here nor in ../clib. Pass --with-scanmeqr-clib=DIR.])
  fi

  if test -z "$SCANMEQR_CLIB"; then
    AC_MSG_RESULT([bundled])
    PHP_ADD_INCLUDE([$abs_srcdir])
  else
    SCANMEQR_CLIB=`cd "$SCANMEQR_CLIB" && pwd`
    AC_MSG_RESULT([$SCANMEQR_CLIB])

    dnl Symlinked rather than copied so an edit in clib/ is picked up by the
    dnl next make, and file by file rather than as a directory so the .lo and
    dnl .dep files land in php-ext/ instead of inside clib/src.
    for scanmeqr_src in $SCANMEQR_CORE_SOURCES $SCANMEQR_CORE_OPTIONAL; do
      if test -r "$SCANMEQR_CLIB/src/$scanmeqr_src"; then
        ln -sf "$SCANMEQR_CLIB/src/$scanmeqr_src" "$abs_srcdir/$scanmeqr_src"
      fi
    done

    dnl The headers stay where they are: `#include "encoder.hpp"` resolves
    dnl against the symlink's own directory, not the link target's, so they have
    dnl to be reachable through -I either way.
    PHP_ADD_INCLUDE([$SCANMEQR_CLIB/src])
    PHP_ADD_INCLUDE([$SCANMEQR_CLIB/include])
  fi

  dnl -- SIMD kernels -------------------------------------------------------

  SCANMEQR_AVX2_FLAGS="-mavx2 -mbmi -mbmi2 -mpopcnt -mlzcnt"
  SCANMEQR_AVX512_FLAGS="-mavx512f -mavx512bw -mavx512vl -mavx512vpopcntdq -mbmi -mbmi2 -mpopcnt -mlzcnt"
  SCANMEQR_HAVE_AVX2=no
  SCANMEQR_HAVE_AVX512=no

  case $host_cpu in
    i?86|x86_64|amd64)
      AC_LANG_PUSH([C++])
      scanmeqr_saved_cxxflags="$CXXFLAGS"

      AC_MSG_CHECKING([whether $CXX accepts $SCANMEQR_AVX2_FLAGS])
      CXXFLAGS="$scanmeqr_saved_cxxflags $SCANMEQR_STDCXX $SCANMEQR_AVX2_FLAGS -Werror"
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
        [SCANMEQR_HAVE_AVX2=yes], [SCANMEQR_HAVE_AVX2=no])
      AC_MSG_RESULT([$SCANMEQR_HAVE_AVX2])

      AC_MSG_CHECKING([whether $CXX accepts $SCANMEQR_AVX512_FLAGS])
      CXXFLAGS="$scanmeqr_saved_cxxflags $SCANMEQR_STDCXX $SCANMEQR_AVX512_FLAGS -Werror"
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
        [SCANMEQR_HAVE_AVX512=yes], [SCANMEQR_HAVE_AVX512=no])
      AC_MSG_RESULT([$SCANMEQR_HAVE_AVX512])

      CXXFLAGS="$scanmeqr_saved_cxxflags"
      AC_LANG_POP([C++])
      ;;
  esac

  dnl -- symbol visibility --------------------------------------------------

  dnl The C++ core exports 70-odd symbols that nothing outside the extension has
  dnl any business seeing, and PHP dlopens extensions into the global namespace:
  dnl on ELF those definitions then interpose on the identically named ones in
  dnl libscanme_qr.so, so loading the extension corrupted FfiEncoder's results.
  dnl CMake already builds the same sources with CXX_VISIBILITY_PRESET hidden.
  dnl get_module survives because ZEND_DLEXPORT marks it visibility("default").
  AC_MSG_CHECKING([whether $CXX accepts -fvisibility=hidden])
  AC_LANG_PUSH([C++])
  scanmeqr_saved_cxxflags="$CXXFLAGS"
  CXXFLAGS="$scanmeqr_saved_cxxflags -fvisibility=hidden -Werror"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
    [SCANMEQR_VISIBILITY="-fvisibility=hidden -DSCANME_QR_NO_EXPORT"],
    [SCANMEQR_VISIBILITY="-DSCANME_QR_NO_EXPORT"])
  CXXFLAGS="$scanmeqr_saved_cxxflags"
  AC_LANG_POP([C++])
  AC_MSG_RESULT([${SCANMEQR_VISIBILITY:-no}])

  dnl -- build --------------------------------------------------------------

  dnl scanme_qr.c #includes native_encoder.c, so the glue is one translation
  dnl unit; the C++ core is added below, per file, because the two SIMD kernels
  dnl need -m flags that must not reach anything else. Compiling the whole
  dnl extension with -mavx512f would let the compiler emit AVX-512 anywhere and
  dnl the binary would SIGILL on every CPU without it.
  PHP_NEW_EXTENSION([scanmeqr], [scanme_qr.c], [$ext_shared],, [$SCANMEQR_VISIBILITY], [cxx])

  dnl -D rather than AC_DEFINE: mask.cpp is a clib source and never sees PHP's
  dnl config.h, so a define that only lands there would leave the dispatcher
  dnl blind to kernels that were in fact compiled in.
  SCANMEQR_CXXFLAGS="$SCANMEQR_STDCXX $SCANMEQR_VISIBILITY -DZEND_COMPILE_DL_EXT=1"
  if test "$SCANMEQR_HAVE_AVX2" = "yes"; then
    SCANMEQR_CXXFLAGS="$SCANMEQR_CXXFLAGS -DSCANME_HAVE_AVX2_KERNEL"
  fi
  if test "$SCANMEQR_HAVE_AVX512" = "yes"; then
    SCANMEQR_CXXFLAGS="$SCANMEQR_CXXFLAGS -DSCANME_HAVE_AVX512_KERNEL"
  fi

  PHP_ADD_SOURCES_X([$ext_dir], [$SCANMEQR_CORE_SOURCES],
    [$SCANMEQR_CXXFLAGS], [shared_objects_scanmeqr], [yes])

  if test "$SCANMEQR_HAVE_AVX2" = "yes"; then
    PHP_ADD_SOURCES_X([$ext_dir], [mask_kernel_avx2.cpp],
      [$SCANMEQR_CXXFLAGS $SCANMEQR_AVX2_FLAGS], [shared_objects_scanmeqr], [yes])
  fi

  if test "$SCANMEQR_HAVE_AVX512" = "yes"; then
    PHP_ADD_SOURCES_X([$ext_dir], [mask_kernel_avx512.cpp],
      [$SCANMEQR_CXXFLAGS $SCANMEQR_AVX512_FLAGS], [shared_objects_scanmeqr], [yes])
  fi
fi
