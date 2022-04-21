#pragma once

#include <functional>
#include <tuple>
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */

#include <nodable/core/types.h>
#include <nodable/core/reflection/variant.h>
#include <nodable/core/reflection/type.h>
#include <nodable/core/reflection/func_type.h>

namespace Nodable {

    class iinvokable
    {
    public:
        virtual ~iinvokable() {};
        virtual const func_type& get_type() const = 0;
        virtual void invoke(variant *_result, const std::vector<variant *> &_args) const = 0;
    };

    /** Helpers to call a function (need serious work here) */

    /** 0 arg function */
    template<typename T, typename F = T()>
    void call(F *_function, variant *_result, const std::vector<variant *> &_args)
    {
        _result->set( _function() );
    }

    /** 1 arg function */
    template<typename T, typename A0, typename F = T(A0)>
    void call(F *_function, variant *_result, const std::vector<variant *> &_args)
    {
        _result->set( _function( (A0) *_args[0] ) );
    }

    /** 2 arg function */
    template<typename T, typename A0, typename A1, typename F = T(A0, A1)>
    void call(F *_function, variant *_result, const std::vector<variant *> &_args)
    {
        _result->set( _function( (A0) *_args[0], (A1) *_args[1] ) );
    }

    /** 3 arg function */
    template<typename T, typename A0, typename A1, typename A2, typename F = T(A0, A1, A2)>
    void call(F *_function, variant *_result, const std::vector<variant *> &_args)
    {
        _result->set( _function( (A0) *_args[0], (A1) *_args[1], (A2) *_args[2] ) );
    }

    /** 4 arg function */
    template<typename T, typename A0, typename A1, typename A2, typename A3, typename F = T(A0, A1, A2, A3)>
    void call(F *_function, variant *_result, const std::vector<variant *> &_args)
    {
        _result->set( _function( (A0) *_args[0], (A1) *_args[1], (A2) *_args[2], (A3) *_args[3] ) );
    }

    /** 5 arg function */
    template<typename T, typename A0, typename A1, typename A2, typename A3, typename A4, typename F = T(A0, A1, A2, A3, A4)>
    void call(F *_function, variant *_result, const std::vector<variant *> &_args)
    {
        _result->set( _function( (A0) *_args[0], (A1) *_args[1], (A2) *_args[2], (A3) *_args[3], (A4) *_args[4] ) );
    }

    template<typename T>
    class invokable;

    /** Generic Invokable Function */
    template<typename T, typename... Args>
    class invokable<T(Args...)> : public iinvokable
    {
    public:
        using function_t  = T(Args...);
        using return_t    = T;
        using arguments_t = std::tuple<Args...>;

        invokable(function_t* _implem, const char* _name)
        : m_function_impl(_implem)
        , m_function_type(*func_type_builder<function_t>::with_id(_name))
        {
            NODABLE_ASSERT(m_function_impl)
        }

        ~invokable() override {}

        inline void invoke(variant *_result, const std::vector<variant *> &_args) const override
        {
            call<return_t, Args... >(m_function_impl, _result, _args);
        }
        const func_type& get_type() const override { return m_function_type; }

    private:
        function_t* const m_function_impl;
        func_type         m_function_type;
    };

}