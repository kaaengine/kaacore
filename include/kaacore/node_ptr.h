#pragma once

namespace kaacore {

class Node;

class _NodePtrBase {
  protected:
    Node* _node;

    _NodePtrBase(Node* node);

  public:
    operator bool() const;
    bool operator==(const Node*) const;

    Node* get() const;
    Node* operator->() const;

    void destroy();
};

class NodePtr : public _NodePtrBase {
  public:
    NodePtr(Node* node = nullptr);
};

class NodeOwnerPtr : public _NodePtrBase {
    friend class Node;

    bool _ownership_transferred;

  public:
    explicit NodeOwnerPtr(Node* node = nullptr);

    ~NodeOwnerPtr();
    NodeOwnerPtr(const NodeOwnerPtr&) = delete;
    NodeOwnerPtr(NodeOwnerPtr&&);

    NodeOwnerPtr& operator=(const NodeOwnerPtr&) = delete;
    NodeOwnerPtr& operator=(NodeOwnerPtr&&);

    operator NodePtr() const;
};

} // namespace kaacore
