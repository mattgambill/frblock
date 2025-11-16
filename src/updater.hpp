#pragma once

#include <string>
#include <utility>
#include <vector>

// Generic comment block updater that replaces known "old" blocks with new ones.
class CommentBlockUpdater {
public:
    struct Mapping {
        std::vector<std::string> oldBlock; // trimmed lines to match
        std::vector<std::string> newBlock; // lines to insert (indentation added automatically)
    };

    explicit CommentBlockUpdater(std::vector<Mapping> mappings, bool stripNested = false);

    // Apply all mappings to the given lines, returning the updated lines. Multiple occurrences
    // of the same old block are replaced. Optionally strips nested markers based on mappings.
    std::vector<std::string> apply(const std::vector<std::string>& lines) const;

    const std::vector<std::string>& warnings() const { return warnings_; }

private:
    struct NormalizedMapping {
        std::vector<std::string> oldTrimmed;
        std::vector<std::string> newBlock;
    };

    static std::string trim(const std::string& line);
    static std::string leadingWhitespace(const std::string& line);
    static std::string ltrim(const std::string& line);

    bool matchesAt(const std::vector<std::string>& lines,
                   std::size_t start,
                   const NormalizedMapping& mapping,
                   std::size_t& consumed,
                   std::string& indent) const;

    // Heuristically derive start/end marker pairs from mappings: single-line newBlock entries that
    // contain "start"/"begin" are paired with those containing "end"/"stop"/"close".
    static std::vector<std::pair<std::string, std::string>> detectStripPairs(
        const std::vector<NormalizedMapping>& mappings);

    std::vector<NormalizedMapping> mappings_;
    bool stripNested_{false};
    mutable std::vector<std::string> warnings_;
};
