#include "dagraph.h"
#include "dom_tree.h"

#include <cxxopts.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace graphs;

int main(int argc, const char* argv[]) {
    cxxopts::Options options("graphs",
        "Directed Acyclic Graph topological sorting and dominator tree building");

    options.add_options()
        ("i,input", "Input file with graph description", cxxopts::value<std::filesystem::path>())
        ("d,dump_dir", "Dump directory", cxxopts::value<std::filesystem::path>()->
                                                  default_value("dumps/"))
        ("h,help", "Print help")
    ;

    options.positional_help("<input>");

    options.parse_positional("input");
    options.show_positional_help();
    auto opt_result = options.parse(argc, argv);

    if (opt_result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    const auto& dump_dir = opt_result["dump_dir"].as<std::filesystem::path>();
    std::filesystem::create_directory(dump_dir);

    try {
        std::ifstream file;
        file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        file.open(opt_result["input"].as<std::filesystem::path>());

        std::stringstream file_contents;
        file_contents << file.rdbuf();
        file.close();

        DAGraph graph(file_contents, dump_dir / "input");

        graph.topological_sort();
        graph.dump(dump_dir / "topo_sort");

        DomTree dominator_tree = graph.build_dominator_tree();
        dominator_tree.dump(dump_dir / "dom_tree");

        DomTree postdominator_tree = graph.build_postdominator_tree();
        postdominator_tree.dump(dump_dir / "postdom_tree");

    } catch (const std::ifstream::failure &e) {
        std::cerr << "DAGraph read error: " << e.what() << std::endl;
        return -1;
    } catch (DAGraph::creation_error& e) {
        std::cerr << "DAGraph creation error: " << e.what() << std::endl;
        return -2;
    }  catch (DAGraph::loops_detected& e) {
        std::cerr << "Detected " << e.what() << " loop(s) in graph" << std::endl;
        return -3;
    }

    return 0;
}

