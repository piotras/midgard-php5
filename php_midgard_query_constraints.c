/* Copyright (C) 2010 Alexey Zakhlestin <indeyets@gmail.com>
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "php_midgard.h"
#include "php_midgard_query.h"
#include "php_midgard_gobject.h"

#include "php_midgard__helpers.h"

static void php_midgard_array_from_constraints(MidgardQueryConstraintSimple **objects, zval *zarray TSRMLS_DC);


zend_class_entry *php_midgard_query_constraint_simple_class;
zend_class_entry *php_midgard_query_constraint_class;
zend_class_entry *php_midgard_query_constraint_group_class;

// midgard_query_constraint_simple
static PHP_METHOD(midgard_query_constraint_simple, list_constraints)
{
	if (zend_parse_parameters_none() == FAILURE)
		return;

	MidgardQueryConstraintSimple *object = MIDGARD_QUERY_CONSTRAINT_SIMPLE(__php_gobject_ptr(getThis()));

	guint n_objects;
	MidgardQueryConstraintSimple **constraints = midgard_query_constraint_simple_list_constraints(object, &n_objects);

	array_init(return_value);

	if (constraints) {
		php_midgard_array_from_constraints(constraints, return_value TSRMLS_CC);
        g_free(constraints);
	}
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_midgard_query_constraint_simple_list_constraints, 0, 0, 0)
ZEND_END_ARG_INFO()


// midgard_query_constraint
static PHP_METHOD(midgard_query_constraint, __construct)
{
	char *operator = NULL;
	int operator_len = 0;
	zval *z_property = NULL;
	zval *z_holder = NULL;
	zval *z_storage = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "OsO|O",
			&z_property, php_midgard_query_property_class,
			&operator, &operator_len,
			&z_holder, php_midgard_query_holder_class,
			&z_storage, php_midgard_query_storage_class
			) == FAILURE
	) {
		return;
	}

	MidgardQueryProperty *property = MIDGARD_QUERY_PROPERTY(__php_gobject_ptr(z_property));
	MidgardQueryHolder   *holder   = MIDGARD_QUERY_HOLDER(__php_gobject_ptr(z_holder));
	MidgardQueryStorage  *storage  = NULL; 

	if (z_storage) {
		storage = MIDGARD_QUERY_STORAGE(__php_gobject_ptr(z_storage));
	}

	MidgardQueryConstraint *constraint = midgard_query_constraint_new(property, operator, holder, storage);

	if (!constraint) {
		zend_throw_exception_ex(ce_midgard_error_exception, 0 TSRMLS_CC, "Failed to create constraint");
		return;
	}

	MGD_PHP_SET_GOBJECT(getThis(), constraint);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_midgard_query_constraint___construct, 0, 0, 3)
	ZEND_ARG_OBJ_INFO(0, property, midgard_query_property, 0)
	ZEND_ARG_INFO(0, operator)
	ZEND_ARG_OBJ_INFO(0, holder, midgard_query_holder, 0)
	ZEND_ARG_OBJ_INFO(0, storage, midgard_query_storage, 1)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_constraint, get_storage)
{
	if (zend_parse_parameters_none() == FAILURE)
		return;

	MidgardQueryConstraint *constraint = MIDGARD_QUERY_CONSTRAINT(__php_gobject_ptr(getThis()));
	MidgardQueryStorage *storage = midgard_query_constraint_get_storage(constraint);

	if (!storage) {
		return;
	}

	object_init_ex(return_value, php_midgard_query_storage_class);
	MGD_PHP_SET_GOBJECT(return_value, storage);
	// ^^^ no need to call constructor, as constructor doesn't do any magic here
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_midgard_query_constraint_get_storage, 0, 0, 0)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_constraint, set_storage)
{
	zval *z_storage = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &z_storage, php_midgard_query_storage_class) == FAILURE) {
		return;
	}

	MidgardQueryConstraint *constraint = MIDGARD_QUERY_CONSTRAINT(__php_gobject_ptr(getThis()));
	MidgardQueryStorage *storage = MIDGARD_QUERY_STORAGE(__php_gobject_ptr(z_storage));

	zend_bool result = midgard_query_constraint_set_storage(constraint, storage);

	RETURN_BOOL(result);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_midgard_query_constraint_set_storage, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, storage, midgard_query_storage, 0)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_constraint, get_property)
{
	if (zend_parse_parameters_none() == FAILURE)
		return;

	MidgardQueryConstraint *constraint = MIDGARD_QUERY_CONSTRAINT(__php_gobject_ptr(getThis()));
	MidgardQueryProperty *property = midgard_query_constraint_get_property(constraint);

	if (!property) {
		return;
	}

	object_init_ex(return_value, php_midgard_query_property_class);
	MGD_PHP_SET_GOBJECT(return_value, property);
	// ^^^ no need to call constructor, as constructor doesn't do any magic here
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_midgard_query_constraint_get_property, 0, 0, 0)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_constraint, set_property)
{
	zval *z_property = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &z_property, php_midgard_query_property_class) == FAILURE) {
		return;
	}

	MidgardQueryConstraint *constraint = MIDGARD_QUERY_CONSTRAINT(__php_gobject_ptr(getThis()));
	MidgardQueryProperty *property = MIDGARD_QUERY_PROPERTY(__php_gobject_ptr(z_property));

	zend_bool result = midgard_query_constraint_set_property(constraint, property);

	RETURN_BOOL(result);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_midgard_query_constraint_set_property, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, property, midgard_query_property, 0)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_constraint, get_operator)
{
	if (zend_parse_parameters_none() == FAILURE)
		return;

	MidgardQueryConstraint *constraint = MIDGARD_QUERY_CONSTRAINT(__php_gobject_ptr(getThis()));

	const gchar *operator = midgard_query_constraint_get_operator(constraint);

	if (!operator) {
		return;
	}

	RETURN_STRING(operator, 1);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_midgard_query_constraint_get_operator, 0, 0, 0)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_constraint, set_operator)
{
	char *operator = NULL;
	int operator_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &operator, &operator_len) == FAILURE) {
		return;
	}

	MidgardQueryConstraint *constraint = MIDGARD_QUERY_CONSTRAINT(__php_gobject_ptr(getThis()));
	zend_bool result = (zend_bool) midgard_query_constraint_set_operator(constraint, operator);

	RETURN_BOOL(result);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_midgard_query_constraint_set_operator, 0, 0, 1)
	ZEND_ARG_INFO(0, operator)
ZEND_END_ARG_INFO()


// midgard_query_constraint_group



PHP_MINIT_FUNCTION(midgard2_query_constraints)
{
	static function_entry midgard_query_constraint_simple_methods[] = {
		PHP_ME(midgard_query_constraint_simple, list_constraints, arginfo_midgard_query_constraint_simple_list_constraints, ZEND_ACC_PUBLIC)
		{NULL, NULL, NULL}
	};

	static zend_class_entry php_midgard_query_constraint_simple_class_entry;
	INIT_CLASS_ENTRY(php_midgard_query_constraint_simple_class_entry, "midgard_query_constraint_simple", midgard_query_constraint_simple_methods);

	php_midgard_query_constraint_simple_class = zend_register_internal_class(&php_midgard_query_constraint_simple_class_entry TSRMLS_CC);
	php_midgard_query_constraint_simple_class->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
	php_midgard_query_constraint_simple_class->create_object = php_midgard_gobject_new;
	php_midgard_query_constraint_simple_class->doc_comment = strdup("Base class for holding constraint information for midgard_query");


	static function_entry midgard_query_constraint_methods[] = {
		PHP_ME(midgard_query_constraint, __construct,  arginfo_midgard_query_constraint___construct,  ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
		PHP_ME(midgard_query_constraint, get_storage,  arginfo_midgard_query_constraint_get_storage,  ZEND_ACC_PUBLIC)
		PHP_ME(midgard_query_constraint, set_storage,  arginfo_midgard_query_constraint_set_storage,  ZEND_ACC_PUBLIC)
		PHP_ME(midgard_query_constraint, get_property, arginfo_midgard_query_constraint_get_property, ZEND_ACC_PUBLIC)
		PHP_ME(midgard_query_constraint, set_property, arginfo_midgard_query_constraint_set_property, ZEND_ACC_PUBLIC)
		PHP_ME(midgard_query_constraint, get_operator, arginfo_midgard_query_constraint_get_operator, ZEND_ACC_PUBLIC)
		PHP_ME(midgard_query_constraint, set_operator, arginfo_midgard_query_constraint_set_operator, ZEND_ACC_PUBLIC)
		{NULL, NULL, NULL}
	};

	static zend_class_entry php_midgard_query_constraint_class_entry;
	INIT_CLASS_ENTRY(php_midgard_query_constraint_class_entry, "midgard_query_constraint", midgard_query_constraint_methods);

	php_midgard_query_constraint_class = zend_register_internal_class_ex(&php_midgard_query_constraint_class_entry, php_midgard_query_constraint_simple_class, "midgard_query_constraint_simple" TSRMLS_CC);
	php_midgard_query_constraint_class->create_object = php_midgard_gobject_new;
	php_midgard_query_constraint_class->doc_comment = strdup("Class for holding data-atom in midgard_query");

	return SUCCESS;
}



// ===========================
// = Helper-functions follow =
// ===========================

static void php_midgard_array_from_constraints(MidgardQueryConstraintSimple **objects, zval *zarray TSRMLS_DC)
{
	if (!objects)
		return;

	size_t i = 0;

	while (objects[i] != NULL) {
		MidgardQueryConstraintSimple *constraint = objects[i];
		zend_class_entry *ce = NULL;

		if (MIDGARD_IS_QUERY_CONSTRAINT(constraint)) {
			ce = php_midgard_query_constraint_class;
		} else if (MIDGARD_IS_QUERY_CONSTRAINT_GROUP(constraint)) {
			ce = php_midgard_query_constraint_group_class;
		} else {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "found constraint of unknown class. midgard-php has to be fixed");
			return;
		}

		zval *zobject;
		MAKE_STD_ZVAL(zobject);

		object_init_ex(zobject, ce);
		MGD_PHP_SET_GOBJECT(zobject, constraint);
		zend_call_method_with_0_params(&zobject, ce, &ce->constructor, "__construct", NULL);

		zend_hash_next_index_insert(HASH_OF(zarray), &zobject, sizeof(zval *), NULL);

		i++;
	}

	return;
}