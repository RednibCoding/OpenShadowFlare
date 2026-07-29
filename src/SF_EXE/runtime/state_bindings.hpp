#ifndef OPENSHADOWFLARE_RUNTIME_STATE_BINDINGS_HPP
#define OPENSHADOWFLARE_RUNTIME_STATE_BINDINGS_HPP

#include "states/character_select_state.hpp"
#include "states/gameplay_state.hpp"
#include "states/title_state.hpp"

#include <filesystem>

struct LwlWindow;

namespace osf {

struct PlayerLoadRequest;
class WorldScene;

namespace runtime {

class AudioSystem;
class FrontendAssets;

TitleStateHooks makeTitleStateHooks(
    const std::filesystem::path& data_root,
    FrontendAssets& assets,
    AudioSystem& audio);

CharacterSelectStateHooks makeCharacterSelectStateHooks(
    const std::filesystem::path& data_root,
    FrontendAssets& assets,
    AudioSystem& audio,
    LwlWindow*& window);

GameplayStateHooks makeGameplayStateHooks(
    const std::filesystem::path& data_root,
    PlayerLoadRequest& player,
    FrontendAssets& assets,
    AudioSystem& audio,
    WorldScene& world);

}  // namespace runtime
}  // namespace osf

#endif
