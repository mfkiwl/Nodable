#include "Physics.h"
#include "core/Node.h"
#include "core/math.h"
#include "NodeView.h"
#include "core/IConditionalStruct.h"
#include "core/ForLoopNode.h"

using namespace ndbl;

Physics::Physics(ndbl::NodeView *_view)
    : m_view(_view)
    , is_active(true)
{}

void Physics::clear_constraints()
{
    constraints.clear();
}

void Physics::add_constraint(NodeViewConstraint& _constraint)
{
    constraints.push_back(std::move(_constraint));
}

void Physics::apply_constraints(float _dt)
{
    if( !is_active ) return;

    for (NodeViewConstraint& eachConstraint : constraints)
    {
        eachConstraint.apply(_dt);
    }
}

void Physics::add_force_to_translate_to(ImVec2 desiredPos, float _factor, bool _recurse)
{
    ImVec2 delta(desiredPos - m_view->get_position());
    auto factor = std::max(0.0f, _factor);
    add_force(delta * factor, _recurse);
}

void Physics::add_force(ImVec2 force, bool _recurse)
{
    m_forces_sum += force;

    if ( !_recurse ) return;

    for ( NodeView* each_input_view: m_view->inputs )
    {
        if (!each_input_view->pinned && each_input_view->should_follow_output(m_view))
        {
            if(Physics* physics_component = each_input_view->get_owner()->components.get<Physics>())
            {
                physics_component->add_force(force, _recurse);
            }
        }
    }
}

void Physics::apply_forces(float _dt, bool _recurse)
{
    float magnitude = std::sqrt(m_forces_sum.x * m_forces_sum.x + m_forces_sum.y * m_forces_sum.y );

    constexpr float magnitude_max  = 1000.0f;
    const float     friction       = fw::math::lerp (0.0f, 0.5f, magnitude / magnitude_max);
    const ImVec2 avg_forces_sum      = (m_forces_sum + m_last_frame_forces_sum) * 0.5f;

    m_view->translate( avg_forces_sum * ( 1.0f - friction) * _dt , _recurse);

    m_last_frame_forces_sum = avg_forces_sum;
    m_forces_sum            = ImVec2();
}

void Physics::create_constraints(const std::vector<Node*>& nodes)
{
    LOG_VERBOSE(__FILE__, "create_constraints ...\n");
    for(Node* each_node: nodes)
    {
        auto each_view    = each_node->components.get<NodeView>();
        auto each_physics = each_node->components.get<Physics>();
        if ( each_view )
        {
            const fw::type* node_type = each_node->get_type();

            // Follow predecessor Node(s), except if first predecessor is a Conditional if/else
            //---------------------------------------------------------------------------------

            const std::vector<Node*>& predecessor_nodes = each_node->predecessors.content();
            std::vector<NodeView*> predecessor_views;
            Node::get_components<NodeView>(predecessor_nodes, predecessor_views);
            if (!predecessor_nodes.empty() && predecessor_nodes[0]->get_type()->is_not_child_of<IConditionalStruct>() )
            {
                NodeViewConstraint constraint("follow predecessor except if IConditionalStruct", ViewConstraint_t::FollowWithChildren);
                constraint.add_drivers(predecessor_views);
                constraint.add_target(each_view);
                each_physics->add_constraint(constraint);

                constraint.apply_when(NodeViewConstraint::always);
            }

            // Align in row Conditional Struct Node's children
            //------------------------------------------------

            std::vector<NodeView*>& children = each_view->children.content();
            if(!children.empty() && node_type->is_child_of<IConditionalStruct>() )
            {
                NodeViewConstraint constraint("align IConditionalStruct children", ViewConstraint_t::MakeRowAndAlignOnBBoxBottom);
                constraint.apply_when(NodeViewConstraint::drivers_are_expanded);
                constraint.add_driver(each_view);
                constraint.add_targets(children);

                if (node_type->is<ForLoopNode>() )
                {
                    constraint.add_targets(each_view->successors.content());
                }
                each_physics->add_constraint(constraint);
            }

            // Align in row Input connected Nodes
            //-----------------------------------

            if ( !each_view->inputs.empty() )
            {
                NodeViewConstraint constraint("align inputs", ViewConstraint_t::MakeRowAndAlignOnBBoxTop);
                constraint.add_driver(each_view);
                constraint.add_targets(each_view->inputs.content());
                each_physics->add_constraint(constraint);
                constraint.apply_when(NodeViewConstraint::always);
            }
        }
    }
    LOG_VERBOSE(__FILE__, "create_constraints OK\n");
}

void Physics::destroy_constraints(const std::vector<Node*>& nodes)
{
    LOG_VERBOSE(__FILE__, "destroy_constraints ...\n");
    std::vector<Physics*> physics_components;
    Node::get_components(nodes, physics_components);
    for(Physics* physics: physics_components)
    {
        physics->clear_constraints();
    }
    LOG_VERBOSE(__FILE__, "destroy_constraints OK\n");
}