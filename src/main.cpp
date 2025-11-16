#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "argparse.hpp"

#include "config.hpp"
#include "updater.hpp"

namespace {
struct Arguments {
    std::filesystem::path input;
    std::filesystem::path config{"markings.cfg"};
    bool stripNested{false};
};

std::vector<std::string> readLines(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Failed to open file: " + path.string());
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    return lines;
}

Arguments parseArguments(int argc, char** argv) {
    argparse::ArgumentParser program("replace_code", "0.1");
    program.add_argument("input")
        .help("C++ source file to update");
    program.add_argument("-c", "--config")
        .help("Config file path (default: markings.cfg)")
        .default_value(std::string("markings.cfg"));
    program.add_argument("--strip-nested")
        .help("Delete nested safety-critical start/end markers after updating")
        .default_value(false)
        .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << "\n";
        std::cerr << program;
        throw;
    }

    Arguments args;
    args.input = program.get<std::string>("input");
    args.config = program.get<std::string>("-c");
    args.stripNested = program.get<bool>("--strip-nested");
    return args;
}

void writeLines(const std::filesystem::path& path, const std::vector<std::string>& lines) {
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Failed to write file: " + path.string());
    }
    for (std::size_t i = 0; i < lines.size(); ++i) {
        out << lines[i];
        if (i + 1 < lines.size()) {
            out << '\n';
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    Arguments arguments;
    try {
        arguments = parseArguments(argc, argv);
    } catch (...) {
        return 1;
    }

    try {
        Config config(arguments.config);
        std::vector<CommentBlockUpdater::Mapping> mappings = config.loadMappings();
        CommentBlockUpdater updater(mappings, arguments.stripNested);
        
        std::vector<std::string> lines = readLines(arguments.input);

        auto totalStart = std::chrono::steady_clock::now();

        std::vector<std::string> updated = updater.apply(lines);
        writeLines(arguments.input, updated);

        auto totalEnd = std::chrono::steady_clock::now();
        auto totalUs = std::chrono::duration_cast<std::chrono::microseconds>(totalEnd - totalStart);

        std::cout << "Updated file: " << arguments.input << "\n";
        std::cout << "  total time: " << totalUs.count() << " us\n";

        if (!updater.warnings().empty()) {
            std::cerr << "Warnings:\n";
            for (const auto& w : updater.warnings()) {
                std::cerr << "  - " << w << "\n";
            }
        }

    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        return 1;
    }

    return 0;
}
