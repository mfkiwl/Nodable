#include <nodable/core/NodeFactory.h>
#include <nodable/core/InstructionNode.h>
#include <nodable/core/VariableNode.h>
#include <nodable/core/LiteralNode.h>
#include <nodable/core/Language.h>
#include <nodable/core/InvokableComponent.h>
#include <nodable/core/Scope.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

using namespace Nodable;

InstructionNode* NodeFactory::new_instr() const
{
    InstructionNode* instr_node = new InstructionNode();
    instr_node->set_label("Instr.", "");
    m_post_process(instr_node);
    return instr_node;
}

VariableNode* NodeFactory::new_variable(std::shared_ptr<const R::MetaType> _type, const std::string& _name, IScope *_scope) const
{
    // create
    auto* node = new VariableNode(_type);
    node->set_name(_name.c_str());

    if( _scope)
    {
        _scope->add_variable(node);
    }
    else
    {
        LOG_WARNING("HeadlessNodeFactory", "Variable %s has been created without defining its scope.\n", _name.c_str())
    }

    m_post_process(node);

    return node;
}

Node* NodeFactory::new_operator(const InvokableOperator* _operator) const
{
    switch (_operator->get_operator_type() )
    {
        case InvokableOperator::Type::Binary:
            return new_binary_op(_operator);
        case InvokableOperator::Type::Unary:
            return new_unary_op(_operator);
        default:
            return nullptr;
    }
}

Node* NodeFactory::new_binary_op(const InvokableOperator* _operator) const
{
    // Create a node with 2 inputs and 1 output
    auto node = new Node();

    setup_node_labels(node, _operator);

    const FunctionSignature* signature = _operator->get_signature();
    const auto args = signature->get_args();
    auto props   = node->props();
    Member* left   = props->add(k_lh_value_member_name, Visibility::Default, args[0].m_type, Way_In);
    Member* right  = props->add(k_rh_value_member_name, Visibility::Default, args[1].m_type, Way_In);
    Member* result = props->add(k_value_member_name, Visibility::Default, signature->get_return_type(), Way_Out);

    // Create ComputeBinaryOperation component and link values.
    auto binOpComponent = new InvokableComponent( _operator );
    binOpComponent->set_result(result);
    binOpComponent->set_l_handed_val(left);
    binOpComponent->set_r_handed_val(right);
    node->add_component(binOpComponent);

    m_post_process(node);

    return node;
}

void NodeFactory::setup_node_labels(Node *_node, const InvokableOperator *_operator)
{
    _node->set_label(_operator->get_signature()->get_label().c_str(), _operator->get_short_identifier().c_str());
}

Node* NodeFactory::new_unary_op(const InvokableOperator* _operator) const
{
    // Create a node with 2 inputs and 1 output
    auto node = new Node();

    setup_node_labels(node, _operator);

    const FunctionSignature* signature = _operator->get_signature();
    const auto args = signature->get_args();
    Properties* props = node->props();
    Member* left = props->add(k_lh_value_member_name, Visibility::Default, args[0].m_type, Way_In);
    Member* result = props->add(k_value_member_name, Visibility::Default, signature->get_return_type(), Way_Out);

    // Create ComputeBinaryOperation binOpComponent and link values.
    auto unaryOperationComponent = new InvokableComponent( _operator );
    unaryOperationComponent->set_result(result);
    unaryOperationComponent->set_l_handed_val(left);
    node->add_component(unaryOperationComponent);

    m_post_process(node);

    return node;
}

Node* NodeFactory::new_abstract_function(const FunctionSignature* _signature) const
{
    auto node = _new_abstract_function(_signature);

    // Create an InvokableComponent with the function.
    auto functionComponent = new InvokableComponent( /* no InvokableFunction */);
    node->add_component(functionComponent);

    m_post_process(node);
    return node;
}

Node* NodeFactory::_new_abstract_function(const FunctionSignature* _signature) const
{
    Node* node              = new Node();
    std::string label       = _signature->get_identifier() + "()";
    std::string short_label = _signature->get_label().substr(0, 2) + "..()";
    node->set_label(label.c_str(), short_label.c_str());

    // Create a result/value
    Properties* props = node->props();
    props->add(k_value_member_name, Visibility::Default, _signature->get_return_type(), Way_Out);

    // Create arguments
    auto args = _signature->get_args();
    for (auto& arg : args)
    {
        props->add(arg.m_name.c_str(), Visibility::Default, arg.m_type, Way_In); // create node input
    }

    m_post_process(node);

    return node;
}

Node* NodeFactory::new_function(const IInvokable* _function) const
{
    // Create an abstract function node
    Node* node = _new_abstract_function(_function->get_signature());
    Properties* props = node->props();

    // Create an InvokableComponent with the function.
    auto functionComponent = new InvokableComponent( _function );
    node->add_component(functionComponent);

    // Link result member
    functionComponent->set_result(props->get(k_value_member_name));

    // Link arguments
    auto args = _function->get_signature()->get_args();
    for (size_t argIndex = 0; argIndex < args.size(); argIndex++)
    {
        Member* member = props->get(args[argIndex].m_name.c_str());
        functionComponent->set_arg(argIndex, member);
    }

    m_post_process(node);
    return node;
}

Node* NodeFactory::new_scope() const
{
    auto scope_node = new Node();
    scope_node->set_label("{} Scope", "{}");

    scope_node->predecessor_slots().set_limit(std::numeric_limits<int>::max());
    scope_node->successor_slots().set_limit(1);

    auto* scope = new Scope();
    scope_node->add_component(scope);

    m_post_process(scope_node);

    return scope_node;
}

ConditionalStructNode* NodeFactory::new_cond_struct() const
{
    auto cond_struct_node = new ConditionalStructNode();
    cond_struct_node->set_label("If");

    cond_struct_node->predecessor_slots().set_limit(std::numeric_limits<int>::max());
    cond_struct_node->successor_slots().set_limit(2); // true/false branches

    auto* scope = new Scope();
    cond_struct_node->add_component(scope);

    m_post_process(cond_struct_node);

    return cond_struct_node;
}

ForLoopNode* NodeFactory::new_for_loop_node() const
{
    auto for_loop = new ForLoopNode();
    for_loop->set_label("For loop", "For");

    for_loop->predecessor_slots().set_limit(std::numeric_limits<int>::max());
    for_loop->successor_slots().set_limit(1);

    auto* scope = new Scope();
    for_loop->add_component(scope);

    m_post_process(for_loop);

    return for_loop;
}

Node* NodeFactory::new_program() const
{
    Node* prog = new_scope();
    prog->set_label(ICON_FA_FILE_CODE " Program", ICON_FA_FILE_CODE);

    m_post_process(prog);
    return prog;
}

Node* NodeFactory::new_node() const
{
    auto node = new Node();
    m_post_process(node);
    return node;
}

LiteralNode* NodeFactory::new_literal(std::shared_ptr<const R::MetaType> _type) const
{
    LiteralNode* node = new LiteralNode(_type);
    node->set_label("Literal", "");
    m_post_process(node);
    return node;
}
