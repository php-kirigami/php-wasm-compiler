/*
  +----------------------------------------------------------------------+
  | PHP Version 8.1+                                                     |
  +----------------------------------------------------------------------+
  | Copyright (c) 2024 Crazy Goat                                        |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | https://www.php.net/license/3_01.txt                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Crazy Goat <crazy@goat.com>                                  |
  +----------------------------------------------------------------------+
*/

#ifndef PHP_SCANME_QR_H
#define PHP_SCANME_QR_H

extern zend_module_entry scanme_qr_module_entry;
#define phpext_scanme_qr_ptr &scanme_qr_module_entry

#define PHP_SCANME_QR_VERSION "0.5.2"

#if defined(ZTS) && defined(COMPILE_DL_SCANME_QR)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif	/* PHP_SCANME_QR_H */
