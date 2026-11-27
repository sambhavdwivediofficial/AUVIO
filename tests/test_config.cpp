#include <catch2/catch_test_macros.hpp>
#include "Config.h"

#include <filesystem>

TEST_CASE("Config stores and retrieves values", "[config]") {
    auto& config = auvio::Config::instance();

    config.set("volume", 72);
    REQUIRE(config.get("volume", 0) == 72);

    config.set("fullscreen", true);
    REQUIRE(config.get("fullscreen", false) == true);

    config.set("last_file", std::string("movie.mp4"));
    REQUIRE(config.get("last_file", std::string()) == "movie.mp4");
}

TEST_CASE("Config save and load round-trip", "[config]") {
    auto& config = auvio::Config::instance();
    const std::string path = (std::filesystem::temp_directory_path() / "auvio_test_config.ini").string();

    config.set("volume", 55);
    REQUIRE(config.save(path));
    REQUIRE(config.load(path));
    REQUIRE(config.get("volume", 0) == 55);

    std::filesystem::remove(path);
}
