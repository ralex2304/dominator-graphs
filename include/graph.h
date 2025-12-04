#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace graphs {

using NodeIdx = size_t;

class DAGraph {
public:
    DAGraph(std::stringstream& text_stream,
            std::filesystem::path input_dump = std::filesystem::path(),
            bool generate_dot_images = true);

    void dump(std::filesystem::path path);

    static void generate_dot_image(std::filesystem::path dot_path);

    size_t find_and_break_loops() {
        size_t loop_count = start_->count_and_break_loops_traversal(traversal_counter_);

        traversal_counter_ += VISITED;
        return loop_count;
    }

    void topological_sort();

    bool topological_sort_check();

    struct creation_error: public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    struct loops_detected: public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

private:
    enum StartEndIdx {
        START =  0ul,
        END   = ~0ul,
    };

    enum TraversalStatus {
        UNVISITED = 0,
        VISITING  = 1,
        VISITED   = 2,
        INCORRECT = 3,
    };

    class Node {
    public:
        Node(NodeIdx index) : index_(index) {}

        void add_child(std::shared_ptr<Node> node) {
            children_.push_back(node);
        }

        void set_index(NodeIdx index) {
            index_ = index;
        }

        size_t count_and_break_loops_traversal(size_t traversal_counter);

        void topological_sort_traversal(std::vector<std::weak_ptr<Node>>* stack, size_t traversal_counter);

        bool topological_sort_check_traversal(size_t traversal_counter);

        void dump_subtree(std::ofstream& file, size_t traversal_counter);
    private:
        std::vector<std::shared_ptr<Node>> children_;

        NodeIdx index_;
        size_t traversal_counter_ = UNVISITED;

        TraversalStatus traversal_status_(size_t traversal_counter);
    };

    std::shared_ptr<Node> start_ = std::make_shared<Node>(START);
    std::shared_ptr<Node> end_   = std::make_shared<Node>(END);

    const bool generate_dot_images_;

    size_t traversal_counter_ = UNVISITED;
};

} //< namespace graphs

