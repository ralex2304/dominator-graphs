#include "graph.h"

#include <cassert>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace graphs;
using namespace std::literals;

DAGraph::DAGraph(std::stringstream& text_stream, std::filesystem::path dump_dir = std::filesystem::path())
    : dump_dir_(dump_dir) {

    if (!dump_dir_.empty())
        std::filesystem::create_directory(dump_dir_);

    struct NodeInfo {
        std::shared_ptr<Node> node;
        bool is_start;
        bool is_end;
    };

    std::unordered_map<NodeIdx, NodeInfo> nodes;

    for (std::string line; std::getline(text_stream, line);) {
        std::istringstream line_stream(line);

        NodeIdx parent_index = 0;
        line_stream >> parent_index;
        auto [node, is_inserted] = nodes.try_emplace(parent_index, std::make_shared<Node>(parent_index), true, true);
        if (!is_inserted && !node->second.is_end)
            throw creation_error(std::format("Trying to add existing node {}", parent_index));

        NodeIdx ancestor_index = 0;
        while (line_stream >> ancestor_index) {

            auto [ancestor, is_inserted] = nodes.try_emplace(ancestor_index, std::make_shared<Node>(ancestor_index), false, true);

            node->second.node->add_ancestor(ancestor->second.node);

            node->second.is_end = false;
        }
    }

    for (auto node: nodes) {
        if (node.second.is_start)
            start_->add_ancestor(node.second.node);

        if (node.second.is_end)
            node.second.node->add_ancestor(end_);
    }

    dump("input");

    size_t loop_count = 0;
    if ((loop_count = find_and_break_loops()) != 0)
        throw loops_detected(std::to_string(loop_count));
}

void DAGraph::dump(std::filesystem::path path) {
    if (dump_dir_.empty())
        return;

    path = dump_dir_ / path;

    std::ofstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(path.replace_extension("dot"));

    file << "digraph List{\n"
            "    graph [bgcolor=\"#1f1f1f\"];\n"
            "    node[shape=rect, color=white, fontcolor=\"#000000\", fontsize=14, "
                "fontname=\"verdana\", style=\"filled\", fillcolor=\"#6e7681\"];\n\n";

    start_->dump_subtree(file, traversal_counter_);
    traversal_counter_ += VISITED;

    file << "\n}\n";

    file.close();

    std::filesystem::path image_path = path;
    image_path.replace_extension("svg");

    std::system(("dot -Tsvg " + path.generic_string() + " > " +
                 image_path.generic_string()).c_str());
}

void DAGraph::topological_sort() {
    std::vector<std::weak_ptr<Node>> stack;
    start_->topological_sort_traversal(&stack, traversal_counter_);
    traversal_counter_ += VISITED;

    for (size_t i = 1; i <= stack.size(); i++) {
        std::shared_ptr<Node> node = stack[stack.size() - i].lock();
        node->set_index(i);
    }
}

void DAGraph::Node::dump_subtree(std::ofstream& file, size_t traversal_counter) {
    if (traversal_status_(traversal_counter) != UNVISITED) {
        assert(traversal_status_(traversal_counter) == VISITED);
        return;
    }
    traversal_counter_ = traversal_counter + VISITED;

    file << "\nnode_" << index_ << " [label=\"";

    if (index_ == START)
        file << "Start";
    else if (index_ == END)
        file << "End";
    else
        file << "Node" << index_;

    file << "\"]\n";

    for (auto node: ancestors_) {
        node->dump_subtree(file, traversal_counter);

        file << "node_" << index_ << "->node_" << node->index_ << "[color=white]\n";
    }

    file << "\n";
};

DAGraph::TraversalStatus DAGraph::Node::traversal_status_(size_t traversal_counter) {
    assert(traversal_counter_ >= traversal_counter);
    assert(traversal_counter_ - traversal_counter < INCORRECT);

    return static_cast<TraversalStatus>(traversal_counter_ - traversal_counter);
}

size_t DAGraph::Node::count_and_break_loops_traversal(size_t traversal_counter) {
    assert(traversal_status_(traversal_counter) == UNVISITED);

    traversal_counter_ = traversal_counter + VISITING;

    size_t loop_count = 0;

    for (size_t i = 0; i < ancestors_.size(); i++) {
        switch (ancestors_[i]->traversal_status_(traversal_counter)) {
            case UNVISITED:
                loop_count += ancestors_[i]->count_and_break_loops_traversal(traversal_counter);
                break;

            case VISITING:
                ancestors_[i] = nullptr;
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

            for (auto ancestor: ancestors_) {
                ancestor->topological_sort_traversal(stack, traversal_counter);

                if (ancestor->index_ != END)
                    stack->push_back(ancestor);
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

    bool sorted = true;
    for (auto ancestor: ancestors_) {
        switch (ancestor->traversal_status_(traversal_counter)) {
            case UNVISITED:
                traversal_counter_ = traversal_counter + VISITED;

                sorted |= ancestor->topological_sort_check_traversal(traversal_counter);

            [[fallthrough]];
            case VISITED:
                sorted |= ancestor->index_ < index_;
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


