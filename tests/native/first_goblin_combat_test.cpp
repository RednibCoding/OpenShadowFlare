#include "items/item_audio.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "world/enemy_presentation_audio.hpp"
#include "world/retail_save_file.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

const osf::EnemyActor* findEnemy(
    const osf::WorldScene& world,
    std::int32_t enemy_id) {
    const auto found = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [enemy_id](const osf::EnemyActor& enemy) {
            return enemy.id() == enemy_id;
        });
    return found == world.enemies().end()
        ? nullptr
        : &*found;
}

bool findEnemyPointer(
    osf::WorldScene& world,
    std::int32_t enemy_id,
    osf::ScreenPosition& point) {
    const osf::EnemyActor* enemy =
        findEnemy(world, enemy_id);
    if (!enemy || enemy->currentLife() < 1 ||
        !enemy->visible() || !enemy->pointerEnabled()) {
        return false;
    }
    const osf::ScreenPosition anchor =
        osf::calculateRealPosition(enemy->position());
    for (std::int32_t y = -enemy->labelHeight();
         y <= 24;
         ++y) {
        for (std::int32_t x = -64; x <= 64; ++x) {
            point = {
                anchor.x - world.cameraScreenX() + x,
                anchor.y - world.cameraScreenY() + y,
            };
            if (point.x < 0 || point.x >= 640 ||
                point.y < 0 || point.y >= 480) {
                continue;
            }
            world.updatePointerHover(point.x, point.y);
            if (world.hoveredEnemyId() == enemy_id) {
                return true;
            }
        }
    }
    return false;
}

bool approachVisibleEnemy(
    osf::WorldScene& world,
    std::int32_t enemy_id,
    osf::ScreenPosition& point) {
    for (std::int32_t update = 0;
         update < 1000;
         ++update) {
        if (findEnemyPointer(world, enemy_id, point)) {
            world.cancelPlayerMovement();
            return true;
        }
        const osf::EnemyActor* enemy =
            findEnemy(world, enemy_id);
        if (!enemy) {
            return false;
        }
        if (update % 30 == 0) {
            const osf::ScreenPosition target =
                osf::calculateRealPosition(
                    enemy->position());
            world.commandPlayerMovement(
                target.x - world.cameraScreenX(),
                target.y - world.cameraScreenY());
        }
        world.update();
        world.takeAudioSamples();
    }
    return false;
}

bool findGroundItemPointer(
    osf::WorldScene& world,
    std::int32_t item_id,
    osf::ScreenPosition& point) {
    const auto found = std::find_if(
        world.groundItems().begin(),
        world.groundItems().end(),
        [item_id](const osf::GroundItem& item) {
            return item.id == item_id;
        });
    if (found == world.groundItems().end()) {
        return false;
    }
    const osf::ScreenPosition anchor =
        osf::calculateRealPosition(found->position);
    for (std::int32_t y = -96; y <= 64; ++y) {
        for (std::int32_t x = -96; x <= 96; ++x) {
            point = {
                anchor.x - world.cameraScreenX() + x,
                anchor.y - world.cameraScreenY() + y,
            };
            if (point.x < 0 || point.x >= 640 ||
                point.y < 0 || point.y >= 480) {
                continue;
            }
            world.updatePointerHover(point.x, point.y);
            if (world.hoveredGroundItemId() == item_id) {
                return true;
            }
        }
    }
    return false;
}

bool containsSample(
    const std::vector<std::int32_t>& samples,
    std::int32_t sample) {
    return std::find(
               samples.begin(), samples.end(), sample) !=
           samples.end();
}

bool testFirstGoblinCombat() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(
            OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "00000001")) {
        return true;
    }

    osf::WorldScene world;
    osf::PlayerLoadRequest player;
    player.name = "GoblinTest";
    std::string error;
    if (!check(
            world.loadInitialScenario(
                data_root,
                player,
                {1, 0, 0},
                &error),
            "The first outdoor combat map could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    constexpr std::int32_t kFirstGoblinId = 101;
    const osf::EnemyActor* goblin =
        findEnemy(world, kFirstGoblinId);
    const osf::ItemDefinition* starter_sword =
        world.itemDatabase().find(0, 0);
    if (!check(
            goblin &&
                goblin->name() == "Goblin" &&
                goblin->maximumLife() == 40 &&
                goblin->experienceReward() == 1 &&
                goblin->lootTableRow() == 0 &&
                goblin->goldDropChance() == 10 &&
                goblin->goldMinimum() == 10 &&
                goblin->goldMaximum() == 20 &&
                starter_sword &&
                starter_sword->name == "Short Sword" &&
                world.playerEquipment()
                    .place(
                        osf::EquipmentSlot::main_hand,
                        osf::makeInventoryItem(
                            *starter_sword),
                        *starter_sword,
                        world.playerData().level())
                    .accepted,
            "The authored first Goblin or Ostare starter sword "
            "fixture differs from retail.")) {
        return false;
    }
    world.refreshPlayerAppearance();

    const std::size_t ground_before =
        world.groundItems().size();
    const std::int32_t experience_before =
        world.playerData().experience();
    const std::int32_t kills_before =
        world.playerData().totalKillCount();
    const std::int32_t death_sample =
        osf::retailEnemyDeathSample(
            goblin->resourceId());
    osf::ScreenPosition pointer;
    if (!check(
            approachVisibleEnemy(
                world, kFirstGoblinId, pointer),
            "The first Goblin could not be approached through "
            "the live outdoor map.")) {
        return false;
    }

    bool heard_swing = false;
    bool heard_hit = false;
    bool heard_death =
        death_sample < 0;
    bool saw_hit_effect = false;
    bool saw_death_effect = false;
    for (std::int32_t attack = 0;
         attack < 30;
         ++attack) {
        goblin = findEnemy(world, kFirstGoblinId);
        if (!goblin || goblin->currentLife() == 0) {
            break;
        }
        if (!findEnemyPointer(
                world, kFirstGoblinId, pointer)) {
            if (!approachVisibleEnemy(
                    world, kFirstGoblinId, pointer)) {
                return check(
                    false,
                    "The live Goblin became unreachable before "
                    "the fight ended.");
            }
        }
        if (!world.commandWorldInteraction(
                pointer.x, pointer.y)) {
            return check(
                false,
                "The live Goblin rejected a valid attack click.");
        }

        bool impact_seen = false;
        for (std::int32_t update = 0;
             update < 1000;
             ++update) {
            world.update();
            impact_seen =
                impact_seen ||
                world.takePlayerAttackImpactTargetId() ==
                    kFirstGoblinId;
            const std::vector<std::int32_t> samples =
                world.takeAudioSamples();
            heard_swing =
                heard_swing ||
                containsSample(samples, 1);
            heard_hit =
                heard_hit ||
                containsSample(samples, 6);
            heard_death =
                heard_death ||
                containsSample(
                    samples, death_sample);
            saw_hit_effect =
                saw_hit_effect ||
                std::any_of(
                    world.combatEffects().begin(),
                    world.combatEffects().end(),
                    [](const osf::CombatEffectActor& effect) {
                        return effect.effectNumber() >=
                                   21000 &&
                               effect.effectNumber() <=
                                   21006;
                    });
            saw_death_effect =
                saw_death_effect ||
                std::any_of(
                    world.combatEffects().begin(),
                    world.combatEffects().end(),
                    [](const osf::CombatEffectActor& effect) {
                        return effect.effectNumber() ==
                               21010;
                    });
            goblin = findEnemy(world, kFirstGoblinId);
            if (!goblin || goblin->currentLife() == 0 ||
                (impact_seen &&
                 world.playerMotion() ==
                     osf::PlayerMotion::idle)) {
                break;
            }
        }
    }

    for (std::int32_t update = 0; update < 2; ++update) {
        world.update();
        const std::vector<std::int32_t> samples =
            world.takeAudioSamples();
        heard_death =
            heard_death ||
            containsSample(samples, death_sample);
        saw_death_effect =
            saw_death_effect ||
            std::any_of(
                world.combatEffects().begin(),
                world.combatEffects().end(),
                [](const osf::CombatEffectActor& effect) {
                    return effect.effectNumber() ==
                           21010;
                });
    }

    goblin = findEnemy(world, kFirstGoblinId);
    const bool defeated =
        goblin &&
        goblin->currentLife() == 0 &&
        world.playerData().experience() ==
            experience_before + 1 &&
        world.playerData().totalKillCount() ==
            kills_before + 1 &&
        heard_swing &&
        heard_hit &&
        heard_death &&
        saw_hit_effect &&
        saw_death_effect &&
        world.groundItems().size() > ground_before;
    if (!defeated) {
        std::cerr
            << "life=" << (goblin
                    ? goblin->currentLife()
                    : -1)
            << " experience="
            << world.playerData().experience()
            << " kills="
            << world.playerData().totalKillCount()
            << " swing=" << heard_swing
            << " hit=" << heard_hit
            << " death-audio=" << heard_death
            << " hit-effect=" << saw_hit_effect
            << " death-effect=" << saw_death_effect
            << " ground=" << world.groundItems().size()
            << '\n';
    }
    if (!check(
            defeated,
            "The first live Goblin did not complete its retail "
            "attack, death, reward, and loot path.")) {
        return false;
    }

    std::vector<std::int32_t> drop_ids;
    std::vector<std::int32_t> expected_pickup_samples;
    std::vector<std::int32_t> pickup_samples;
    bool heard_landing = false;
    for (const osf::GroundItem& item :
         world.groundItems()) {
        if (item.id >= 0) {
            drop_ids.push_back(item.id);
            const osf::ItemDefinition* definition =
                world.itemDatabase().find(
                    item.item.category,
                    item.item.definition_id);
            if (definition) {
                expected_pickup_samples.push_back(
                    osf::retailItemMoveSound(
                        *definition));
            }
        }
    }
    for (std::int32_t update = 0;
         update < 100;
         ++update) {
        world.update();
        const std::vector<std::int32_t> samples =
            world.takeAudioSamples();
        heard_landing =
            heard_landing ||
            containsSample(samples, 15) ||
            containsSample(samples, 85) ||
            containsSample(samples, 93);
    }

    const std::size_t inventory_before_pickup =
        world.playerInventory().items().size();
    for (std::int32_t drop_id : drop_ids) {
        osf::ScreenPosition item_pointer;
        if (!check(
                findGroundItemPointer(
                    world, drop_id, item_pointer) &&
                    world.commandWorldInteraction(
                        item_pointer.x,
                        item_pointer.y),
                "A first-Goblin drop could not be selected.")) {
            return false;
        }
        for (std::int32_t update = 0;
             update < 2000;
             ++update) {
            world.update();
            const std::vector<std::int32_t> samples =
                world.takeAudioSamples();
            pickup_samples.insert(
                pickup_samples.end(),
                samples.begin(),
                samples.end());
            const auto remaining = std::find_if(
                world.groundItems().begin(),
                world.groundItems().end(),
                [drop_id](const osf::GroundItem& item) {
                    return item.id == drop_id;
                });
            if (remaining == world.groundItems().end()) {
                break;
            }
        }
    }

    const bool heard_every_pickup =
        std::all_of(
            expected_pickup_samples.begin(),
            expected_pickup_samples.end(),
            [&pickup_samples](std::int32_t sample) {
                return containsSample(
                    pickup_samples, sample);
            });
    if (!check(
            world.groundItems().size() == ground_before &&
                world.playerInventory().items().size() >
                    inventory_before_pickup &&
                expected_pickup_samples.size() ==
                    drop_ids.size() &&
                heard_landing &&
                heard_every_pickup,
            "First-Goblin loot did not land, play its move sound, "
            "and transfer into the owned backpack.")) {
        return false;
    }

    const std::filesystem::path save_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_first_goblin_save_test";
    const std::filesystem::path save_path =
        save_root / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(
        save_root, cleanup_error);
    const std::size_t saved_inventory_count =
        world.playerInventory().items().size();
    const std::int32_t saved_gold =
        world.playerInventory().gold();
    const std::int32_t saved_experience =
        world.playerData().experience();
    const std::int32_t saved_kills =
        world.playerData().totalKillCount();
    if (!check(
            osf::writeRetailSave(
                save_path,
                world.playerData(),
                world.itemDatabase(),
                world.playerInventory(),
                world.playerEquipment(),
                world.playerBelt(),
                world.playerSpecialItems(),
                0x41,
                &error),
            "The first-Goblin reward state could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerLoadRequest saved_player;
    saved_player.source =
        osf::PlayerDataSource::retail_save;
    saved_player.save_path = save_path;
    osf::WorldScene restored;
    const bool loaded =
        restored.loadInitialScenario(
            data_root, saved_player, &error);
    std::filesystem::remove_all(
        save_root, cleanup_error);
    return check(
        loaded &&
            restored.playerInventory().items().size() ==
                saved_inventory_count &&
            restored.playerInventory().gold() ==
                saved_gold &&
            restored.playerData().experience() ==
                saved_experience &&
            restored.playerData().totalKillCount() ==
                saved_kills &&
            restored.playerEquipment().item(
                osf::EquipmentSlot::main_hand) &&
            restored.playerEquipment()
                    .item(osf::EquipmentSlot::main_hand)
                    ->definition_id ==
                starter_sword->id,
        "Saving after the first Goblin discarded loot, Gold, "
        "experience, kill credit, or equipped weapon.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    return testFirstGoblinCombat() ? 0 : 1;
}
