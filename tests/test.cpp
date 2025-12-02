#include "graph.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <numeric>
#include <random>
#include <sstream>

#include "gtest/gtest.h"
#include <vector>

namespace {

using namespace graphs;

std::stringstream build_random_dag_description(size_t n, bool randomize_indexes, bool add_loop);

std::filesystem::path get_test_dump_dir(const testing::TestInfo* const test_info);

std::stringstream read_from_file(std::filesystem::path path);

#define DUMP_DIR() (get_test_dump_dir(testing::UnitTest::GetInstance()->current_test_info()))

std::stringstream build_random_dag_description(size_t n, bool add_loop = false) {
    std::vector<size_t> node_indexes(n);
    std::iota(node_indexes.begin(), node_indexes.end(), 1);

    std::mt19937 random_generator;

    std::shuffle(node_indexes.begin(), node_indexes.end(), random_generator);

    std::stringstream description;

    std::vector<std::vector<bool>> adjacent_matrix(n, std::vector<bool>(n, false));

    for (size_t source_idx = 0; source_idx < n; source_idx++) {
        for (size_t dest_idx = 0; dest_idx < source_idx; dest_idx++) {
            if (random_generator() % 5 == 0)
                adjacent_matrix[source_idx][dest_idx] = true;
        }
    }

    if (add_loop) {
        size_t loop_start = random_generator() % n;
        size_t prev = loop_start;

        for (size_t loop_length = random_generator() % (2*n); loop_length > 0; loop_length--) {
            size_t current = random_generator() % n;

            adjacent_matrix[prev][current] = true;
            prev = current;
        }

        adjacent_matrix[prev][loop_start] = true;
    }

    for (size_t source_idx = 0; source_idx < n; source_idx++) {
        description << node_indexes[source_idx];

        for (size_t dest_idx = 0; dest_idx < n; dest_idx++) {
            if (adjacent_matrix[source_idx][dest_idx])
                description << " " << node_indexes[dest_idx];
        }

        description << '\n';
    }

    return description;
}

std::filesystem::path get_test_dump_dir(const testing::TestInfo* const test_info) {
    using namespace std::filesystem;

    path dump_dir(TESTS_SRC_DIR);

    dump_dir /= path("dumps");

    std::filesystem::create_directory(dump_dir);

    dump_dir /= path(test_info->test_suite_name());

    std::filesystem::create_directory(dump_dir);

    return dump_dir / path(test_info->name());
}

std::stringstream read_from_file(std::filesystem::path path) {
    path = std::filesystem::path(TESTS_SRC_DIR) / path;

    std::ifstream file;
    file.exceptions(std::ifstream::badbit);
    file.open(path);

    std::stringstream file_contents;
    file_contents << file.rdbuf();
    file.close();

    return file_contents;
}


TEST(ExamplesTest, BasicExample) {
    EXPECT_NO_THROW({
        std::stringstream file = read_from_file("example.txt");
        DAGraph graph(file, DUMP_DIR());
    });
}

TEST(ExamplesTest, ExampleLoop) {
    EXPECT_THROW({
        std::stringstream file = read_from_file("example_loop.txt");
        DAGraph graph(file, DUMP_DIR());
    }, DAGraph::loops_detected);
}

TEST(ExamplesTest, ExampleSelfLoop) {
    EXPECT_THROW({
        std::stringstream file = read_from_file("example_self_loop.txt");
        DAGraph graph(file, DUMP_DIR());
    }, DAGraph::loops_detected);
}

class GraphGenTest: public testing::Test {
public:
    explicit GraphGenTest(size_t size) : size_(size) {}

protected:
    size_t size_;
};

class InputNoLoopTest: public GraphGenTest {
public:
    explicit InputNoLoopTest(size_t size) : GraphGenTest(size) {}

private:
    void TestBody() override {
        std::stringstream input = build_random_dag_description(size_);

        EXPECT_NO_THROW({
            DAGraph graph(input, DUMP_DIR());
        });
    }
};

class InputLoopTest: public GraphGenTest {
public:
    explicit InputLoopTest(size_t size) : GraphGenTest(size) {}

private:
    void TestBody() override {
        std::stringstream input = build_random_dag_description(size_, true);

        EXPECT_THROW({
            DAGraph graph(input, DUMP_DIR());
        }, DAGraph::loops_detected);
    }
};

class TopologicalSortTest: public GraphGenTest {
public:
    explicit TopologicalSortTest(size_t size) : GraphGenTest(size) {}

private:
    void TestBody() override {
        std::stringstream input = build_random_dag_description(size_);

        DAGraph graph(input, DUMP_DIR());

        graph.topological_sort();

        graph.dump("topo_sort");

        EXPECT_EQ(graph.find_and_break_loops(), 0);
    }
};

} // namespace

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);

    for (size_t i = 0; i < 50; i++) {
        testing::RegisterTest(
            "InputNoLoopTest", ("size_" + std::to_string(i)).c_str(), nullptr,
            std::to_string(i).c_str(),
            __FILE__, __LINE__,
            [=]() -> testing::Test* { return new InputNoLoopTest(i); }
        );
    }

    for (size_t i = 1; i < 50; i++) {
        testing::RegisterTest(
            "InputLoopTest", ("size_" + std::to_string(i)).c_str(), nullptr,
            std::to_string(i).c_str(),
            __FILE__, __LINE__,
            [=]() -> testing::Test* { return new InputLoopTest(i); }
        );
    }

    for (size_t i = 1; i < 50; i++) {
        testing::RegisterTest(
            "TopologicalSortTest", ("size_" + std::to_string(i)).c_str(), nullptr,
            std::to_string(i).c_str(),
            __FILE__, __LINE__,
            [=]() -> testing::Test* { return new TopologicalSortTest(i); }
        );
    }
    return RUN_ALL_TESTS();
}

