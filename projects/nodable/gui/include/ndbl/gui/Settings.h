#pragma once

#include <string>
#include <vector>
#include <memory>

#include <fw/gui/types.h>
#include <fw/gui/FontConf.h>
#include <fw/gui/FontSlot.h>
#include <fw/gui/AppView.h>
#include <fw/core/reflection/reflection>

#include <ImGuiColorTextEdit/TextEditor.h>
#include <ndbl/gui/types.h>

namespace ndbl {

    class Settings  {
    public:

        Settings();

        TextEditor::Palette ui_text_textEditorPalette;
        float          ui_wire_bezier_roundness;
        float          ui_wire_bezier_thickness;
        bool           ui_wire_displayArrows;
        fw::vec4       ui_wire_fillColor;
        fw::vec4       ui_wire_shadowColor;
        float          ui_node_propertyConnectorRadius;
        float          ui_node_padding;
        fw::vec4       ui_node_variableColor;
        fw::vec4       ui_node_invokableColor;
        fw::vec4       ui_node_instructionColor;
        fw::vec4       ui_node_literalColor;
        fw::vec4       ui_node_shadowColor;
        fw::vec4       ui_node_borderHighlightedColor;
        fw::vec4       ui_node_borderColor;
        fw::vec4       ui_node_highlightedColor;
        fw::vec4       ui_node_fillColor;
        fw::vec4       ui_node_nodeConnectorColor;
        fw::vec4       ui_node_nodeConnectorHoveredColor;
        float          ui_node_spacing;
        float          ui_node_speed;
        u8_t           ui_node_animation_subsample_count;
        float          ui_node_connector_height;
        float          ui_node_connector_padding;
        float          ui_node_connector_width;
        fw::vec4       ui_codeFlow_lineColor;
        fw::vec4       ui_codeFlow_lineShadowColor;
        fw::vec2       ui_toolButton_size;
        float          ui_history_btn_spacing;
        float          ui_history_btn_height;
        float          ui_history_btn_width_max;
        const char*    ui_splashscreen_imagePath;
        float          ui_overlay_margin;
        float          ui_overlay_indent;
        fw::vec4       ui_overlay_window_bg_golor;
        fw::vec4       ui_overlay_border_color;
        fw::vec4       ui_overlay_text_color;
        const char*    ui_file_info_window_label;
        const char*    ui_help_window_label;
        const char*    ui_imgui_settings_window_label;
        const char*    ui_node_properties_window_label;
        const char*    ui_settings_window_label;
        const char*    ui_startup_window_label;
        const char*    ui_toolbar_window_label ;
        const char*    ui_virtual_machine_window_label;
        bool           experimental_graph_autocompletion;
        bool           experimental_hybrid_history;
        bool           isolate_selection;
        fw::AppView::Conf fw_app_view;

        static Settings& get_instance();                  // Get the shared settings
    };
}
