#pragma once

#include "dom_tree.h"
#include "dump.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace graphs {

class DAGraph: public DumpableGraph {
public:
    DAGraph(std::stringstream& text_stream,
            std::filesystem::path input_dump = std::filesystem::path(),
            bool generate_dot_images = true);

    size_t find_and_break_loops() {
        size_t loop_count = start_->count_and_break_loops_traversal(traversal_counter_);

        traversal_counter_ += Node::VISITED;
        return loop_count;
    }

    void topological_sort();

    bool topological_sort_check();

    DomTree build_dominator_tree();

    DomTree build_postdominator_tree();

    struct creation_error: public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    struct loops_detected: public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

private:
    DAGraph(bool generate_dot_images) : DumpableGraph(generate_dot_images) {}

    virtual void dump_traversal_entry_(std::ofstream& file) override {
        start_->dump_subtree(file, nullptr, traversal_counter_);
    }

    class Node: public DumpableNode {
    public:
        Node(NodeIdx index) : DumpableNode(index) {}

        void add_child(std::shared_ptr<Node> node) {
            children_.push_back(node);
        }

        size_t count_and_break_loops_traversal(size_t traversal_counter);

        void topological_sort_traversal(std::vector<std::weak_ptr<Node>>* stack,
                                        size_t traversal_counter);

        bool topological_sort_check_traversal(size_t traversal_counter);

        void build_dominator_sets_traversal_(const std::set<NodeIdx>& parent_set);

        const std::set<NodeIdx>& build_postdominator_sets_traversal_();

        void build_dominator_tree_traversal_(DomTree* dom_tree, size_t traversal_counter);

        void build_postdominator_tree_traversal_(DomTree* postdom_tree, size_t traversal_counter);

    private:
        std::vector<std::shared_ptr<Node>> children_;

        virtual void dump_subtree_traversal_(std::ofstream& file, size_t traversal_counter) override {
            for (auto node: children_)
                node->dump_subtree(file, this, traversal_counter);
        };

        std::set<NodeIdx> dominators_;
        std::set<NodeIdx> postdominators_;
    };

    std::shared_ptr<Node> start_ = std::make_shared<Node>(Node::START);
    std::shared_ptr<Node> end_   = std::make_shared<Node>(Node::END);
};

} //< namespace graphs

