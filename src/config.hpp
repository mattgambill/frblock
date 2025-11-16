#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "updater.hpp"

class Config {
public:
    explicit Config(std::filesystem::path configPath);

    const std::filesystem::path& path() const { return path_; }

    // Loads mappings from the config file on demand.
    std::vector<CommentBlockUpdater::Mapping> loadMappings() const;

private:
    static std::string trim(const std::string& line);

    std::filesystem::path path_;
};
