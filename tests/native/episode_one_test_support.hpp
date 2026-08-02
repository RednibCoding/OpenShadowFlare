#ifndef OPENSHADOWFLARE_EPISODE_ONE_TEST_SUPPORT_HPP
#define OPENSHADOWFLARE_EPISODE_ONE_TEST_SUPPORT_HPP

#include "world/enemy_actor.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace osf::test {

inline bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

inline bool containsSample(
    const std::vector<std::int32_t>& samples,
    std::int32_t sample) {
    return std::find(samples.begin(), samples.end(), sample) !=
           samples.end();
}

inline bool findNpcPointerPoint(
    WorldScene& world,
    std::int32_t npc_id,
    ScreenPosition& point) {
    const auto found = std::find_if(
        world.npcs().begin(),
        world.npcs().end(),
        [npc_id](const NpcActor& npc) {
            return npc.id() == npc_id;
        });
    if (found == world.npcs().end()) {
        return false;
    }
    const ScreenPosition anchor =
        calculateRealPosition(found->position());
    for (std::int32_t y = -found->labelHeight(); y <= 16; ++y) {
        for (std::int32_t x = -48; x <= 48; ++x) {
            point = {
                anchor.x - world.cameraScreenX() + x,
                anchor.y - world.cameraScreenY() + y,
            };
            if (point.x < 0 || point.x >= 640 ||
                point.y < 0 || point.y >= 480) {
                continue;
            }
            world.updatePointerHover(point.x, point.y);
            if (world.hoveredNpcId() == npc_id) {
                return true;
            }
        }
    }
    return false;
}

inline bool updateUntilConversation(
    WorldScene& world,
    std::vector<std::int32_t>* audio = nullptr,
    std::int32_t maximum_updates = 2000) {
    for (std::int32_t update = 0;
         update < maximum_updates && !world.conversationActive();
         ++update) {
        world.update();
        std::vector<std::int32_t> samples =
            world.takeAudioSamples();
        if (audio) {
            audio->insert(
                audio->end(), samples.begin(), samples.end());
        }
    }
    return world.conversationActive();
}

inline bool openNpcConversation(
    WorldScene& world,
    std::int32_t npc_id,
    std::vector<std::int32_t>* audio = nullptr) {
    ScreenPosition pointer;
    for (std::int32_t update = 0; update < 2000; ++update) {
        if (findNpcPointerPoint(world, npc_id, pointer)) {
            world.cancelPlayerMovement();
            return world.commandWorldInteraction(
                       pointer.x, pointer.y) &&
                   updateUntilConversation(world, audio);
        }
        const auto npc = std::find_if(
            world.npcs().begin(),
            world.npcs().end(),
            [npc_id](const NpcActor& candidate) {
                return candidate.id() == npc_id;
            });
        if (npc == world.npcs().end()) {
            return false;
        }
        if (update % 30 == 0) {
            const ScreenPosition target =
                calculateRealPosition(npc->position());
            world.commandPlayerMovement(
                target.x - world.cameraScreenX(),
                target.y - world.cameraScreenY());
        }
        world.update();
        std::vector<std::int32_t> samples =
            world.takeAudioSamples();
        if (audio) {
            audio->insert(
                audio->end(), samples.begin(), samples.end());
        }
    }
    return false;
}

inline bool loadSavedFixture(
    const std::filesystem::path& data_root,
    const std::filesystem::path& save_path,
    WorldScene& world,
    std::string& error) {
    PlayerLoadRequest request;
    request.source = PlayerDataSource::retail_save;
    request.save_path = save_path;
    return world.loadInitialScenario(data_root, request, &error);
}

inline bool raiseToLevel(
    PlayerData& player,
    std::int32_t level,
    const TableDatabase& tables) {
    while (player.level() < level) {
        const std::int32_t threshold =
            player.experienceThreshold(tables);
        if (threshold <= player.experience()) {
            return false;
        }
        player.addExperience(threshold - player.experience());
        if (!player.applyLevelThreshold(tables)) {
            return false;
        }
    }
    return player.level() == level;
}

inline bool markScenarioEnemiesDefeated(
    WorldScene& world,
    std::int32_t first_id,
    std::int32_t last_id) {
    std::vector<EnemyActor>& enemies =
        const_cast<std::vector<EnemyActor>&>(world.enemies());
    std::int32_t defeated = 0;
    for (EnemyActor& enemy : enemies) {
        if (enemy.id() < first_id || enemy.id() > last_id) {
            continue;
        }
        EnemyDamageReceiverState state =
            enemy.damageReceiverState(world.scenarioId());
        state.current_life = 0;
        state.presentation_action = 11;
        state.presentation_counter = 0;
        state.action_lock = 1;
        enemy.applyDamageReceiverState(state);
        ++defeated;
    }
    return defeated == last_id - first_id + 1;
}

inline bool scriptedObjectVisible(
    const WorldScene& world,
    std::int32_t character_number,
    bool visible) {
    const auto found = std::find_if(
        world.scenarioObjects().begin(),
        world.scenarioObjects().end(),
        [character_number](const ScenarioObjectActor& object) {
            return object.characterNumber() == character_number;
        });
    return found != world.scenarioObjects().end() &&
           found->visible() == visible;
}

}  // namespace osf::test

#endif
