/*
  +----------------------------------------------------------------------+
  | cmark                                                                |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2018                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: krakjoe <krakjoe@php.net>                                    |
  +----------------------------------------------------------------------+
 */
#ifndef PHP_CMARK_NODE_H
#define PHP_CMARK_NODE_H

#include <cmark.h>

extern zend_class_entry *php_cmark_node_ce;
extern zend_object_handlers   php_cmark_node_handlers;

typedef struct _php_cmark_node_t {
	cmark_node* node;
	zend_bool owned;
	struct {
		zend_refcounted_h gc;
		uint32_t handle;
		uint32_t extra_flags; /* CLAUDE.md decision 35: PHP 8.5 added this field to the real zend_object struct after krakjoe/cmark last touched this file (2019). This hand-rolled mimic MUST mirror zend_object's exact field order, or every offsetof-based access below (ce, handlers, properties, and Zend's own properties_table walk during object_properties_init/zend_object_std_dtor) silently reads/writes the wrong memory instead of crashing cleanly. */
		zend_class_entry *ce;
		const zend_object_handlers *handlers;
		HashTable *properties;
	} std;
	zval parent;
	zval previous;
	zval next;
	zval firstChild;
	zval lastChild;

	zval startLine;
	zval endLine;
	zval startColumn;
	zval endColumn;
} php_cmark_node_t;

#define php_cmark_node_from(o) \
	((php_cmark_node_t*) \
		((char*) o - XtOffsetOf(php_cmark_node_t, std)))
#define php_cmark_node_fetch(z) php_cmark_node_from(Z_OBJ_P(z))
/* CLAUDE.md decision 34: PHP 8's object handlers (read_property et al.)
 * receive a zend_object* directly instead of a zval* — this fetches from
 * that zend_object* without going through Z_OBJ_P(). */
#define php_cmark_node_fetch_obj(o) php_cmark_node_from(o)
#define php_cmark_node_zend(z) ((zend_object*) &(z)->std)

extern PHP_MINIT_FUNCTION(CommonMark_Node);
extern PHP_RINIT_FUNCTION(CommonMark_Node);

extern void php_cmark_node_new(zval *object, cmark_node_type type);
extern void php_cmark_node_list_new(zval *object, cmark_list_type type);
extern php_cmark_node_t* php_cmark_node_shadow(zval *return_value, cmark_node *node);
extern zend_class_entry* php_cmark_node_class(cmark_node* node);
#endif
