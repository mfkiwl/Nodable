#pragma once

#include <memory> // std::shared_ptr

#include <fw/core/reflection/reflection>
#include "fw/core/assertions.h"
#include "fw/gui/ImGuiEx.h"

#include "Connector.h"
#include "types.h"

namespace ndbl {

    // forward declarations
    class PropertyView;
    class Property;
    class IAppCtx;

    /**
     * @brief A PropertyConnector represents a physical input or output on a PropertyView.
     */
    class PropertyConnector: public Connector<PropertyConnector>
    {
    public:

        enum class Side
        {
            Top,
            Bottom,
            Left,
            Right
        };

        PropertyView* m_property_view;
        Side          m_display_side;
        Way           m_way;

        PropertyConnector();
        PropertyConnector(PropertyView* _property, Way _way, Side _pos);
        ~PropertyConnector() = default;
        PropertyConnector (const PropertyConnector&) = delete;
        PropertyConnector& operator= (const PropertyConnector&) = delete;

        Property*          get_property()const;
        const fw::type*    get_property_type()const;
        ImVec2             get_pos() const override;
        bool               share_parent_with(const PropertyConnector* other)const override;
        bool               has_node_connected() const;
        static void        draw(PropertyConnector*, float _radius, const ImColor &_color, const ImColor &_borderColor, const ImColor &_hoverColor, bool _editable);
        static void        dropped(const PropertyConnector*, const PropertyConnector*);

        static PropertyConnector* s_hovered;
        static PropertyConnector* s_dragged;
        static PropertyConnector* s_focused;


    };
}