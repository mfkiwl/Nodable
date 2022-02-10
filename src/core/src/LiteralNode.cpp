#include <nodable/LiteralNode.h>

using namespace Nodable;

REFLECT_DEFINE_CLASS(LiteralNode)

LiteralNode::LiteralNode(Reflect::Type type) : Node()
{
    m_props.add(Node::VALUE_MEMBER_NAME, Visibility::Always, type, Way_Out);
}
