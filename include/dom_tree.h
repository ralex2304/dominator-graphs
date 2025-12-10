#pragma once

#include "dump.h"

#include <cassert>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <strings.h>
#include <vector>

namespace graphs {

class DomTreeNode: public DumpableNode {
public:
    DomTreeNode(NodeIdx index) : DumpableNode(index) {}

    void add_node_with_dominators_traversal(NodeIdx index,
                                            const std::set<NodeIdx>& dominators) {
        for (auto child: children_) {
            if (dominators.contains(child->index_)) {
                child->add_node_with_dominators_traversal(index, dominators);
                return;
            }
        }

        assert(index_ != index);
        children_.push_back(std::make_shared<DomTreeNode>(index));
    }

    NodeIdx get_index() { return index_; }

private:
    virtual void dump_subtree_traversal_(std::ofstream& file, size_t traversal_counter) override {
        for (auto node: children_)
            node->dump_subtree(file, this, traversal_counter);
    };

    std::vector<std::shared_ptr<DomTreeNode>> children_;
};

class DomTree: public DumpableGraph {
public:
    enum class DomType : NodeIdx {
        DOMINATOR     = DumpableNode::START,
        POSTDOMINATOR = DumpableNode::END,
    };
    
    DomTree(DomType dom_type, bool generate_dot_images = true)
        : DumpableGraph(generate_dot_images)
        , root_(std::make_shared<DomTreeNode>(static_cast<NodeIdx>(dom_type))) {}

    void add_node_with_dominators(NodeIdx index,
                                  const std::set<NodeIdx>& dominators) {
        if (index == root_->get_index()) {
            assert(dominators.size() == 1);
            return;
        }

        root_->add_node_with_dominators_traversal(index, dominators);
    }

private:
    virtual void dump_traversal_entry_(std::ofstream& file) override {
        root_->dump_subtree(file, nullptr, traversal_counter_);
    }

    std::shared_ptr<DomTreeNode> root_;
};

} //< namespace graphs

