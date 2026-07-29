#ifndef OPENSHADOWFLARE_CHARACTER_SELECT_FLOW_HPP
#define OPENSHADOWFLARE_CHARACTER_SELECT_FLOW_HPP

#include "states/character_select_state.hpp"

#include <cstdint>
#include <functional>

namespace osf::character_select {

bool updateSavedGameDeleteDialog(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result,
    const std::function<void(std::int32_t)>& delete_saved_character);
void updateSavedGameMode(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result);
void updateNewCharacterMode(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result);
void updateGameModeScreen(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result);
void updateNetworkModeScreen(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result);
void updateHostScreen(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result,
    const std::function<std::string()>& read_clipboard);

}  // namespace osf::character_select

#endif
