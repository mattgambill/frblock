#include "config.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>

Config::Config(std::filesystem::path configPath) : path_(std::move(configPath)) {}

std::string Config::trim(const std::string& line) {
    auto first = std::find_if_not(line.begin(), line.end(), [](unsigned char ch) { return std::isspace(ch); });
    auto last = std::find_if_not(line.rbegin(), line.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
    if (first >= last) {
        return {};
    }
    return std::string(first, last);
}

std::vector<CommentBlockUpdater::Mapping> Config::loadMappings() const {
    std::ifstream in(path_);
    if (!in) {
        throw std::runtime_error("Failed to open config: " + path_.string());
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }

    std::vector<CommentBlockUpdater::Mapping> mappings;
    std::size_t i = 0;
    while (i < lines.size()) {
        std::string lineTrimmed = trim(lines[i]);
        if (lineTrimmed.empty() || lineTrimmed[0] == '#') {
            ++i;
            continue;
        }

        if (lineTrimmed != "BEGIN") {
            throw std::runtime_error("Expected 'BEGIN' in config: " + lines[i]);
        }
        ++i;

        CommentBlockUpdater::Mapping mapping;
        for (; i < lines.size(); ++i) {
            std::string current = trim(lines[i]);
            if (current == "NEW") {
                ++i;
                break;
            }
            mapping.oldBlock.push_back(lines[i]);
        }

        if (mapping.oldBlock.empty()) {
            throw std::runtime_error("Config mapping missing old block");
        }

        for (; i < lines.size(); ++i) {
            std::string current = trim(lines[i]);
            if (current == "END") {
                ++i;
                break;
            }
            mapping.newBlock.push_back(lines[i]);
        }

        if (mapping.newBlock.empty()) {
            throw std::runtime_error("Config mapping missing new block");
        }

        mappings.push_back(std::move(mapping));
    }

    if (mappings.empty()) {
        throw std::runtime_error("No mappings found in config");
    }

    return mappings;
}
