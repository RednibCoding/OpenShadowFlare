#ifndef OPENSHADOWFLARE_RUNTIME_GAMEPLAY_ARTWORK_HPP
#define OPENSHADOWFLARE_RUNTIME_GAMEPLAY_ARTWORK_HPP

#include <string>

namespace osf {

class ResourceManager;
class WorldScene;

namespace runtime {

class GameplayUiController;

// Keeps optional gameplay artwork aligned with the currently visible UI.
// Calls are cheap while the UI is unchanged; file access only occurs when a
// panel introduces an artwork sheet that is not already resident.
bool synchronizeGameplayArtwork(
    ResourceManager& resources,
    WorldScene& world,
    const GameplayUiController& ui,
    std::string* error = nullptr);

}  // namespace runtime
}  // namespace osf

#endif
