#include "VirtualMachine.h"

#include <string>
#include "InvokableComponent.h"
#include "VariableNode.h"

using namespace ndbl;
using namespace tools;

static VirtualMachine* g_virtual_machine{ nullptr };

CPU::CPU()
{
    clear_registers();
}

void CPU::clear_registers()
{
    for(u8_t reg = 0; reg < Register_COUNT; ++reg )
    {
        write(reg, qword());
    }
}

qword CPU::read(Register _id)const
{
    ASSERT(_id < Register_COUNT)
    LOG_VERBOSE("VM::CPU", "read register %s (value: %s)\n", Register_to_string(_id), m_register[_id].to_string().c_str() )
    return m_register[_id];
}

qword& CPU::read_write(Register _id)
{
    ASSERT(_id < Register_COUNT)
    LOG_VERBOSE("VM::CPU", "read_write register %s (value: %s)\n", Register_to_string(_id), m_register[_id].to_string().c_str() )
    return m_register[_id];
}

void CPU::write(Register _id, qword _data)
{
    ASSERT(_id < Register_COUNT)
    qword& mem_dst = read_write(_id);
    mem_dst = _data;
    LOG_VERBOSE("VM::CPU", "write register %s (value: %s)\n", Register_to_string(_id), mem_dst.to_string().c_str())
}


VirtualMachine::VirtualMachine()
    : m_is_debugging(false)
    , m_is_program_running(false)
    , m_program_asm_code(nullptr)
{

}

void VirtualMachine::advance_cursor(i64_t _amount)
{
    qword eip = m_cpu.read(Register_eip);
    eip.u64 += _amount;
    m_cpu.write(Register_eip, eip );
}

void VirtualMachine::run_program()
{
    ASSERT(m_program_asm_code);
    LOG_MESSAGE("VM", "Running program ...\n")
    m_is_program_running = true;

    m_cpu.clear_registers();

    while( is_there_a_next_instr() && get_next_instr()->opcode != OpCode_ret )
    {
        _stepOver();
    }
    stop_program();
    LOG_MESSAGE("VM", "Program terminated\n")
}

void VirtualMachine::stop_program()
{
    if ( m_is_program_running )
    {
        m_is_program_running = false;
        m_is_debugging       = false;
        m_next_node          = {};
        LOG_MESSAGE("VM", "Program stopped\n")
    }
    else
    {
        LOG_ERROR("VM", "stop_program() failed, program is not running\n")
    }
}

const Code* VirtualMachine::release_program()
{
    if( m_is_program_running )
    {
        LOG_VERBOSE("VM", "stopping program before continue\n");
        stop_program();
    }

    m_cpu.clear_registers(); // will also clear reset instruction pointer (stored in a register Register_eip)
    LOG_VERBOSE("VM", "registers cleared\n")

    LOG_VERBOSE("VM", "program released\n")
    const Code* copy = m_program_asm_code;
    m_program_asm_code = nullptr;
    return copy;
}

bool VirtualMachine::_stepOver()
{
    bool success{false};
    Instruction* next_instr = get_next_instr();

    LOG_MESSAGE("VM", "%s\n", Instruction::to_string(*next_instr).c_str() );

    switch ( next_instr->opcode )
    {
        case OpCode_cmp:
        {
            qword left  = m_cpu.read(static_cast<Register>(next_instr->cmp.left.u8));  // dereference registers, get their value
            qword right = m_cpu.read(static_cast<Register>(next_instr->cmp.right.u8));
            qword result;
            result.b = left.b == right.b;
            m_cpu.write(Register_rax, result);       // boolean comparison
            advance_cursor();
            break;
        }

        case OpCode_deref_qword:
        {
            // TODO: code is WTH because of a lack of stack/heap

            const qword* qword = next_instr->uref.ptr;
            m_cpu.write(Register_rax, *qword );

            const type* ptr_type = next_instr->uref.type;
            if(ptr_type->is<bool>() )
            {
                LOG_VERBOSE("VM", "deref_qword bool: %b\n", qword->b);
            }
            else if(ptr_type->is<double>() )
            {
                LOG_VERBOSE("VM", "deref_qword double: %d\n", qword->d );
            }
            else if(ptr_type->is<i16_t>() )
            {
                LOG_VERBOSE("VM", "deref_qword i16_t: %i\n", qword->i16 );
            }
            else if(ptr_type->is<i32_t>() )
            {
                LOG_VERBOSE("VM", "deref_qword i32_t: %i\n", qword->i32 );
            }
            else if(ptr_type->is<std::string>() )
            {
                LOG_VERBOSE("VM", "deref_qword std::string* (%p): %s\n", qword->ptr, ((std::string*)qword)->c_str() );
            }
            else if(ptr_type->is<void *>() )
            {
                LOG_VERBOSE("VM", "deref_qword Node* (%p): %s\n", qword->ptr, ((Node*)qword->ptr)->name.c_str() );
            }
            else
            {
                EXPECT(false, "This type is not handled!")
            }

            advance_cursor();
            break;
        }

        case OpCode_mov:
        {
            // write( <destination_register>, <source_data>)
            m_cpu.write(next_instr->mov.dst.u8, next_instr->mov.src);

            advance_cursor();
            break;
        }

        case OpCode_push_var:
        {
            advance_cursor();
            VariableNode* variable = next_instr->push.var;
            ASSERT( !variable->has_flags(VariableFlag_INITIALIZED) )
            //
            // TODO: implement a stack/heap
            //
            break;
        }

        case OpCode_pop_var:
        {
            advance_cursor();
            VariableNode* variable = next_instr->push.var;
            if(variable->has_flags(VariableFlag_INITIALIZED) ) // Might not have been initialized, check needed.
                variable->clear_flags(VariableFlag_INITIALIZED);
            //
            // TODO: implement a stack/heap
            //
            break;
        }

        case OpCode_push_stack_frame: // ensure variable declared in this scope are unitialized before/after scope
        case OpCode_pop_stack_frame:
        {
            advance_cursor();
            //
            // not implemented, currently we use VariableNode's data instead of a dedicated stack/heap.
            //
            break;
        }

        case OpCode_eval_node:
        {
            auto update_input__by_value_only = [](Node* _node)
            {
                for(Slot* slot: _node->filter_slots( SlotFlag_INPUT ) )
                {
                    if( slot->adjacent_count() == 0)
                        continue;

                    Property* property = slot->get_property();
                    if( !property->is_ref() )
                    {
                        *property->value() = *slot->first_adjacent()->get_property()->value();
                    }
                }
            };

            if( auto variable = cast<VariableNode>(next_instr->eval.node))
            {
                if( !variable->has_flags(VariableFlag_INITIALIZED) )
                {
                    variable->set_flags(VariableFlag_INITIALIZED);
                    update_input__by_value_only(variable);
                }
            }
            else
            {
                update_input__by_value_only(next_instr->eval.node);
            }

            // evaluate Invokable Component, could be an operator or a function
            auto* invokable = next_instr->eval.node->get_component<InvokableComponent>();
            if( invokable != nullptr )
            {
                ASSERT(invokable->has_flags(InvokableFlag_WAS_EVALUATED) == false ) // flag should have been reset
                invokable->invoke();
            }

            next_instr->eval.node->dirty = false;
            advance_cursor();
            break;
        }

        case OpCode_jmp:
        {
            advance_cursor(next_instr->jmp.offset);
            break;
        }

        case OpCode_jne:
        {
            qword rax = m_cpu.read(Register_rax);
            i64_t offset{1};
            if ( rax.b ) // last comparison result is stored in rax
                offset = next_instr->jmp.offset; // jump if NOT equal
            advance_cursor( offset );
            break;
        }

        case OpCode_ret:
            //advance_cursor();
            success = false;
            break;

        default:
            advance_cursor();
            success = false;
    }

    return success;
}

bool VirtualMachine::step_over()
{
    auto must_break = [&]() -> bool {
        return
                get_next_instr()->opcode == OpCode_eval_node
               && m_last_step_next_instr != get_next_instr();
    };

    bool must_exit = false;

    while(is_there_a_next_instr() && !must_break() && !must_exit )
    {
        must_exit = get_next_instr()->opcode == OpCode_ret;
        _stepOver();
    }


    bool continue_execution = is_there_a_next_instr() && !must_exit;
    if( !continue_execution )
    {
        stop_program();
        m_next_node            = {};
        m_last_step_next_instr = {};
    }
    else
    {
        // update m_current_node and m_last_step_instr
        m_last_step_next_instr = get_next_instr();
        auto next_instr = get_next_instr();

        switch ( next_instr->opcode )
        {
            case OpCode_eval_node:
            {
                m_next_node = next_instr->eval.node;
                break;
            }

            default:
                break;
        }
        LOG_MESSAGE("VM", "Step over (current line %#1llx)\n", next_instr->line)
    }

    return continue_execution;
}

void VirtualMachine::debug_program()
{
    ASSERT(m_program_asm_code);
    m_is_debugging = true;
    m_is_program_running = true;
    m_cpu.clear_registers();
    m_next_node = m_program_asm_code->get_meta_data().root_node;
    LOG_MESSAGE("VM", "Debugging program ...\n")
}

bool VirtualMachine::is_there_a_next_instr() const
{
    const qword& eip = m_cpu.read(Register_eip);
    return eip.u64 < m_program_asm_code->size();
}

qword VirtualMachine::get_last_result()const
{
    return m_cpu.read(Register_rax);
}

Instruction* VirtualMachine::get_next_instr() const
{
    if ( is_there_a_next_instr() )
    {
        return m_program_asm_code->get_instruction_at(m_cpu.read(Register_eip).u32);
    }
    return nullptr;
}

bool VirtualMachine::load_program(const Code *_code)
{
    ASSERT(!m_is_program_running)   // dev must stop before to load program.
    ASSERT(!m_program_asm_code)     // dev must unload before to load.

    m_program_asm_code = _code;

    return m_program_asm_code && m_program_asm_code->size() != 0;
}

qword VirtualMachine::read_cpu_register(Register _register)const
{
    return m_cpu.read(_register);
}

const Code *VirtualMachine::get_program_asm_code()
{
    return m_program_asm_code;
}

VirtualMachine* ndbl::get_virtual_machine()
{
    return g_virtual_machine;
}

VirtualMachine* ndbl::init_virtual_machine()
{
    ASSERT(g_virtual_machine == nullptr) // singleton
    g_virtual_machine = new VirtualMachine();
    return g_virtual_machine;
}

void ndbl::shutdown_virtual_machine(VirtualMachine* _virtual_machine)
{
    ASSERT(g_virtual_machine == _virtual_machine) // singleton
    ASSERT(g_virtual_machine != nullptr)
    g_virtual_machine->release_program();
    delete g_virtual_machine;
    g_virtual_machine = nullptr;
}

