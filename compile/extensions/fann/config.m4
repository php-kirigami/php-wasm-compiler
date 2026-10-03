dnl config.m4 for extension fann

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary.

PHP_ARG_WITH([fann],
  [for fann support],
  [AS_HELP_STRING([--with-fann],
    [Include vendored FANN support])],
  [yes],
  [yes])

if test "$PHP_FANN" != "no"; then
  FANN_SRC_DIR=""
  if test -d "$ext_srcdir/lib/libfann"; then
    FANN_SRC_DIR="$ext_srcdir/lib/libfann"
  elif test -d "$srcdir/lib/libfann"; then
    FANN_SRC_DIR="$srcdir/lib/libfann"
  elif test -d "./lib/libfann"; then
    FANN_SRC_DIR="./lib/libfann"
  else
    AC_MSG_ERROR([Vendored libfann not found in lib/libfann.])
  fi

  PHP_ADD_INCLUDE("$FANN_SRC_DIR/src")
  PHP_ADD_INCLUDE("$FANN_SRC_DIR/src/include")

  AC_DEFINE(HAVE_FANN, 1, [Have fann support])
  PHP_SUBST(FANN_SHARED_LIBADD)

  FANN_SOURCES="fann.c lib/libfann/src/floatfann.c"
  PHP_NEW_EXTENSION(fann, $FANN_SOURCES, $ext_shared)
fi
