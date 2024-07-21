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

bool
_NodePtrBase::is_marked_to_delete() const
{
    KAACORE_ASSERT(this->_node != nullptr, "Node already deleted/released.");
    return this->_node->_marked_to_delete;
}

Node*
_NodePtrBase::get() const
{
    return this->_node;
}

Node*
_NodePtrBase::operator->() const
{
    KAACORE_CHECK(this->_node != nullptr, "Node already deleted/released.");
    KAACORE_CHECK(
        not this->_node->_marked_to_delete, "Node marked for deletion."
    );
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
    this->_node = nullptr;
}

NodePtr::NodePtr(Node* node) : _NodePtrBase(node) {}

NodeOwnerPtr::NodeOwnerPtr(Node* node) : _NodePtrBase(node)
{
    if (node) {
        KAACORE_LOG_DEBUG(
            "Created ownership of node ({}).", fmt::ptr(this->_node)
        );
    }
}

NodeOwnerPtr::~NodeOwnerPtr()
{
    if (this->_node != nullptr) {
        KAACORE_LOG_DEBUG(
            "Ownership of node ({}) destroyed.", fmt::ptr(this->_node)
        );
        delete this->_node;
        this->_node = nullptr;
    }
}

NodeOwnerPtr::NodeOwnerPtr(NodeOwnerPtr&& other) : NodeOwnerPtr(other._node)
{
    other._node = nullptr;
    KAACORE_LOG_DEBUG("Moved ownership of node ({}).", fmt::ptr(this->_node));
}

NodeOwnerPtr&
NodeOwnerPtr::operator=(NodeOwnerPtr&& other)
{
    this->_node = other._node;
    other._node = nullptr;
    KAACORE_LOG_DEBUG("Moved ownership of node ({}).", fmt::ptr(this->_node));
    return *this;
}

NodeOwnerPtr::operator NodePtr() const
{
    return NodePtr(this->_node);
}

NodePtr
NodeOwnerPtr::release()
{
    auto result = this->_node;
    this->_node = nullptr;
    return result;
}

} // namespace kaacore
