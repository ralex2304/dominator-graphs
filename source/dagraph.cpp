#include "dagraph.h"
#include "dom_tree.h"
#include "dump.h"
#include "graph_traversal.h"

#include <cassert>
#include <cstddef>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace graphs;
using namespace std::literals;

DAGraph::DAGraph(std::stringstream& text_stream, std::filesystem::path input_dump, bool generate_images)
    : DumpableGraph(generate_images) {

    struct NodeInfo {
        std::shared_ptr<Node> node;
        bool is_start;
        bool is_end;
    };

    std::unordered_map<NodeIdx, NodeInfo> nodes;

    for (std::string line; std::getline(text_stream, line);) {
        if (line.size() == 0)
            continue;

        std::istringstream line_stream(line);

        NodeIdx parent_index = 0;
        line_stream >> parent_index;
        if (line_stream.fail())
            throw creation_error("failed to parse line \""s + line + "\""s);

        auto [node, is_inserted] = nodes.try_emplace(parent_index, std::make_shared<Node>(parent_index), true, true);
        if (!is_inserted && !node->second.is_end)
            throw creation_error(std::format("Trying to add existing node {}", parent_index));

        NodeIdx child_index = 0;
        while (line_stream >> child_index) {
            auto [child, is_inserted] = nodes.try_emplace(child_index, std::make_shared<Node>(child_index), false, true);

            node->second.node->add_child(child->second.node);

            node->second.is_end = false;
        }
        if (line_stream.fail() && !line_stream.eof())
            throw creation_error("failed to parse line \""s + line + "\""s);


    }

    for (auto node: nodes) {
        if (node.second.is_start)
            start_->add_child(node.second.node);

        if (node.second.is_end)
            node.second.node->add_child(end_);
    }

    if (!input_dump.empty())
        dump(input_dump);

    size_t loop_count = 0;
    if ((loop_count = find_and_break_loops()) != 0)
        throw loops_detected(std::to_string(loop_count));
}

void DAGraph::topological_sort() {
    std::vector<std::weak_ptr<Node>> stack;
    start_->topological_sort_traversal(&stack, traversal_counter_);
    traversal_counter_ += Node::VISITED;

    for (size_t i = 1; i <= stack.size(); i++) {
        std::shared_ptr<Node> node = stack[stack.size() - i].lock();
        node->set_index(i);
    }
}

bool DAGraph::topological_sort_check() {
    bool result = start_->topological_sort_check_traversal(traversal_counter_);
    traversal_counter_ += Node::VISITED;
    return result;
}

DomTree DAGraph::build_dominator_tree() {
    start_->build_dominator_sets_traversal_({});

    DomTree dom_tree(DomTree::DomType::DOMINATOR, generate_dot_images_);

    start_->build_dominator_tree_traversal_(&dom_tree, traversal_counter_);
    traversal_counter_ += Node::VISITED;

    return dom_tree;
}

DomTree DAGraph::build_postdominator_tree() {
    start_->build_postdominator_sets_traversal_();

    DomTree postdom_tree(DomTree::DomType::POSTDOMINATOR, generate_dot_images_);

    start_->build_postdominator_tree_traversal_(&postdom_tree, traversal_counter_);
    traversal_counter_ += Node::VISITED;

    return postdom_tree;
}

void DAGraph::Node::build_dominator_sets_traversal_(const std::set<NodeIdx>& parent_set) {
    if (dominators_.empty()) {
        dominators_ = parent_set;
        dominators_.insert(index_);
    } else {
        std::erase_if(dominators_, [this, parent_set](NodeIdx index){
                return !(index == index_ || parent_set.contains(index));
            });
    }

    for (auto child: children_)
        child->build_dominator_sets_traversal_(dominators_);
}

const std::set<NodeIdx>& DAGraph::Node::build_postdominator_sets_traversal_() {
    for (auto child: children_) {
        const std::set<NodeIdx>& child_set = child->build_postdominator_sets_traversal_();

        if (postdominators_.empty()) {
            postdominators_ = child_set;
            postdominators_.insert(index_);
        } else {
            std::erase_if(postdominators_, [this, child_set](NodeIdx index){
                    return !(index == index_ || child_set.contains(index));
                });
        }
    }
    if (children_.size() == 0)
        postdominators_.insert(index_);

    return postdominators_;
}

void DAGraph::Node::build_dominator_tree_traversal_(DomTree* dom_tree, size_t traversal_counter) {
    assert(dom_tree);
    if (traversal_status_(traversal_counter) != UNVISITED) {
        assert(traversal_status_(traversal_counter) == VISITED);
        return;
    }
    traversal_counter_ = traversal_counter + VISITED;

    dom_tree->add_node_with_dominators(index_, dominators_);

    for (auto child: children_)
        child->build_dominator_tree_traversal_(dom_tree, traversal_counter);
}

void DAGraph::Node::build_postdominator_tree_traversal_(DomTree* postdom_tree, size_t traversal_counter) {
    assert(postdom_tree);
    if (traversal_status_(traversal_counter) != UNVISITED) {
        assert(traversal_status_(traversal_counter) == VISITED);
        return;
    }
    traversal_counter_ = traversal_counter + VISITED;

    for (auto child: children_)
        child->build_postdominator_tree_traversal_(postdom_tree, traversal_counter);

    postdom_tree->add_node_with_dominators(index_, postdominators_);
}

size_t DAGraph::Node::count_and_break_loops_traversal(size_t traversal_counter) {
    assert(traversal_status_(traversal_counter) == UNVISITED);

    traversal_counter_ = traversal_counter + VISITING;

    size_t loop_count = 0;

    for (size_t i = 0; i < children_.size(); i++) {
        switch (children_[i]->traversal_status_(traversal_counter)) {
            case UNVISITED:
                loop_count += children_[i]->count_and_break_loops_traversal(traversal_counter);
                break;

            case VISITING:
                children_[i] = nullptr;
                loop_count += 1;
                break;

            case VISITED:
                break;

            case INCORRECT:
            default:
                assert(0 && "DAGraph is corrupted");
                loop_count += true;
                break;
        }
    }

    traversal_counter_ = traversal_counter + VISITED;
    return loop_count;
}

void DAGraph::Node::topological_sort_traversal(std::vector<std::weak_ptr<Node>>* stack, size_t traversal_counter) {
    assert(stack);

    switch (traversal_status_(traversal_counter)) {
        case UNVISITED:
            traversal_counter_ = traversal_counter + VISITED;

            for (auto child: children_) {
                child->topological_sort_traversal(stack, traversal_counter);

                if (child->index_ != END)
                    stack->push_back(child);
            }

            return;

        case VISITED:
            return;

        case VISITING:
        case INCORRECT:
        default:
            assert(0 && "DAGraph is corrupted");
            return;
    }
}

bool DAGraph::Node::topological_sort_check_traversal(size_t traversal_counter) {
    assert(traversal_status_(traversal_counter) == UNVISITED);
    traversal_counter_ = traversal_counter + VISITED;

    bool sorted = true;
    for (auto child: children_) {
        switch (child->traversal_status_(traversal_counter)) {
            case UNVISITED:
                sorted |= child->topological_sort_check_traversal(traversal_counter);

            [[fallthrough]];
            case VISITED:
                sorted |= child->index_ < index_;
                break;

            case VISITING:
            case INCORRECT:
            default:
                assert(0 && "DAGraph is corrupted");
                sorted = false;
                break;
        }
    }

    return sorted;
}


