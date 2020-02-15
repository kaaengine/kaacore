#include "kaacore/exceptions.h"
#include "kaacore/nodes.h"

#include "kaacore/node_ptr.h"

namespace kaacore {

_NodePtrBase::_NodePtrBase(Node* node) : _node(node) {}

_NodePtrBase::operator bool() const
{
    return this->_node != nullptr;
}

bool
_NodePtrBase::operator==(const Node* node) const
{
    return this->_node == node;
}

Node*
_NodePtrBase::get() const
{
    return this->_node;
}

Node* _NodePtrBase::operator->() const
{
    KAACORE_CHECK(this->_node != nullptr);
    KAACORE_CHECK(not this->_node->_marked_to_delete);
    return this->_node;
}

void
_NodePtrBase::destroy()
{
    if (this->_node == nullptr) {
        throw kaacore::exception("Cannot destroy empty NodePtr.");
    }
    if (this->_node->_scene == nullptr) {
        throw kaacore::exception("Cannot destroy not-in-tree node.");
    }
    if (this->_node->_marked_to_delete) {
        throw kaacore::exception("Node was already marked to deletion.");
    }

    this->_node->_mark_to_delete();
}

NodePtr::NodePtr(Node* node) : _NodePtrBase(node) {}

NodeOwnerPtr::NodeOwnerPtr(Node* node)
    : _NodePtrBase(node), _ownership_transferred(false)
{
    if (node) {
        log<LogLevel::debug, LogCategory::nodes>(
            "Created/moved ownership of node (%p).", this->_node);
    }
}

NodeOwnerPtr::~NodeOwnerPtr()
{
    if (this->_node != nullptr and not this->_ownership_transferred) {
        log<LogLevel::debug, LogCategory::nodes>(
            "Ownership of node (%p) destroyed it.", this->_node);
        delete this->_node;
        this->_node = nullptr;
    } else if (this->_node != nullptr) {
        log<LogLevel::debug, LogCategory::nodes>(
            "Ownership of node (%p) released without destroying it.",
            this->_node);
    }
}

NodeOwnerPtr::NodeOwnerPtr(NodeOwnerPtr&& other) : NodeOwnerPtr(other._node)
{
    this->_ownership_transferred = other._ownership_transferred;
    other._node = nullptr;
}

NodeOwnerPtr&
NodeOwnerPtr::operator=(NodeOwnerPtr&& other)
{
    this->_node = other._node;
    this->_ownership_transferred = other._ownership_transferred;
    other._node = nullptr;
    return *this;
}

NodeOwnerPtr::operator NodePtr() const
{
    return NodePtr(this->_node);
}

} // namespace kaacore
