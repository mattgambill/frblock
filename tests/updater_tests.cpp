#include <gtest/gtest.h>

#include <fstream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

#include "config.hpp"
#include "updater.hpp"

namespace {
std::string writeTempConfig(const std::string& contents) {
    char tmpName[] = "/tmp/markingsXXXXXX";
    int fd = mkstemp(tmpName);
    if (fd == -1) {
        throw std::runtime_error("Failed to create temp config");
    }
    close(fd);
    std::ofstream out(tmpName);
    out << contents;
    out.close();
    return std::string(tmpName);
}
} // namespace

TEST(UpdaterTests, ReplacesLegacyBlock) {
    CommentBlockUpdater updater({
        CommentBlockUpdater::Mapping{
            {"OLD"},
            {"NEW"},
        },
    });

    std::vector<std::string> input = {"int x = 0;", "OLD", "return x;"};
    std::vector<std::string> expected = {"int x = 0;", "NEW", "return x;"};

    auto output = updater.apply(input);
    EXPECT_EQ(output, expected);
    EXPECT_TRUE(updater.warnings().empty());
}

TEST(UpdaterTests, StripsNestedMarkersAndWarnsUnmatched) {
    CommentBlockUpdater updater({
        CommentBlockUpdater::Mapping{{"OLD_START"}, {"START"}},
        CommentBlockUpdater::Mapping{{"OLD_END"}, {"END"}},
    }, /*stripNested=*/true);

    // Nested pair should collapse to outermost markers only.
    std::vector<std::string> nested = {"OLD_START", "OLD_START", "payload", "OLD_END", "OLD_END"};
    auto collapsed = updater.apply(nested);
    std::vector<std::string> expected = {"START", "payload", "END"};
    EXPECT_EQ(collapsed, expected);
    EXPECT_TRUE(updater.warnings().empty());

    // Missing end should produce a warning.
    std::vector<std::string> unmatched = {"OLD_START", "payload"};
    auto out2 = updater.apply(unmatched);
    ASSERT_EQ(out2.front(), "START");
    ASSERT_EQ(out2.back(), "payload");
    ASSERT_FALSE(updater.warnings().empty());
}

TEST(ConfigTests, LoadsMappingsAndDrivesUpdater) {
    const std::string cfg = R"(BEGIN
OLD
NEW
NEW
END

BEGIN
X
NEW
Y
END
)";
    const std::string path = writeTempConfig(cfg);
    Config config(path);
    auto mappings = config.loadMappings();
    ASSERT_EQ(mappings.size(), 2u);
    CommentBlockUpdater updater(mappings);

    std::vector<std::string> input = {"OLD", "X"};
    auto out = updater.apply(input);
    std::vector<std::string> expected = {"NEW", "Y"};
    EXPECT_EQ(out, expected);
}
