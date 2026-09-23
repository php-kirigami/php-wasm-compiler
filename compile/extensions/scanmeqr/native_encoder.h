/*
  +----------------------------------------------------------------------+
  | NativeEncoder class for scanme_qr extension                          |
  +----------------------------------------------------------------------+
*/

#ifndef NATIVE_ENCODER_H
#define NATIVE_ENCODER_H

#include "php.h"

/* NativeEncoder class entry */
extern zend_class_entry *scanme_qr_native_encoder_ce;

/* Initialize NativeEncoder class */
PHPAPI void scanme_qr_register_native_encoder(zend_class_entry *parent_ce);

/* Create NativeEncoder object */
PHPAPI zend_object *scanme_qr_create_native_encoder(void);

#endif	/* NATIVE_ENCODER_H */
