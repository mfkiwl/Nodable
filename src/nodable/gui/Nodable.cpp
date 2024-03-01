#include "Nodable.h"

#include "Action.h"
#include "Condition.h"
#include "Event.h"
#include "GraphView.h"
#include "HybridFile.h"
#include "HybridFileView.h"
#include "NodableView.h"
#include "NodeView.h"
#include "Physics.h"
#include "SlotView.h"
#include "commands/Cmd_ConnectEdge.h"
#include "commands/Cmd_DisconnectEdge.h"
#include "commands/Cmd_Group.h"
#include "core/DataAccess.h"
#include "core/InvokableComponent.h"
#include "core/LiteralNode.h"
#include "core/NodeUtils.h"
#include "core/Slot.h"
#include "core/VariableNode.h"
#include "fw/core/assertions.h"
#include "fw/core/system.h"
#include "fw/gui/EventManager.h"
#include <algorithm>

using namespace ndbl;
using namespace fw;
using fw::View;

Nodable *Nodable::s_instance = nullptr;

template<typename T>
static fw::func_type* create_variable_node_signature()
{ return fw::func_type_builder<T(T)>::with_id("variable"); }

template<typename T>
static fw::func_type* create_literal_node_signature()
{ return fw::func_type_builder<T(/*void*/)>::with_id("literal"); }

Nodable::Nodable()
    : App(config.common, new NodableView(this) )
    , current_file(nullptr)
    , virtual_machine()
{
    LOG_VERBOSE("ndbl::App", "Constructor ...\n");

    fw::type_register::log_statistics();

    // set this instance as s_instance to access it via App::get_instance()
    FW_EXPECT(s_instance == nullptr, "Can't create two concurrent App. Delete first instance.");
    s_instance = this;

    // Bind methods to framework events
    LOG_VERBOSE("ndbl::App", "Binding framework ...\n");

    node_factory.override_post_process_fct( [&]( PoolID<Node> node ) -> void {
        // Code executed after node instantiation

        // add a view with physics
        auto *pool = Pool::get_pool();
        PoolID<NodeView> new_view_id = pool->create<NodeView>();
        PoolID<Physics> physics_id = pool->create<Physics>( new_view_id );
        node->add_component( new_view_id );
        node->add_component( physics_id );

        // Set fill_color
        ImVec4* fill_color;
        if ( fw::extends<VariableNode>( node.get() ) )
        {
            fill_color = &config.ui_node_variableColor;
        }
        else if ( node->has_component<InvokableComponent>() )
        {
            fill_color = &config.ui_node_invokableColor;
        }
        else if ( node->is_instruction() )
        {
            fill_color = &config.ui_node_instructionColor;
        }
        else if ( fw::extends<LiteralNode>( node.get() ) )
        {
            fill_color = &config.ui_node_literalColor;
        }
        else if ( fw::extends<IConditional>( node.get() ) )
        {
            fill_color = &config.ui_node_condStructColor;
        }
        else
        {
            fill_color = &config.ui_node_fillColor;
        }
        new_view_id->set_color( fill_color );
    } );

    LOG_VERBOSE("ndbl::App", "Constructor " OK "\n");
}

Nodable::~Nodable()
{
    delete m_view;
    s_instance = nullptr;
    LOG_VERBOSE("ndbl::App", "Destructor " OK "\n");
}

bool Nodable::on_init()
{
    LOG_VERBOSE("ndbl::App", "on_init ...\n");

    fw::Pool::init();

    // Bind commands to shortcuts
    using fw::EventID;
    event_manager.register_action<NodeAction>("Delete", { SDLK_DELETE, KMOD_NONE }, { NodeActionType::DELETE } );
    event_manager.register_action<NodeAction>("Arrange", { SDLK_a, KMOD_NONE }, { NodeActionType::ARRANGE } /* Condition_ENABLE_IF_HAS_SELECTION | Condition_HIGHLIGHTED_IN_GRAPH_EDITOR */ );
    event_manager.register_action<NodeAction>("Fold", { SDLK_x, KMOD_NONE }, { NodeActionType::FOLD } /* Condition_ENABLE_IF_HAS_SELECTION | Condition_HIGHLIGHTED_IN_GRAPH_EDITOR }*/ );
    event_manager.register_action(
            { "Next",
              EventID_REQUEST_SELECT_SUCCESSOR,
              { SDLK_n, KMOD_NONE },
              Condition_ENABLE } );
    event_manager.register_action(
            { ICON_FA_SAVE " Save",
              fw::EventID_REQUEST_FILE_SAVE,
              { SDLK_s, KMOD_CTRL },
              Condition_ENABLE } );
    event_manager.register_action(
            { ICON_FA_SAVE " Save as",
              fw::EventID_REQUEST_FILE_SAVE_AS,
              { SDLK_s, KMOD_CTRL },
              Condition_ENABLE } );
    event_manager.register_action(
            { ICON_FA_TIMES "  Close",
              fw::EventID_REQUEST_FILE_CLOSE,
              { SDLK_w, KMOD_CTRL },
              Condition_ENABLE } );
    event_manager.register_action(
            { ICON_FA_FOLDER_OPEN " Open",
              fw::EventID_REQUEST_FILE_BROWSE,
              { SDLK_o, KMOD_CTRL },
              Condition_ENABLE } );
    event_manager.register_action(
            { ICON_FA_FILE " New",
              fw::EventID_REQUEST_FILE_NEW,
              { SDLK_n, KMOD_CTRL },
              Condition_ENABLE } );
    event_manager.register_action(
            { "Splashscreen",
              fw::EventID_REQUEST_SHOW_SLASHSCREEN,
              { SDLK_F1 },
              Condition_ENABLE } );
    event_manager.register_action(
            { ICON_FA_SIGN_OUT_ALT " Exit",
              fw::EventID_REQUEST_EXIT,
              { SDLK_F4, KMOD_ALT },
              Condition_ENABLE } );
    event_manager.register_action(
            { "Undo",
              fw::EventID_REQUEST_UNDO,
              { SDLK_z, KMOD_CTRL },
              Condition_ENABLE } );
    event_manager.register_action(
            { "Redo",
              fw::EventID_REQUEST_REDO,
              { SDLK_y, KMOD_CTRL },
              Condition_ENABLE } );
    event_manager.register_action(
            { "Isolate",
              EventID_REQUEST_TOGGLE_ISOLATE_SELECTION,
              { SDLK_i, KMOD_CTRL },
              Condition_ENABLE | Condition_HIGHLIGHTED_IN_TEXT_EDITOR } );
    event_manager.register_action(
            { "Deselect",
              fw::EventID_NONE,
              { 0, KMOD_NONE, "Double click on bg" },
              Condition_ENABLE_IF_HAS_SELECTION | Condition_HIGHLIGHTED_IN_GRAPH_EDITOR } );
    event_manager.register_action(
            { "Move Graph",
              fw::EventID_NONE,
              { 0, KMOD_NONE, "Drag background" },
              Condition_ENABLE | Condition_HIGHLIGHTED_IN_GRAPH_EDITOR } );
    event_manager.register_action(
            { "Frame Selection",
              EventID_REQUEST_FRAME_SELECTION,
              { SDLK_f, KMOD_NONE },
              Condition_ENABLE_IF_HAS_SELECTION | Condition_HIGHLIGHTED_IN_GRAPH_EDITOR } );
    event_manager.register_action(
            { "Frame All",
              EventID_REQUEST_FRAME_ALL,
              { SDLK_f, KMOD_LCTRL },
              Condition_ENABLE } );

    // Prepare context menu items
    {
        // 1) Blocks
        event_manager.register_action<CreateBlockAction>( ICON_FA_CODE " Condition", {}, NodeType_BLOCK_CONDITION );
        event_manager.register_action<CreateBlockAction>( ICON_FA_CODE " For Loop", {}, NodeType_BLOCK_FOR_LOOP );
        event_manager.register_action<CreateBlockAction>( ICON_FA_CODE " While Loop", {}, NodeType_BLOCK_WHILE_LOOP );
        event_manager.register_action<CreateBlockAction>( ICON_FA_CODE " Scope", {}, NodeType_BLOCK_SCOPE );
        event_manager.register_action<CreateBlockAction>( ICON_FA_CODE " Program", {}, NodeType_BLOCK_PROGRAM );

        // 2) Variables
        event_manager.register_action<CreateNodeAction>( ICON_FA_DATABASE " Boolean Variable", {}, { NodeType_VARIABLE_BOOLEAN, create_variable_node_signature<bool>() } );
        event_manager.register_action<CreateNodeAction>( ICON_FA_DATABASE " Double Variable", {}, { NodeType_VARIABLE_DOUBLE, create_variable_node_signature<double>() } );
        event_manager.register_action<CreateNodeAction>( ICON_FA_DATABASE " Integer Variable", {}, { NodeType_VARIABLE_INTEGER, create_variable_node_signature<int>() } );
        event_manager.register_action<CreateNodeAction>( ICON_FA_DATABASE " String Variable", {}, { NodeType_VARIABLE_STRING, create_variable_node_signature<std::string>() } );

        // 3) Literals
        event_manager.register_action<CreateNodeAction>( ICON_FA_FILE " Boolean Literal", {}, { NodeType_LITERAL_BOOLEAN, create_variable_node_signature<bool>() } );
        event_manager.register_action<CreateNodeAction>( ICON_FA_FILE " Double Literal", {}, { NodeType_LITERAL_DOUBLE, create_variable_node_signature<double>() } );
        event_manager.register_action<CreateNodeAction>( ICON_FA_FILE " Integer Literal", {}, { NodeType_LITERAL_INTEGER, create_variable_node_signature<int>() } );
        event_manager.register_action<CreateNodeAction>( ICON_FA_FILE " String Literal", {}, { NodeType_LITERAL_STRING, create_variable_node_signature<std::string>() } );

        // 4) Functions/Operators from the API
        const Nodlang& language = Nodlang::get_instance();
        for ( auto& each_fct: language.get_api() )
        {
            const fw::func_type* func_type = each_fct->get_type();
            std::string label;
            language.serialize_func_sig( label, func_type );
            event_manager.register_action<CreateNodeAction>( label.c_str(), {}, { NodeType_INVOKABLE, func_type } );
        }
    }
    return true;
}

void Nodable::on_update()
{
    LOG_VERBOSE("ndbl::App", "on_update ...\n");

    // 1. Update current file
    if (current_file && !virtual_machine.is_program_running())
    {
        //
        // When history is dirty we update the graph from the text.
        // (By default undo/redo are text-based only, if hybrid_history is ON, the behavior is different
        if ( current_file->get_history()->is_dirty() && !config.experimental_hybrid_history )
        {
            current_file->update_graph_from_text(config.isolate_selection);
            current_file->get_history()->set_dirty(false);
        }
        // Run the main update loop for the file
        current_file->update();
    }

    // 2. Handle events

    // shorthand to push all shortcuts to a file view overlay depending on conditions
    auto push_overlay_shortcuts = [&](ndbl::HybridFileView& _view, Condition _condition) -> void {
        for (const auto& _action: event_manager.get_actions())
        {
            if( ( _action.condition & _condition) == _condition)
            {
                if ( _action.condition & Condition_HIGHLIGHTED_IN_GRAPH_EDITOR)
                {
                    _view.push_overlay(
                        { _action.label.substr(0, 12),
                              _action.shortcut.to_string()
                        }, OverlayType_GRAPH);
                }
                if ( _action.condition & Condition_HIGHLIGHTED_IN_TEXT_EDITOR)
                {
                    _view.push_overlay(
                        { _action.label.substr(0,12),
                              _action.shortcut.to_string()
                        }, OverlayType_TEXT);
                }
            }

        }
    };

    // Nodable events
    auto       selected_view       = NodeView::get_selected();
    GraphView* graph_view          = current_file ? current_file->get_graph_view() : nullptr;
    History*   curr_file_history   = current_file ? current_file->get_history() : nullptr;

    Event* event = nullptr;
    while( (event = event_manager.poll_event()) && event != nullptr )
    {
        switch ( event->id )
        {
            case EventID_REQUEST_TOGGLE_ISOLATE_SELECTION:
            {
                config.isolate_selection = !config.isolate_selection;
                if(current_file)
                {
                    current_file->update_graph_from_text(config.isolate_selection);
                }
                break;
            }

            case fw::EventID_REQUEST_EXIT:
            {
                should_stop = true;
                break;
            }

            case fw::EventID_REQUEST_FILE_CLOSE:
            {
                if(current_file) close_file(current_file);
                break;
            }
            case fw::EventID_REQUEST_UNDO:
            {
                if(curr_file_history) curr_file_history->undo();
                break;
            }

            case fw::EventID_REQUEST_REDO:
            {
                if(curr_file_history) curr_file_history->redo();
                break;
            }

            case fw::EventID_REQUEST_FILE_BROWSE:
            {
                std::string path;
                if( m_view->pick_file_path(path, fw::AppView::DIALOG_Browse))
                {
                    open_file(path);
                    break;
                }
                LOG_MESSAGE("App", "Browse file aborted by user.\n");
                break;

            }

            case fw::EventID_REQUEST_FILE_NEW:
            {
                new_file();
                break;
            }

            case fw::EventID_REQUEST_FILE_SAVE_AS:
            {
                if (current_file)
                {
                    std::string path;
                    if( m_view->pick_file_path(path, fw::AppView::DIALOG_SaveAs))
                    {
                        save_file_as(path);
                        break;
                    }
                }
                break;
            }

            case fw::EventID_REQUEST_FILE_SAVE:
            {
                if (current_file)
                {
                    if( !current_file->path.empty())
                    {
                        save_file(current_file);
                    }
                    else
                    {
                        std::string path;
                        if( m_view->pick_file_path(path, fw::AppView::DIALOG_SaveAs))
                        {
                            save_file_as(path);
                        }
                    }
                }
                break;
            }

            case fw::EventID_REQUEST_SHOW_SLASHSCREEN:
            {
                config.common.splashscreen = true;
                break;
            }
             case EventID_REQUEST_FRAME_SELECTION:
            {
                if (graph_view) graph_view->frame_selected_node_views();
                break;
            }

            case EventID_REQUEST_FRAME_ALL:
            {
                if (graph_view) graph_view->frame_all_node_views();
                break;
            }
            case EventID_NODE_SELECTION_CHANGE:
            {
                auto _event = dynamic_cast<NodeViewSelectionChangeEvent*>( event );
                current_file->view.clear_overlay();
                Condition_ condition = _event->payload.new_selection ? Condition_ENABLE_IF_HAS_SELECTION
                                                                       : Condition_ENABLE_IF_HAS_NO_SELECTION;
                push_overlay_shortcuts(current_file->view, condition );
                break;
            }
            case fw::EventID_FILE_OPENED:
            {
                if (!current_file) break;
                current_file->view.clear_overlay();
                push_overlay_shortcuts(current_file->view, Condition_ENABLE_IF_HAS_NO_SELECTION);
                break;
            }
            case EventID_REQUEST_DELETE_NODE:
            {
                if ( selected_view && !ImGui::IsAnyItemFocused() )
                {
                    Node* selected_node = selected_view->get_owner().get();
                    selected_node->flagged_to_delete = true;
                }
                break;
            }

            case EventID_REQUEST_ARRANGE_HIERARCHY:
            {
                if ( selected_view ) selected_view->arrange_recursively();
                break;
            }

            case EventID_REQUEST_SELECT_SUCCESSOR:
            {
                if (!selected_view) break;
                std::vector<PoolID<Node>> successors = selected_view->get_owner()->successors();
                if (!successors.empty())
                {
                    if (PoolID<NodeView> successor_view = successors.front()->get_component<NodeView>() )
                    {
                        NodeView::set_selected(successor_view);
                    }
                }
                break;
            }
            case EventID_REQUEST_TOGGLE_FOLDING:
            {
                if ( !selected_view ) break;
                auto _event = dynamic_cast<ToggleFoldingEvent*>(event);
                _event->payload.recursive ? selected_view->expand_toggle_rec()
                                               : selected_view->expand_toggle();
                break;
            }

            case EventID_SLOT_DROPPED:
            {
                auto _event = dynamic_cast<SlotDroppedEvent*>(event);
                SlotRef tail = _event->payload.first;
                SlotRef head = _event->payload.second;
                if (head.flags & SlotFlag_ORDER_SECOND ) std::swap(tail, head); // guarantee src to be the output
                DirectedEdge edge(tail, head);
                auto cmd = std::make_shared<Cmd_ConnectEdge>(edge);
                curr_file_history->push_command(cmd);

                break;
            }

            case EventID_SLOT_DISCONNECTED:
            {
                auto _event = dynamic_cast<SlotDisconnectedEvent*>(event);
                SlotRef slot = _event->payload.first;

                auto cmd_grp = std::make_shared<Cmd_Group>("Disconnect All Edges");
                for( const auto& adjacent_slot: slot->adjacent() )
                {
                    DirectedEdge edge{slot, adjacent_slot};
                    auto each_cmd = std::make_shared<Cmd_DisconnectEdge>(edge);
                    cmd_grp->push_cmd( std::static_pointer_cast<AbstractCommand>(each_cmd) );
                }
                curr_file_history->push_command(std::static_pointer_cast<AbstractCommand>(cmd_grp));

                break;
            }

            case EventID_REQUEST_CREATE_NODE:
            {
                auto _event = dynamic_cast<CreateNodeAction*>(event);

                // 1) create the node
                PoolID<Node> new_node_id  = current_file->get_graph()->create_node( _event->payload.node_type, _event->payload.node_signature );

                // 2) handle connections
                if ( !_event->payload.dragged_slot )
                {
                    // Experimental: we try to connect a parent-less child
                    if ( new_node_id != _event->payload.graph->get_root() && config.experimental_graph_autocompletion )
                    {
                        _event->payload.graph->ensure_has_root();
                        // m_graph->connect( new_node, m_graph->get_root(), RelType::CHILD  );
                    }
                }
                else
                {
                    Slot* complementary_slot = new_node_id->find_slot_by_property_type(
                            get_complementary_flags( _event->payload.dragged_slot->slot().static_flags() ),
                            _event->payload.dragged_slot->get_property()->get_type() );

                    if ( !complementary_slot )
                    {
                        // TODO: this case should not happens, instead we should check ahead of time whether or not this not can be attached
                        LOG_ERROR( "GraphView", "unable to connect this node" )
                    }
                    else
                    {
                        Slot* out = &_event->payload.dragged_slot->slot();
                        Slot* in = complementary_slot;

                        if ( out->has_flags( SlotFlag_ORDER_SECOND ) ) std::swap( out, in );

                        _event->payload.graph->connect( *out, *in, ConnectFlag_ALLOW_SIDE_EFFECTS );
                    }
                }

                // set new_node's view position, select it
                if ( auto view = new_node_id->get_component<NodeView>() )
                {
                    view->set_position( _event->payload.node_view_local_pos, fw::Space_Local );
                    NodeView::set_selected( view );
                }
                break;
            }

            default:
            {
                LOG_VERBOSE("App", "Ignoring and event, this case is not handled\n")
            }
        }
    }
    LOG_VERBOSE("ndbl::App", "on_update " OK "\n");
}

bool Nodable::on_shutdown()
{
    LOG_VERBOSE("ndbl::App", "on_shutdown ...\n");
    for( HybridFile* each_file : m_loaded_files )
    {
        LOG_VERBOSE("ndbl::App", "Delete file %s ...\n", each_file->path.c_str())
        delete each_file;
    }
    LOG_VERBOSE("ndbl::App", "on_shutdown " OK "\n");

    fw::Pool::shutdown();

    return true;
}

HybridFile *Nodable::open_file(const ghc::filesystem::path& _path)
{
    auto file = new HybridFile(fw::App::asset_path(_path) );

    if ( !file->load() )
    {
        LOG_ERROR("File", "Unable to open file %s (%s)\n", _path.filename().c_str(), _path.c_str());
        delete file;
        return nullptr;
    }
    add_file(file);
    file->update_graph_from_text(config.isolate_selection);
    return file;
}

HybridFile *Nodable::add_file(HybridFile* _file)
{
    FW_EXPECT(_file, "File is nullptr");
    m_loaded_files.push_back( _file );
    current_file = _file;
    event_manager.dispatch( fw::EventID_FILE_OPENED );
    return _file;
}

void Nodable::save_file(HybridFile* _file) const
{
    FW_EXPECT(_file,"file must be defined");

	if ( !_file->write_to_disk() )
    {
        LOG_ERROR("ndbl::App", "Unable to save %s (%s)\n", _file->name.c_str(), _file->path.c_str());
        return;
    }
    LOG_MESSAGE("ndbl::App", "File saved: %s\n", _file->path.c_str());
}

void Nodable::save_file_as(const ghc::filesystem::path& _path) const
{
    ghc::filesystem::path absolute_path = fw::App::asset_path(_path);
    current_file->path = absolute_path.string();
    current_file->name = absolute_path.filename().string();
    if( !current_file->write_to_disk() )
    {
        LOG_ERROR("App", "Unable to save as %s (%s)\n", absolute_path.filename().c_str(), absolute_path.c_str());
    }
}

void Nodable::close_file(HybridFile* _file)
{
    // Find and delete the file
    FW_EXPECT(_file, "Cannot close a nullptr File!");
    auto it = std::find(m_loaded_files.begin(), m_loaded_files.end(), _file);
    FW_EXPECT(it != m_loaded_files.end(), "Unable to find the file in the loaded_files");
    it = m_loaded_files.erase(it);
    delete _file;

    // Switch to the next file if possible
    if ( it != m_loaded_files.end() )
    {
        current_file = *it;
    }
    else
    {
        current_file = nullptr;
    }
}

bool Nodable::compile_and_load_program()
{
    if ( current_file )
    {
        const Graph* graph = current_file->get_graph();

        if (graph)
        {
            assembly::Compiler compiler;
            auto asm_code = compiler.compile_syntax_tree(graph);

            if (asm_code)
            {
                virtual_machine.release_program();

                if (virtual_machine.load_program(std::move(asm_code)))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void Nodable::run_program()
{
    if (compile_and_load_program() )
    {
        virtual_machine.run_program();
    }
}

void Nodable::debug_program()
{
    if (compile_and_load_program() )
    {
        virtual_machine.debug_program();
    }
}

void Nodable::step_over_program()
{
    virtual_machine.step_over();
    if (!virtual_machine.is_there_a_next_instr() )
    {
        NodeView::set_selected({});
        return;
    }

    const Node* next_node = virtual_machine.get_next_node();
    if ( !next_node ) return;

    if( PoolID<NodeView> next_node_view = next_node->get_component<NodeView>() )
    {
        NodeView::set_selected( next_node_view );
    }
}

void Nodable::stop_program()
{
    virtual_machine.stop_program();
}

void Nodable::reset_program()
{
    if(!current_file) return;

    if (virtual_machine.is_program_running() )
    {
        virtual_machine.stop_program();
    }
    current_file->update_graph_from_text(config.isolate_selection);
}

HybridFile *Nodable::new_file()
{
    m_untitled_file_count++;
    std::string name{"Untitled_"};
    name.append(std::to_string(m_untitled_file_count));
    name.append(".cpp");

    HybridFile* file = new HybridFile(ghc::filesystem::path{name});
    // file->set_text( "// " + name);

    return add_file(file);
}

Nodable &Nodable::get_instance()
{
    FW_EXPECT(s_instance, "No App instance available. Did you forget App app(...) or App* app = new App(...)");
    return *s_instance;
}