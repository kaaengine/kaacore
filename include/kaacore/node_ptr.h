#pragma once

namespace kaacore {

class Node;

class _NodePtrBase {
  public:
    operator bool() const;
    bool operator==(const Node*) const;
    bool is_marked_to_delete() const;

    Node* get() const;
    Node* operator->() const;

    void destroy();

  protected:
    Node* _node;

    _NodePtrBase(Node* node);
};

class NodePtr : public _NodePtrBase {
  public:
    NodePtr(Node* node = nullptr);
};

class NodeOwnerPtr : public _NodePtrBase {
  public:
    explicit NodeOwnerPtr(Node* node = nullptr);

    ~NodeOwnerPtr();
    NodeOwnerPtr(const NodeOwnerPtr&) = delete;
    NodeOwnerPtr(NodeOwnerPtr&&);

    NodeOwnerPtr& operator=(const NodeOwnerPtr&) = delete;
    NodeOwnerPtr& operator=(NodeOwnerPtr&&);

    operator NodePtr() const;

    NodePtr release();

    friend class Node;
};

} // namespace kaacore
