/*
 * Copyright (C) 2009 Piotr Pokora <piotrek.pokora@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of\
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <zend_interfaces.h>

#include "php_midgard.h"
#include "php_midgard_gobject.h"

#include "php_midgard__helpers.h"

/* ReflectionMethod::getDocComment workaround */
static GHashTable *midgard_classes = NULL;

void 
__initialize_midgard_classes_hash (void)
{
	if (midgard_classes == NULL)
		midgard_classes = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify)g_hash_table_destroy);

	return;
}

void 
php_midgard_docs_add_class (const gchar *classname)
{
	GHashTable *class_methods = g_hash_table_lookup (midgard_classes, (gpointer) classname);

	if (!class_methods) {
		
		class_methods = g_hash_table_new (g_str_hash, g_str_equal);
		/* Do not copy classname, it's available till module unload */
		g_hash_table_insert (midgard_classes, (gpointer)classname, (gpointer)class_methods);
	}
}

void 
php_midgard_docs_add_method_comment (const gchar *classname, const gchar *method, const gchar *comment)
{
	php_midgard_docs_add_class (classname);
	
	GHashTable *class_methods = g_hash_table_lookup (midgard_classes, (gpointer) classname);
	/* Do not copy, comment is static */
	g_hash_table_insert (class_methods, (gpointer)method, (gpointer)comment);
}

const char* php_midgard_docs_get_method_comment (const gchar *classname, const gchar *method)
{
	GHashTable *class_methods = g_hash_table_lookup (midgard_classes, (gpointer) classname);
	
	if (!class_methods)
		return "";

	const char *comment = (const char *) g_hash_table_lookup (class_methods, method);

	return comment ? comment : "";
}

/* End of ReflectionMethod::getDocComment workaround */

/* copypasted from php_reflection.c {{{ */
typedef enum {
	REF_TYPE_OTHER = 0,      /* Must be 0 */
	REF_TYPE_FUNCTION,
	REF_TYPE_PARAMETER,
	REF_TYPE_PROPERTY
} reflection_type_t;

typedef struct {
	zend_object zo;
	void *ptr;
	reflection_type_t ref_type;
	zval *obj;
	zend_class_entry *ce;
} reflection_object;

#define GET_REFLECTION_OBJECT_PTR(target)                                                                   \
	intern = (reflection_object *) zend_object_store_get_object(getThis() TSRMLS_CC);                       \
	if (intern == NULL || intern->ptr == NULL) {                                                            \
		zend_error(E_ERROR, "Internal error: Failed to retrieve the reflection object");                    \
	}                                                                                                       \
	target = intern->ptr;                                                                                   \

/* }}} */


static zend_class_entry *php_midgard_reflection_method_class = NULL;
static zend_class_entry *php_midgard_reflection_class_class = NULL;
static zend_class_entry *zend_reflection_function_class = NULL;
static zend_class_entry *zend_reflection_class_class = NULL;

static PHP_METHOD(php_midgard_reflection_method, __construct)
{
	zval *this = getThis();
	zval *arg1, *arg2;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &arg1, &arg2) == FAILURE)
		return;

	zend_call_method_with_2_params(&this, zend_reflection_function_class, &zend_reflection_function_class->constructor, "__construct", NULL, arg1, arg2);
}

static PHP_METHOD(php_midgard_reflection_method, getDocComment)
{
	reflection_object *intern;
	zend_function *fptr;
	RETVAL_FALSE;

	if (zend_parse_parameters_none() == FAILURE) 
		return;

	GET_REFLECTION_OBJECT_PTR(fptr);

	if (fptr->type == ZEND_USER_FUNCTION) {
		if (fptr->op_array.doc_comment) {
			RETURN_STRINGL(fptr->op_array.doc_comment, fptr->op_array.doc_comment_len, 1);
		} else {
			RETURN_FALSE;
		}
	} else {
		zval *ref_class = zend_read_property(php_midgard_reflection_class_class, getThis(), "class", sizeof("class")-1, 0 TSRMLS_CC); \
		zval *ref_method = zend_read_property(php_midgard_reflection_class_class, getThis(), "name", sizeof("name")-1, 0 TSRMLS_CC); \

		if (!ref_class || !ref_method)
			RETURN_FALSE;

		const char *comment = php_midgard_docs_get_method_comment((const gchar *) Z_STRVAL_P(ref_class), (const gchar *) Z_STRVAL_P(ref_method));
		RETURN_STRING ((char *)comment, 1);
	}
}

/* ReflectionClass */

#define _GET_RC_CE  \
	zval *_ref_class; \
	zend_class_entry *ce = NULL; \
	_ref_class = zend_read_property(php_midgard_reflection_class_class, getThis(), "name", sizeof("name")-1, 0 TSRMLS_CC); \
	if (_ref_class) { \
		ce = zend_fetch_class(Z_STRVAL_P(_ref_class), Z_STRLEN_P(_ref_class), ZEND_FETCH_CLASS_NO_AUTOLOAD TSRMLS_CC); \
	} \

static PHP_METHOD(php_midgard_reflection_class, __construct)
{
	zval *this = getThis();
	zval *arg1;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &arg1) == FAILURE)
		return;

	zend_call_method_with_1_params(&this, zend_reflection_class_class, &zend_reflection_class_class->constructor, "__construct", NULL, arg1);
}

static PHP_METHOD(php_midgard_reflection_class, getDocComment)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	_GET_RC_CE;

	if (ce == NULL)
		RETURN_NULL();

	RETURN_STRING(ce->doc_comment ? ce->doc_comment : "", 1);
}

static PHP_METHOD(php_midgard_reflection_class, listSignals)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	_GET_RC_CE;
	array_init(return_value);

	if (ce == NULL)
		return;

	GType classtype = g_type_from_name(php_class_name_to_g_class_name(ce->name));
	guint *ids;
	guint n_ids;
	guint i;

	if (!classtype)
		return;

	ids = g_signal_list_ids(classtype, &n_ids);

	if (ids == NULL)
		return;

	for (i = 0; i < n_ids; i++) {

		zval *signalname;
		MAKE_STD_ZVAL(signalname);
		ZVAL_STRING(signalname, (char *) g_signal_name(ids[i]), 1);
		zend_hash_next_index_insert(HASH_OF(return_value), &signalname, sizeof(zval *), NULL);
	}

	g_free(ids);
}

PHP_MINIT_FUNCTION(midgard2_reflection_workaround)
{
	__initialize_midgard_classes_hash ();

	zend_reflection_function_class = php_midgard_get_class_ptr_by_name("reflectionmethod");
	zend_reflection_class_class = php_midgard_get_class_ptr_by_name("reflectionclass");

	/* Extend ReflectionMethod */
	static function_entry midgard_reflection_method_methods[] = {
		PHP_ME(php_midgard_reflection_method, __construct,   NULL, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
		PHP_ME(php_midgard_reflection_method, getDocComment, NULL, ZEND_ACC_PUBLIC)
		{NULL, NULL, NULL}
	};

	/* Extend ReflectionClass */
	static function_entry midgard_reflection_class_methods[] = {
		PHP_ME(php_midgard_reflection_class, __construct,   NULL, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
		PHP_ME(php_midgard_reflection_class, getDocComment, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(php_midgard_reflection_class, listSignals,   NULL, ZEND_ACC_PUBLIC)
		{NULL, NULL, NULL}
	};

	static zend_class_entry _reflection_ce;
	INIT_CLASS_ENTRY(_reflection_ce, "midgard_reflection_method", midgard_reflection_method_methods);

	php_midgard_reflection_method_class = zend_register_internal_class_ex(&_reflection_ce, zend_reflection_function_class, NULL TSRMLS_CC);
	php_midgard_reflection_method_class->doc_comment = g_strdup("Helps Midgard to show doc_comments of methods of internal classes");

	php_midgard_docs_add_method_comment("midgard_reflection_method", "getDocComment", "returns doc_comment of method");

	INIT_CLASS_ENTRY(_reflection_ce, "midgard_reflection_class", midgard_reflection_class_methods);

	php_midgard_reflection_class_class = zend_register_internal_class_ex(&_reflection_ce, zend_reflection_class_class, NULL TSRMLS_CC);
	php_midgard_reflection_class_class->doc_comment = g_strdup("Helps Midgard to show doc_comments of internal classes");

	return SUCCESS;
}
