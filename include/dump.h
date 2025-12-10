#pragma once

#include "graph_traversal.h"

#include <filesystem>

namespace graphs {

using NodeIdx = size_t;

class DumpableNode: public TraversableNode {
public:
    enum StartEndIdx {
        START =  0ul,
        END   = ~0ul,
    };

    void set_index(NodeIdx index) { index_ = index; }

    void dump_subtree(std::ofstream& file, DumpableNode* parent, size_t traversal_counter);

    virtual ~DumpableNode() = default;
protected:
    DumpableNode(NodeIdx index) : index_(index) {}

    virtual void dump_subtree_traversal_(std::ofstream& file, size_t traversal_counter) = 0;

    NodeIdx index_;
};

class DumpableGraph: protected TraversableGraph {
public:
    DumpableGraph(bool generate_dot_images) : generate_dot_images_(generate_dot_images) {}

    void dump(std::filesystem::path path);

    static void generate_dot_image(std::filesystem::path dot_path);

    virtual ~DumpableGraph() = default;
protected:
    const bool generate_dot_images_;

    virtual void dump_traversal_entry_(std::ofstream& file) = 0;
};

} // namespace graphs

