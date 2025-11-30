#include "graph.h"

#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>
#include <string>

using namespace graphs;

int main(int argc, const char* argv[]) {
    cxxopts::Options options("graphs", "Graph topological sorting and dominator tree building");

    options.add_options()
        ("i,input", "Input file with graph description", cxxopts::value<std::filesystem::path>())
        ("d,dump_dir", "Dump directory", cxxopts::value<std::filesystem::path>()->default_value("dumps/"))
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

    try {
        Graph graph(opt_result["input"].as<std::filesystem::path>(),
                    opt_result["dump_dir"].as<std::filesystem::path>());

    } catch (Graph::read_error& e) {
        std::cerr << "Graph read or creation error: " << e.what() << std::endl;
        return -1;
    }  catch (Graph::loops_detected& e) {
        std::cerr << "Detected " << e.what() << " loop(s) in graph" << std::endl;
        return -2;
    }

    return 0;
}
