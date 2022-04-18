/*
	Macros to easily bind cpp functions to a Nodable::Language.
    These MACROS must be used within Nodable::Language non-static scope
*/

/**
* Wrap a native function with a specific signature.
* If your function is not polymorphic you can also use BIND_FUNCTION
*
* ex: Same functions with two different signatures:
*
*  BIND_FUNCTION_T( sin, double(double) )
*  BIND_FUNCTION_T( sin, double(float) )
*/
#define BIND_FUNCTION_T( func, func_t ) \
    { \
        const Signature* sig = new_function_signature<func_t>(#func); \
        add_invokable( new Invokable<func_t>(func, sig ) );\
    }

/**
* Wrap a native function with a specific signature as an operator.
* If your function is not polymorphic you can also use BIND_OPERATOR
*
* ex: Same function name with different label and signatures:
 *
*  BIND_OPERATOR_T( add, "+", double(double, double) )
*  BIND_OPERATOR_T( add, "+", std::string(std::string, double) )
*/
#define BIND_OPERATOR_T( func, id, func_t ) \
    { \
        const Signature* sig = new_operator_signature<func_t>(id);\
        add_invokable( new Invokable<func_t>(func, sig ) );\
    }

/**
* Wrap a native non-polymorphic function.
*
* ex: BIND_FUNCTION( my_unique_name_function )
*/
#define BIND_FUNCTION( function ) BIND_FUNCTION_T( function, decltype(function))

/**
* Wrap a native non-polymorphic function.
*
* ex: BIND_OPERATOR( my_unique_name_function, "+")
*/
#define BIND_OPERATOR( function, identifier ) BIND_OPERATOR_T( function, identifier, decltype(function) )