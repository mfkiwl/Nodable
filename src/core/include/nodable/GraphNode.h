#pragma once

#include <string>
#include <vector>
#include <nodable/Reflect.h>
#include <nodable/AbstractCodeBlock.h>

#include <nodable/Nodable.h>
#include <nodable/Component.h>
#include <nodable/Node.h>
#include <nodable/Language.h>

namespace Nodable{

    // forward declaration
    class ScopeNode;
    class InstructionNode;
    class ConditionalStructNode;
    class LiteralNode;
    class AbstractNodeFactory;

    typedef std::pair<const RelationType, std::pair<Node*, Node*>> Relation;

    inline bool operator==(
            const Relation& _left,
            const Relation& _right)
    {
        return (_left.first == _right.first) && (_left.second == _right.second);
    }

    /**
     * @brief
     * The role of a GraphNode is to manage a set of Node and Wire used in a ScopeNode with a given language.
     *
     * @details
     * Nodes and Wires are instantiated and destroyed by this class.
     * The ScopedNode contain the structure of the program in which Nodes are used.
     */
	class GraphNode: public Node {
	public:

		explicit GraphNode(const Language* _language, const AbstractNodeFactory* _factory);
		~GraphNode();

		/** Update the graph by evaluating its nodes only when necessary. */
        UpdateResult                update() override;

        /** Clear Graph. Delete all Nodes/Wires and reset scope */
		void                        clear();

		/** Find a VariableNode given its name/identifier
		 * @return - a valid VariableNode* or nullptr if variable is not found. */
        [[nodiscard]] VariableNode*           findVariable(std::string);

        [[nodiscard]] std::vector<Node*>&     getNodeRegistry() {return m_nodeRegistry;}
        [[nodiscard]] std::vector<Wire*>&     getWireRegistry() {return m_wireRegistry;}
        [[nodiscard]] inline const Language*  getLanguage()const { return m_language; }
        [[nodiscard]] inline ScopeNode* getProgram(){ return m_program; }
        [[nodiscard]] bool                    hasProgram();
        [[nodiscard]] std::multimap<Relation::first_type , Relation::second_type>& getRelationRegistry() {return m_relationRegistry;}

        /* node factory */
        ScopeNode*        newProgram();
        InstructionNode*		    newInstruction_UserCreated();
        InstructionNode*            newInstruction();
		VariableNode*				newVariable(Type, const std::string&, ScopeNode*);
		LiteralNode*                newLiteral(const Type &type);
		Node*                       newBinOp(const InvokableOperator*);
		Node*                       newUnaryOp(const InvokableOperator*);
        Node*                       newOperator(const InvokableOperator*);
		Wire*                       newWire();
		Node*                       newFunction(const Invokable* _proto);
        ScopeNode*        newScope();
        ConditionalStructNode*      newConditionalStructure();
        ForLoopNode*                new_for_loop_node();
        Node*                       newNode();

        /** Connects two Member using a Wire (oriented edge)
         *  If _from is not owned, _to will digest it and nullptr is return.
          * Otherwise a new Wire will be created ( _from -----> _to) and returned.
          */
        Wire* connect(Member *_source, Member *_target, ConnBy_ _connect_by = ConnectBy_Ref );

        /**
         * Connect two nodes with a given connection type
         * ex: _source IS_CHILD_OF _target
        */
        void connect(Node* _source, Node* _target, RelationType, bool _sideEffects = true);
        void connect(Member* _source, InstructionNode* _target);
        void connect(Member* _source, VariableNode* _target);
        void disconnect(Node* _source, Node* _target, RelationType, bool _sideEffects = true);
        void disconnect(Wire* _wire);
        void disconnect(Member* _member, Way _way = Way_InOut);
        void deleteNode(Node* _node);
    private:
        void registerNode(Node* node);
        void unregisterNode(Node* node);
        void registerWire(Wire *_wire);
        void unregisterWire(Wire *_wire);
        void deleteWire(Wire* _wire);

	private:		
		std::vector<Node*> m_nodeRegistry;
		std::vector<Wire*> m_wireRegistry;
		std::multimap<Relation::first_type , Relation::second_type> m_relationRegistry;
		const Language*            m_language;
		ScopeNode*       m_program;
		const AbstractNodeFactory* m_factory;

        REFLECT_DERIVED(GraphNode)
        REFLECT_EXTENDS(Node)
        REFLECT_END
    };
}