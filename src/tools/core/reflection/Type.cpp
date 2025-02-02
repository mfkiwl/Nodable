#include "Type.h"
#include "Invokable.h"
#include "reflection"
#include <stdexcept>// std::runtime_error

#include <algorithm> // find_if
#include "Operator.h"

using namespace tools;

// declare some types manually to get friendly names
REFLECT_STATIC_INITIALIZER
(
    DEFINE_REFLECT(double);
    DEFINE_REFLECT(bool);
    DEFINE_REFLECT(void);
    DEFINE_REFLECT(void*);
    DEFINE_REFLECT(any);
    DEFINE_REFLECT(null);

    DEFINE_REFLECT_WITH_ALIAS(std::string, "string");
    DEFINE_REFLECT_WITH_ALIAS(i8_t       , "i8");
    DEFINE_REFLECT_WITH_ALIAS(i16_t      , "i16");
    DEFINE_REFLECT_WITH_ALIAS(i32_t      , "i32");
    DEFINE_REFLECT_WITH_ALIAS(i64_t      , "i64");
)

bool type::equals(const TypeDescriptor* left, const TypeDescriptor* right)
{
    ASSERT(left != nullptr);
    return right != nullptr && left->id() == right->id();
}

const TypeDescriptor* type::any()
{
    static const TypeDescriptor* descriptor  = type::get<tools::any>();
    return descriptor;
}

const TypeDescriptor* type::null()
{
    static const TypeDescriptor* descriptor  = type::get<tools::null>();
    return descriptor;
}

bool type::is_implicitly_convertible(const TypeDescriptor* _src, const TypeDescriptor* _dst )
{
    return _src->is_implicitly_convertible(_dst);
}

bool TypeDescriptor::is_implicitly_convertible(const TypeDescriptor* _dst ) const
{
    if( _dst->is_const() )
        return false;

    if (this->equals(_dst))
        return true;

    if (!this->is_ptr() && !_dst->is_ptr() && this->m_primitive_id == _dst->m_primitive_id)
        return true;

    if(this->is<any>() || _dst->is<any>() ) // We allow cast to unknown type
        return true;

    return
        // Allows specific casts:
        //  from                 to
        this->is<i16_t>() && _dst->is<i32_t>()  ||
        this->is<i16_t>() && _dst->is<double>() ||
        this->is<i32_t>() && _dst->is<double>();
}

bool TypeDescriptor::any_of(std::vector<const TypeDescriptor*> types) const
{
    for ( auto each : types )
        if(equals(each))
            return true;
    return false;
}

ClassDescriptor::~ClassDescriptor()
{
    for (auto* each : m_methods )
        delete each;

    for (auto* each : m_static_methods )
        delete each;
}


bool ClassDescriptor::is_child_of(std::type_index _possible_parent_id, bool _selfCheck) const
{
    if (_selfCheck && m_id == _possible_parent_id )
    {
        return true;
    }

    if( !has_parent() )
    {
        return false;
    }

    auto direct_parent_found = m_parents.find( _possible_parent_id );

    // direct parent check
    if ( direct_parent_found != m_parents.end())
    {
        return true;
    }

    // indirect parent check
    for (std::type_index parent_id : m_parents)
    {
        auto parent_class = TypeRegister::get_class(parent_id);
        if (parent_class->is_child_of(_possible_parent_id, true))
        {
            return true;
        }
    }

    return false;
};

void ClassDescriptor::add_parent(std::type_index parent)
{
    m_parents.insert(parent);
    m_flags |= TypeFlag_HAS_PARENT;
}

void ClassDescriptor::add_child(std::type_index _child)
{
    m_children.insert( _child );
    m_flags |= TypeFlag_HAS_CHILD;
}

void ClassDescriptor::add_static(const char* _name, const IInvokable* _func_type)
{
    m_static_methods.insert(_func_type);
    m_static_methods_by_name.insert({_name, _func_type});
}

void ClassDescriptor::add_method(const char* _name, const IInvokableMethod* _func_type)
{
    m_methods.insert(_func_type);
    m_methods_by_name.insert({_name, _func_type});
}

const IInvokableMethod* ClassDescriptor::get_method(const char* _name) const
{
    auto found = m_methods_by_name.find(_name);
    if( found != m_methods_by_name.end() )
    {
        return found->second;
    }
    return nullptr;
}

const IInvokable* ClassDescriptor::get_static(const char*  _name)const
{
    auto found = m_static_methods_by_name.find(_name);
    if( found != m_static_methods_by_name.end() )
    {
        return found->second;
    }
    return nullptr;
}

void FunctionDescriptor::push_arg( const TypeDescriptor* _type, bool _pass_by_ref )
{
    size_t index     = m_argument.size();
    FuncArg& arg     = m_argument.emplace_back();
    arg.type         = _type;
    arg.name         = "arg_" + std::to_string( index );
    arg.pass_by_ref  = _pass_by_ref;
}

bool FunctionDescriptor::is_exactly(const FunctionDescriptor* _other)const
{
    if ( this == _other )
        return true;
    if (m_argument.size() != _other->m_argument.size())
        return false;
    if ( m_name != _other->m_name )
        return false;
    if ( m_argument.empty() )
        return true;

    size_t i = 0;
    while(i < m_argument.size() )
    {
        const TypeDescriptor* arg_t       = m_argument[i].type;
        const TypeDescriptor* other_arg_t = _other->m_argument[i].type;

        if ( !arg_t->equals(other_arg_t) )
        {
            return false;
        }
        i++;
    }
    return true;
}

bool FunctionDescriptor::is_compatible(const FunctionDescriptor* _other)const
{
    if ( this == _other )
        return true;
    if (m_argument.size() != _other->m_argument.size())
        return false;
    if ( m_name != _other->m_name )
        return false;
    if ( m_argument.empty() )
        return true;

    size_t i = 0;
    while(i < m_argument.size() )
    {
        const TypeDescriptor* arg_t       = m_argument[i].type;
        const TypeDescriptor* other_arg_t = _other->m_argument[i].type;

        if ( !arg_t->equals(other_arg_t) &&
             !other_arg_t->is_implicitly_convertible(arg_t) )
        {
            return false;
        }
        i++;
    }
    return true;

}

bool FunctionDescriptor::has_arg_with_type(const TypeDescriptor* _type) const
{
    auto found = std::find_if(m_argument.begin(), m_argument.end(), [&_type](const FuncArg& each) { return each.type->equals(_type); } );
    return found != m_argument.end();
}
