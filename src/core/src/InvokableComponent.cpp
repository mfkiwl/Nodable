#include <nodable/core/InvokableComponent.h>

#include <nodable/core/Log.h>
#include <nodable/core/VariableNode.h>
#include <nodable/core/reflection/func_type.h>

using namespace ndbl;

REGISTER
{
    registration::push_class<InvokableComponent>("InvokableComponent").extends<Component>();
}

InvokableComponent::InvokableComponent(const func_type* _signature, bool _is_operator, const iinvokable* _invokable)
        : Component()
        , m_result( nullptr )
        , m_signature(_signature)
        , m_invokable(nullptr)
        , m_is_operator(_is_operator)
{
    NODABLE_ASSERT_EX(_signature != nullptr, "Signature must be defined!")
    m_invokable = _invokable;
    m_args.resize(_signature->get_arg_count(), nullptr );
    m_source_token = std::make_shared<Token>(Token_t::identifier, _signature->get_identifier(), 0 );
}

bool InvokableComponent::update()
{
    bool success = true;

    auto not_declared_predicate = [](Member* _member)
    {
        auto var = _member->get_owner()->as<VariableNode>();
        return var && !var->is_declared();
    };

    /* avoid to invoke if arguments contains an undeclared variable */
    if ( std::find_if(m_args.begin(), m_args.end(), not_declared_predicate) != m_args.end() )
    {
        success = false;
    }
    else if( m_invokable )
    {
        try
        {
            std::vector<variant*> args;
            *m_result->get_variant() = (*m_invokable)( Member::get_variant(m_args, args) );
            for(auto arg : m_args)
            {
                arg->ensure_is_defined(true);
            }
        }
        catch (std::exception& err)
        {
            LOG_ERROR("InvokableComponent", "Exception thrown updating \"%s\" Component"
                                            " while updating Node \"%s\"."
                                            " Reason: %s\n",
                                            get_type().get_name(),
                                            get_owner()->get_label(),
                                            err.what() )
            success = false;
        }
    }

	return success;
}

