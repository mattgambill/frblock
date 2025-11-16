#include "updater.hpp"

#include <algorithm>
#include <cctype>

CommentBlockUpdater::CommentBlockUpdater(std::vector<Mapping> mappings, bool stripNested)
    : stripNested_(stripNested) {
    mappings_.reserve(mappings.size());
    for (auto& m : mappings) {
        NormalizedMapping nm;
        nm.newBlock = std::move(m.newBlock);
        nm.oldTrimmed.reserve(m.oldBlock.size());
        for (auto& line : m.oldBlock) {
            nm.oldTrimmed.push_back(trim(line));
        }
        mappings_.push_back(std::move(nm));
    }
}

std::vector<std::string> CommentBlockUpdater::apply(const std::vector<std::string>& lines) const {
    warnings_.clear();

    std::vector<std::string> out;
    std::size_t i = 0;

    // Greedy single-pass replacement: at each position, try all mappings and emit the first match.
    // Indentation from the source line is preserved and prefixed to each emitted new line.
    while (i < lines.size()) {
        bool replaced = false;
        for (const auto& mapping : mappings_) {
            std::size_t consumed = 0;
            std::string indent;
            if (matchesAt(lines, i, mapping, consumed, indent)) {
                for (const auto& newLine : mapping.newBlock) {
                    out.push_back(indent + newLine);
                }
                i += consumed;
                replaced = true;
                break;
            }
        }

        if (!replaced) {
            out.push_back(lines[i]);
            ++i;
        }
    }

    if (!stripNested_) {
        return out;
    }

    // Post-process to collapse nested start/end markers derived from mappings.
    // Each marker pair is handled independently to support multiple marker types.
    std::vector<std::string> current = out;
    const auto pairs = detectStripPairs(mappings_);
    for (const auto& [startToken, endToken] : pairs) {
        std::vector<std::string> next;
        int depth = 0;
        bool unmatchedEnd = false;

        for (const auto& line : current) {
            const std::string trimmed = ltrim(line);
            const bool isStart = trimmed.find(startToken) != std::string::npos;
            const bool isEnd = trimmed.find(endToken) != std::string::npos;

            if (isStart) {
                if (depth == 0) {
                    next.push_back(line);
                }
                ++depth;
                continue;
            }

            if (isEnd) {
                if (depth == 0) {
                    unmatchedEnd = true;
                    next.push_back(line);
                    continue;
                }
                if (depth == 1) {
                    next.push_back(line);
                }
                --depth;
                continue;
            }

            next.push_back(line);
        }

        current.swap(next);

        if (depth > 0) {
            warnings_.push_back("Unmatched start token: " + startToken);
        }
        if (unmatchedEnd) {
            warnings_.push_back("Unmatched end token: " + endToken);
        }
    }

    return current;
}

std::string CommentBlockUpdater::trim(const std::string& line) {
    auto first = std::find_if_not(line.begin(), line.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    auto last = std::find_if_not(line.rbegin(), line.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if (first >= last) {
        return {};
    }
    return std::string(first, last);
}

std::string CommentBlockUpdater::leadingWhitespace(const std::string& line) {
    auto it = std::find_if_not(line.begin(), line.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    return std::string(line.begin(), it);
}

std::string CommentBlockUpdater::ltrim(const std::string& line) {
    auto it = std::find_if_not(line.begin(), line.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    return std::string(it, line.end());
}

// Heuristic pairing: single-line new blocks with "start"/"begin" are paired with those containing
// "end"/"stop"/"close". Order follows discovery order in the mappings list.
std::vector<std::pair<std::string, std::string>> CommentBlockUpdater::detectStripPairs(
    const std::vector<NormalizedMapping>& mappings) {
    std::vector<std::string> starts;
    std::vector<std::string> ends;

    auto toLower = [](std::string text) {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return text;
    };

    for (const auto& m : mappings) {
        if (m.newBlock.size() != 1) {
            continue;
        }
        const std::string token = m.newBlock.front();
        const std::string lower = toLower(token);

        if (lower.find("start") != std::string::npos || lower.find("begin") != std::string::npos) {
            starts.push_back(token);
        } else if (lower.find("end") != std::string::npos || lower.find("stop") != std::string::npos ||
                   lower.find("close") != std::string::npos) {
            ends.push_back(token);
        }
    }

    std::vector<std::pair<std::string, std::string>> pairs;
    const std::size_t count = std::min(starts.size(), ends.size());
    for (std::size_t i = 0; i < count; ++i) {
        pairs.emplace_back(starts[i], ends[i]);
    }
    return pairs;
}

bool CommentBlockUpdater::matchesAt(const std::vector<std::string>& lines,
                                    std::size_t start,
                                    const NormalizedMapping& mapping,
                                    std::size_t& consumed,
                                    std::string& indent) const {
    if (start + mapping.oldTrimmed.size() > lines.size()) {
        return false;
    }

    indent = leadingWhitespace(lines[start]);

    for (std::size_t j = 0; j < mapping.oldTrimmed.size(); ++j) {
        if (trim(lines[start + j]) != mapping.oldTrimmed[j]) {
            return false;
        }
    }

    consumed = mapping.oldTrimmed.size();
    return true;
}
