#pragma once

#include <string>
#include <vector>
#include <unordered_set>

#include <nodable/core/types.h>
#include <nodable/core/reflection/R.h>
#include <nodable/core/Variant.h>

namespace Nodable
{
    // forward declarations
    class Node;
    class Scope;
    class Member;
    class VariableNode;
    class InstructionNode;
    class ConditionalStructNode;
    class ForLoopNode;

    namespace Asm // Assembly namespace
    {
        /**
         * Enum to identify each register, we try here to follow the x86_64 DASM reference from
         * @see https://www.cs.uaf.edu/2017/fall/cs301/reference/x86_64.html
         */
        enum Register : size_t {
            undefined,
            rax,     // accumulator
            rdx,     // storage
            eip,     // The instruction pointer.
            COUNT
        };

        R_ENUM(Register)
        R_ENUM_VALUE(undefined)
        R_ENUM_VALUE(rax)
        R_ENUM_VALUE(rdx)
        R_ENUM_VALUE(eip)
        R_ENUM_END

        /**
         * Enumerate each possible instruction.
         */
        enum class Instr_t: u8
        {
            cmp, /* compare */
            eval_node,
            jmp,
            jne,
            mov,
            pop_stack_frame,
            pop_var,
            push_stack_frame,
            push_var,
            ret
        };

        R_ENUM(Instr_t)
        R_ENUM_VALUE(mov)
        R_ENUM_VALUE(eval_node)
        R_ENUM_VALUE(push_var)
        R_ENUM_VALUE(pop_var)
        R_ENUM_VALUE(push_stack_frame)
        R_ENUM_VALUE(pop_stack_frame)
        R_ENUM_VALUE(jmp)
        R_ENUM_VALUE(jne)
        R_ENUM_VALUE(ret)
        R_ENUM_VALUE(cmp)
        R_ENUM_END

        /**
         * Simple structure to store (data, type) couple.
         */
        struct MemSpace
        {
            enum class Type: size_t
            {
                Undefined = 0,
                Boolean,
                Double,
                U64,
                VariantPtr,
                Register
            };

            struct Data
            {
                union {
                    bool     m_bool;
                    double   m_double;
                    u64      m_u64;
                    Variant *m_variant;
                    Register m_register;
                };
            };

            MemSpace(Type _type = Type::Undefined):type(_type)
            {
                data.m_u64 = 0;
            }

            void reset()
            {
                data.m_u64 = 0;
                type = Type::Undefined;
            }

            std::string to_string()const { return MemSpace::to_string(*this); }

            Data data;
            Type type;

            static std::string to_string(const MemSpace&);
            static_assert(sizeof(MemSpace::data) == sizeof(size_t));
        };
        static_assert(sizeof(MemSpace) == 2 * sizeof(size_t));

        R_ENUM(MemSpace::Type)
        R_ENUM_VALUE(Undefined)
        R_ENUM_VALUE(Boolean)
        R_ENUM_VALUE(Double)
        R_ENUM_VALUE(U64)
        R_ENUM_VALUE(VariantPtr)
        R_ENUM_VALUE(Register)
        R_ENUM_END



        /**
         * Store a single assembly instruction ( line type larg rarg comment )
         */
        struct Instr
        {
            Instr(Instr_t _type, long _line)
                : type(_type)
                , line(_line)
            {}

            u64         line;
            Instr_t     type;
            union {

                struct {
                    i64 offset;
                } jmp;

                struct {
                    MemSpace dst;
                    MemSpace src;
                } mov;

                struct {
                    MemSpace left;
                    MemSpace right;
                } cmp; // compare

                struct {
                    union {
                        const VariableNode* variable;
                        const Scope*  scope;
                    };
                } push;

                struct {
                    union {
                        const VariableNode* var;
                        const Scope*  scope;
                    };
                } pop;

                struct {
                    const Node* node;
                } eval;
            };
            std::string m_comment;
            static std::string to_string(const Instr&);
        };

        /**
         * Wraps an Instruction vector, plus some shortcuts.
         */
        class Code
        {
        public:
            struct MetaData {
                Node* root_node;
            };

            Code(Node* _root): m_meta_data({_root}) {};
            ~Code();

            Instr*                     push_instr(Instr_t _type);
            inline size_t              size() const { return  m_instructions.size(); }
            inline Instr*              get_instruction_at(size_t _index) const { return  m_instructions.at(_index); }
            size_t                     get_next_index() const { return m_instructions.size(); }
            const std::vector<Instr*>& get_instructions()const { return m_instructions; }
            const MetaData&            get_meta_data()const { return m_meta_data; }
            static std::string         to_string(const Code*);
        private:
            MetaData            m_meta_data;
            std::vector<Instr*> m_instructions;
        };

        /**
         * @brief Class to convert a program graph to Assembly::Code
         */
        class Compiler
        {
        public:
            Compiler()= default;
            std::unique_ptr<const Code> compile_syntax_tree(Node*);
        private:
            bool is_syntax_tree_valid(const Node*);
            void compile(const Node*);
            void compile(const Member*);
            void compile(const Scope*, bool _insert_fake_return = false);
            void compile(const InstructionNode*);
            void compile(const ForLoopNode*);
            void compile(const ConditionalStructNode*);
            void compile_as_condition(const InstructionNode *_instr_node);

            std::unique_ptr<Code> m_temp_code;
        };
    }
}
