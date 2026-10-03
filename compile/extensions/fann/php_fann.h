/* fann extension for PHP */

#ifndef PHP_FANN_H
# define PHP_FANN_H

extern zend_module_entry fann_module_entry;
# define phpext_fann_ptr &fann_module_entry

# define PHP_FANN_VERSION "1.0.0"

# if defined(ZTS) && defined(COMPILE_DL_FANN)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

#endif	/* PHP_FANN_H */
