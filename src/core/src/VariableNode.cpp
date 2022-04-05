#include <nodable/core/VariableNode.h>
#include <nodable/core/Member.h>
#include <nodable/core/Log.h>

using namespace Nodable;

R_DEFINE_CLASS(VariableNode)

VariableNode::VariableNode(std::shared_ptr<const R::MetaType> _type)
    : Node("Variable")
    , m_type_token(nullptr)
    , m_identifier_token(nullptr)
    , m_assignment_operator_token(nullptr)
    , m_is_declared(true)
    , m_scope(nullptr)
{
    NODABLE_ASSERT(_type)
	m_value = m_props.add(k_value_member_name, Visibility::Always, _type, Way_InOut);
}

void VariableNode::set_name(const char* _name)
{
    std::string label;
    const char* short_label = nullptr;

    m_name = _name;

    if (m_value->get_data()->is_initialized())                   // append type only if have one
    {
        label.append(m_value->get_meta_type()->get_name() );
        label.append(" ");
    }
    label.append(_name );                                        // append name

    size_t length_max = 8;
	if (m_name.length() > length_max)                            // limit short_label length
    {
	    std::string tail = "..";
        short_label = m_name.substr(0, length_max-1-tail.length())
                            .append(tail).c_str();
    }

    set_label(label.c_str(), short_label);
}

bool VariableNode::eval() const
{
    if( !m_value->get_data()->is_initialized() )
    {
        m_value->get_data()->set_initialized(true);
        return Node::eval();
    }
    return true;
}
