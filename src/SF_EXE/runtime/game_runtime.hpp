#pragma once

#include "core/game_config.hpp"

#include <filesystem>

namespace osf::runtime {

int runGame(
    const std::filesystem::path& data_root,
    const GameConfig& game_config,
    bool smoke_test);

}  // namespace osf::runtime
