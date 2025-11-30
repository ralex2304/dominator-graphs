#include "graph.h"

#include <cassert>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

using namespace graphs;
using namespace std::literals;

Graph::Graph(std::filesystem::path path, std::filesystem::path dump_dir) {
    std::filesystem::create_directory(dump_dir_ = dump_dir);

    struct NodeInfo {
        std::shared_ptr<Node> node;
        bool is_start;
        bool is_end;
    };

    std::unordered_map<NodeIdx, NodeInfo> nodes;

    try {
        std::ifstream file;
        file.exceptions(std::ifstream::badbit);

        file.open(path);

        for (std::string line; std::getline(file, line);) {
            std::istringstream line_stream(line);

            NodeIdx parent_index = 0;
            line_stream >> parent_index;
            auto [node, is_inserted] = nodes.try_emplace(parent_index, std::make_shared<Node>(parent_index), true, false);
            if (!is_inserted && !node->second.is_end)
                throw read_error(std::format("Trying to add existing node {}", parent_index));

            node->second.is_end = false;

            NodeIdx ancestor_index = 0;
            while (line_stream >> ancestor_index) {

                auto [ancestor, is_inserted] = nodes.try_emplace(ancestor_index, std::make_shared<Node>(ancestor_index), false, true);

                node->second.node->add_ancestor(ancestor->second.node);
            }
        }

        file.close();
    } catch (const std::ifstream::failure &e) {
        throw read_error("File read error: "s + e.what());
    }

    for (auto node: nodes) {
        if (node.second.is_start)
            start_->add_ancestor(node.second.node);

        if (node.second.is_end)
            node.second.node->add_ancestor(end_);
    }

    dump(dump_dir_ / "input");

    size_t loop_count = 0;
    if ((loop_count = find_and_break_loops()) != 0)
        throw loops_detected(std::to_string(loop_count));
}

void Graph::dump(std::filesystem::path path) {
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

void Graph::Node::dump_subtree(std::ofstream& file, size_t traversal_counter) {
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

Graph::TraversalStatus Graph::Node::traversal_status_(size_t traversal_counter) {
    assert(traversal_counter_ >= traversal_counter);
    assert(traversal_counter_ - traversal_counter < INCORRECT);

    return static_cast<TraversalStatus>(traversal_counter_ - traversal_counter);
}

size_t Graph::Node::count_and_break_loops_traversal(size_t traversal_counter) {
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
                assert(0 && "Tree is corrupted");
                loop_count += true;
                break;
        }
    }

    traversal_counter_ = traversal_counter + VISITED;
    return loop_count;
}


