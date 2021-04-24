#include "Serializer.h"
#include "Member.h"
#include "Component/ComputeBinaryOperation.h"
#include "Component/ComputeUnaryOperation.h"
#include "Node/GraphNode.h"
#include "Node/Node.h"
#include "Node/VariableNode.h"
#include "Node/CodeBlockNode.h"
#include "Node/ScopedCodeBlockNode.h"
#include "Node/InstructionNode.h"
#include "Node/ConditionalStructNode.h"

using namespace Nodable;

std::string Serializer::serialize(const ComputeUnaryOperation* _operation) const
{
    std::string result;

    auto args = _operation->getArgs();
    auto inner_operator = _operation->getOwner()->getConnectedOperator(args[0]);

    // operator ( ... innerOperator ... )   ex:   -(a+b)

    // Operator
    const Token* sourceToken = _operation->getSourceToken();
    if ( sourceToken )
    {
        result.append( sourceToken->m_prefix);
    }

    result.append(_operation->getOperator()->identifier);

    if ( sourceToken )
    {
        result.append( sourceToken->m_suffix);
    }

    // Inner part of the expression
    {
        bool needBrackets = inner_operator;

        if (needBrackets)
        {
            result.append(serialize(TokenType_OpenBracket));
        }

        result.append(serialize(args[0]));

        if (needBrackets)
        {
            result.append(serialize(TokenType_CloseBracket));
        }
    }

    return result;
}

std::string Serializer::serialize(const ComputeBinaryOperation * _operation) const
{
    std::string result;

    // Get the left and right source operator
    std::vector<Member*> args = _operation->getArgs();
    auto l_handed_operator = _operation->getOwner()->getConnectedOperator(args[0]);
    auto r_handed_operator = _operation->getOwner()->getConnectedOperator(args[1]);
    // Left part of the expression
    {
        // TODO: check parsed brackets for prefix/suffix
        bool needBrackets = l_handed_operator && !language->hasHigherPrecedenceThan(l_handed_operator, _operation->getOperator());
        if (needBrackets)
        {
            result.append( serialize(TokenType_OpenBracket));
        }

        result.append(serialize(args[0]));

        if (needBrackets)
        {
            result.append( serialize(TokenType_CloseBracket));
        }
    }

    // Operator
    const Token* sourceToken = _operation->getSourceToken();
    if ( sourceToken )
    {
        result.append( sourceToken->m_prefix);
    }
    result.append(_operation->getOperator()->identifier);
    if ( sourceToken )
    {
        result.append( sourceToken->m_suffix);
    }

    // Right part of the expression
    {
        // TODO: check parsed brackets for prefix/suffix
        bool needBrackets = r_handed_operator
                            && (  r_handed_operator->getType() == Operator::Type::Unary
                                  || !language->hasHigherPrecedenceThan(r_handed_operator, _operation->getOperator())
                               );

        if (needBrackets)
        {
            result.append(serialize(TokenType_OpenBracket));
        }

        result.append(serialize(args[1]));

        if (needBrackets)
        {
            result.append(serialize(TokenType_CloseBracket));
        }
    }

    return result;
}

std::string Serializer::serialize(const ComputeFunction *_computeFunction)const
{
    std::string result = serialize(
            _computeFunction->getFunction()->signature,
            _computeFunction->getArgs());
    return result;
}

std::string Serializer::serialize(const ComputeBase *_operation)const
{

    std::string result;

    if( auto computeBinOp = _operation->as<ComputeBinaryOperation>() )
    {
        result = serialize(computeBinOp);
    }
    else if (auto computeUnaryOp = _operation->as<ComputeUnaryOperation>() )
    {
        result = serialize(computeUnaryOp);
    }
    else if (auto fct = _operation->as<ComputeFunction>())
    {
        result = serialize(fct);
    }

    return result;
}

std::string Serializer::serialize(
        const FunctionSignature&   _signature,
        std::vector<Member*> _args) const
{
    std::string expr;
    expr.append(_signature.getIdentifier());
    expr.append(serialize(TokenType_OpenBracket));

    for (auto it = _args.begin(); it != _args.end(); it++) {
        expr.append(serialize(*it));

        if (*it != _args.back()) {
            expr.append(serialize(TokenType_Separator));
        }
    }

    expr.append(serialize(TokenType_CloseBracket));
    return expr;

}

std::string Serializer::serialize(const FunctionSignature& _signature) const {

    std::string result = _signature.getIdentifier() + serialize(TokenType_OpenBracket);
    auto args = _signature.getArgs();

    for (auto it = args.begin(); it != args.end(); it++) {

        if (it != args.begin()) {
            result.append(serialize(TokenType_Separator));
            result.append(" ");

        }
        const auto argType = (*it).type;
        result.append( serialize(argType) );

    }

    result.append( serialize(TokenType_CloseBracket) );

    return result;

}

std::string Serializer::serialize(const TokenType& _type) const
{
    return language->getSemantic()->tokenTypeToString(_type);
}

std::string Serializer::serialize(const VariableNode* _node) const
{
    std::string result;
    Member* value = _node->value();

    // type
    result.append( serialize(_node->getTypeToken() ) );

    // var name
    result.append( _node->getIdentifierToken()->m_prefix);
    result.append( _node->getName());
    result.append( _node->getIdentifierToken()->m_suffix);

    // assigment ?
    if ( _node->getAssignmentOperatorToken() )
    {
        result.append(_node->getAssignmentOperatorToken()->m_prefix );
        result.append(_node->getAssignmentOperatorToken()->m_word );
        result.append(_node->getAssignmentOperatorToken()->m_suffix );

        if ( value->hasInputConnected() )
            result.append(serialize(value));
        else
        {
            result.append( _node->value()->getSourceToken()->m_prefix);
            serialize(result, _node->value()->getData());
            result.append( _node->value()->getSourceToken()->m_suffix);
        }

    }

    return result;
}

void Serializer::serialize(std::string& out, const Variant* variant) const
{
    if (variant->isType(Type_String))
    {
        out.append('"' + (std::string)*variant + '"');
    }
    else
    {
        out.append( (std::string)*variant );
    }
}

std::string Serializer::serialize(const Member * _member, bool followConnections) const
{

    std::string expression;

    const Token *sourceToken = _member->getSourceToken();
    if (sourceToken)
    {
        expression.append(sourceToken->m_prefix);
    }

    auto owner = _member->getOwner();
    if ( followConnections && owner && _member->allowsConnection(Way_In) && owner->hasWireConnectedTo(_member) )
    {
        auto sourceMember = owner->getSourceMemberOf(_member);

        if ( auto computeBase = sourceMember->getOwner()->getComponent<ComputeBase>() )
        {
            expression.append( serialize(computeBase) );
        }
        else
        {
            expression.append( serialize(sourceMember, false) );
        }
    }
    else
    {
        if (owner && owner->getClass() == mirror::GetClass<VariableNode>())
        {
            auto variable = owner->as<VariableNode>();
            expression.append( variable->getName() );
        }
        else
        {
            serialize( expression, _member->getData() );
        }
    }

    if (sourceToken)
    {
        expression.append(sourceToken->m_suffix);
    }

    return expression;
}

std::string Serializer::serialize(const CodeBlockNode* _block) const
{
    if (_block->getChildren().empty())
    {
        return "";
    }

    std::string result;

    for( auto& eachChild : _block->getChildren() )
    {
        auto clss = eachChild->getClass();
        if ( clss == InstructionNode::GetClass())
        {
            result.append( serialize(eachChild->as<InstructionNode>()) );
        }
        else if ( clss == ScopedCodeBlockNode::GetClass())
        {
            result.append( serialize(eachChild->as<ScopedCodeBlockNode>()) );
        }
        else if ( clss == ConditionalStructNode::GetClass())
        {
            result.append( serialize(eachChild->as<ConditionalStructNode>()) );
        }
        else
        {
            NODABLE_ASSERT(false); // Node class not handled !
        }
    }

    return result;
}

std::string Serializer::serialize(const InstructionNode* _instruction ) const
{
    std::string result;

    auto value = _instruction->getValue();

    if ( value->hasInputConnected() )
    {
        // var declaration ?
        if ( auto variableNode = value->getInputMember()->getOwner()->as<VariableNode>() )
        {
            result.append( serialize( variableNode) );
        }
        else
        {
            result.append( serialize( value) );
        }
    }
    else
    {
        result.append( serialize( value) );
    }

    result.append( serialize(_instruction->getEndOfInstrToken() ));

    return result;
}

std::string Serializer::serialize(const Token* _token)const
{
    std::string result;

    if ( _token )
    {
        result.append( _token->m_prefix);
        result.append( serialize(_token->m_type));
        result.append( _token->m_suffix);
    }

    return result;
}

std::string Serializer::serialize(const ConditionalStructNode* _condStruct)const
{
    std::string result;

    // if ( <condition> )
    result.append( serialize(_condStruct->getTokenIf()));
    result.append( serialize(TokenType_OpenBracket));
    result.append( serialize(_condStruct->getCondition()));
    result.append( serialize(TokenType_CloseBracket));

    // if scope
    auto ifScope = _condStruct->getChildren()[0];
    result.append( this->serialize(ifScope->as<ScopedCodeBlockNode>()));

    // else & else scope
    if ( const Token* tokenElse = _condStruct->getTokenElse() )
    {
        result.append( serialize(tokenElse));
        auto elseScope = _condStruct->getChildren()[1];
        auto elseClass = elseScope->getClass();
        if ( elseClass == mirror::GetClass<ConditionalStructNode>())
        {
            result.append( this->serialize(elseScope->as<ConditionalStructNode>()));
        }
        else
        {
            result.append( this->serialize(elseScope->as<ScopedCodeBlockNode>()));
        }
    }

    return result;
}

std::string Serializer::serialize(const ScopedCodeBlockNode* _scope)const
{
    if ( _scope == nullptr )
        return "";

    NODABLE_ASSERT(_scope->getClass() == mirror::GetClass<ScopedCodeBlockNode>());

    std::string result;

    result.append( serialize(_scope->getBeginScopeToken() ) );

    for(auto eachChild : _scope->getChildren() )
    {
        if ( eachChild->getClass()->isChildOf(mirror::GetClass<CodeBlockNode>()) )
        {
            result.append( serialize(eachChild->as<CodeBlockNode>() ) );
        }
        else if ( eachChild->getClass()->isChildOf(mirror::GetClass<InstructionNode>()))
        {
            result.append( serialize(eachChild->as<InstructionNode>()) );
        }
        else if ( eachChild->getClass()->isChildOf(mirror::GetClass<ScopedCodeBlockNode>()))
        {
            result.append( serialize(eachChild->as<ScopedCodeBlockNode>()) );
        }
        else
        {
            NODABLE_ASSERT(false); // Node class not handled !
        }
    }

    result.append( serialize(_scope->getEndScopeToken()) );

    return result;
}
