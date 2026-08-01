#include "items/item_database.hpp"
#include "items/player_belt.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"
#include "items/player_special_items.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "world/player_data.hpp"
#include "world/player_magic.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_items.hpp"
#include "world/retail_save_magic.hpp"
#include "world/retail_save_mines.hpp"
#include "world/retail_save_progress.hpp"

#include <cstddef>
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

osf::PlayerMagicState fixtureState() {
    osf::PlayerMagicState state;
    for (std::size_t index = 0;
         index < state.availability.size();
         ++index) {
        state.availability[index] =
            index % 3u == 0u ? 3 : 0;
        state.levels[index] =
            static_cast<std::int32_t>(index + 1u);
        state.experience[index] =
            static_cast<std::int32_t>(index * 17u);
    }
    for (std::size_t index = 0;
         index < state.bar_slots.size();
         ++index) {
        state.bar_slots[index] =
            static_cast<std::int32_t>(index * 2u);
    }
    return state;
}

}  // namespace

int main() {
    osf::PlayerMagic fresh;
    fresh.initializeNew();
    bool initialized = true;
    for (std::int32_t spell = 0;
         spell <
             static_cast<std::int32_t>(
                 osf::PlayerMagic::spell_count);
         ++spell) {
        initialized =
            initialized &&
            fresh.availability(spell) == 0 &&
            fresh.level(spell) == 1 &&
            fresh.experience(spell) == 0 &&
            !fresh.learned(spell);
    }
    for (std::int32_t slot = 0;
         slot <
             static_cast<std::int32_t>(
                 osf::PlayerMagic::bar_slot_count);
         ++slot) {
        initialized =
            initialized &&
            fresh.barSlot(slot) == -1;
    }
    if (!check(
            initialized,
            "New-character magic does not match FUN_00440f70.")) {
        return 1;
    }

    fresh.setAllSpellsAvailable(true);
    if (!check(
            fresh.allSpellsAvailable() &&
                fresh.learned(19) &&
                fresh.assignBarSlot(0, 19) &&
                fresh.barSlot(0) == 19 &&
                fresh.selectSpell(19) &&
                fresh.state().availability[19] == 0 &&
                fresh.state().bar_slots[0] == -1,
            "The temporary all-spells override changed the saved magic "
            "state or did not expose Counter Burst.")) {
        return 1;
    }
    fresh.setAllSpellsAvailable(false);
    if (!check(
            !fresh.allSpellsAvailable() &&
                !fresh.learned(19) &&
                fresh.barSlot(0) == -1 &&
                fresh.selectedSpell() == -1 &&
                fresh.state().availability[19] == 0 &&
                fresh.state().bar_slots[0] == -1,
            "Disabling the all-spells override leaked its selection or "
            "temporary bar into normal play.")) {
        return 1;
    }

    fresh.setAllSpellsAvailable(true);
    if (!check(
            !fresh.permanentlyLearned(20) &&
                fresh.learnPermanently(20) &&
                fresh.permanentlyLearned(20) &&
                fresh.state().availability[20] == 3 &&
                !fresh.learnPermanently(-1) &&
                !fresh.learnPermanently(
                    static_cast<std::int32_t>(
                        osf::PlayerMagic::spell_count)),
            "Permanent spell learning did not update the saved retail "
            "availability independently of the debug override.")) {
        return 1;
    }
    fresh.setAllSpellsAvailable(false);
    if (!check(
            fresh.learned(20) &&
                fresh.permanentlyLearned(20),
            "A permanently learned spell disappeared with the debug "
            "override.")) {
        return 1;
    }

    std::vector<std::uint8_t> payload(
        osf::PlayerData::retail_record_size, 0x5a);
    const std::size_t items_end = payload.size();
    osf::RetailSaveProgress progress{
        {1, 2, 3},
        {4, 5},
        {6, 7, 8, 9},
        true,
    };
    std::size_t progress_end = 0;
    std::string error;
    if (!check(
            osf::replaceRetailProgress(
                payload,
                items_end,
                progress,
                &progress_end,
                &error),
            "The progress fixture could not be serialized.")) {
        std::cerr << error << '\n';
        return 1;
    }
    osf::RetailSaveProgress restored_progress;
    std::size_t restored_progress_end = 0;
    if (!check(
            osf::restoreRetailProgress(
                payload,
                items_end,
                restored_progress,
                &restored_progress_end,
                &error) &&
                restored_progress_end == progress_end &&
                restored_progress.quest_flags ==
                    progress.quest_flags &&
                restored_progress.transport_flags ==
                    progress.transport_flags &&
                restored_progress.script_state_flags ==
                    progress.script_state_flags &&
                restored_progress.running,
            "The retail type-12, type-10, and type-11 progress arrays "
            "did not round-trip in executable order.")) {
        std::cerr << error << '\n';
        return 1;
    }
    std::vector<std::uint8_t> legacy_payload = payload;
    osf::RetailSaveProgress legacy_written{
        progress.script_state_flags,
        progress.transport_flags,
        progress.quest_flags,
        progress.running,
    };
    if (!check(
            osf::replaceRetailProgress(
                legacy_payload,
                items_end,
                legacy_written,
                nullptr,
                &error) &&
                legacy_payload.size() >= 8,
            "The legacy progress-order fixture could not be serialized.")) {
        std::cerr << error << '\n';
        return 1;
    }
    // Portable extension version one accidentally wrote type 11 first and
    // type 12 third. Mark the deliberately swapped fixture as version one;
    // the reader must migrate it without losing development saves.
    legacy_payload[legacy_payload.size() - 8] = 1;
    legacy_payload[legacy_payload.size() - 7] = 0;
    legacy_payload[legacy_payload.size() - 6] = 0;
    legacy_payload[legacy_payload.size() - 5] = 0;
    osf::RetailSaveProgress migrated_progress;
    if (!check(
            osf::restoreRetailProgress(
                legacy_payload,
                items_end,
                migrated_progress,
                nullptr,
                &error) &&
                migrated_progress.quest_flags ==
                    progress.quest_flags &&
                migrated_progress.transport_flags ==
                    progress.transport_flags &&
                migrated_progress.script_state_flags ==
                    progress.script_state_flags &&
                migrated_progress.running,
            "Portable version-one progress did not migrate from its "
            "old swapped flag order.")) {
        std::cerr << error << '\n';
        return 1;
    }

    osf::PlayerMagic fixture;
    fixture.restore(fixtureState());
    std::size_t magic_end = 0;
    if (!check(
            osf::replaceRetailMagic(
                payload,
                progress_end,
                fixture,
                &magic_end,
                &error),
            "The retail magic fixture could not be serialized.")) {
        std::cerr << error << '\n';
        return 1;
    }
    if (!check(
            magic_end - progress_end ==
                4u +
                    osf::PlayerMagic::spell_count * 12u +
                    osf::PlayerMagic::bar_slot_count * 4u,
            "The retail magic stream has the wrong serialized size.")) {
        return 1;
    }

    osf::PlayerMagic restored;
    restored.initializeNew();
    std::size_t restored_end = 0;
    if (!check(
            osf::restoreRetailMagic(
                payload,
                progress_end,
                restored,
                &restored_end,
                &error) &&
                restored_end == magic_end &&
                restored.state().availability ==
                    fixture.state().availability &&
                restored.state().levels ==
                    fixture.state().levels &&
                restored.state().experience ==
                    fixture.state().experience &&
                restored.state().bar_slots ==
                    fixture.state().bar_slots,
            "The retail magic stream did not round-trip exactly.")) {
        std::cerr << error << '\n';
        return 1;
    }

    std::size_t mine_end = 0;
    if (!check(
            osf::replaceRetailMineCount(
                payload,
                magic_end,
                7,
                &mine_end,
                &error),
            "The portable mine-count fixture could not be serialized.")) {
        std::cerr << error << '\n';
        return 1;
    }
    std::int32_t restored_mines = 5;
    if (!check(
            osf::restoreRetailMineCount(
                payload,
                magic_end,
                restored_mines,
                nullptr,
                &error) &&
                restored_mines == 7 &&
                mine_end == magic_end,
            "The portable mine count did not round-trip after magic.")) {
        std::cerr << error << '\n';
        return 1;
    }
    // Progress owns the running bit, but rewriting it must preserve the
    // mine field owned by the adjacent save component.
    progress.running = false;
    if (!check(
            osf::replaceRetailProgress(
                payload,
                items_end,
                progress,
                &progress_end,
                &error) &&
                osf::restoreRetailMineCount(
                    payload,
                    magic_end,
                    restored_mines,
                    nullptr,
                    &error) &&
                restored_mines == 7,
            "Rewriting progress discarded the portable mine count.")) {
        std::cerr << error << '\n';
        return 1;
    }

    const osf::PlayerMagicState before_bad_restore =
        restored.state();
    std::vector<std::uint8_t> truncated(
        payload.begin(),
        payload.begin() +
            static_cast<std::ptrdiff_t>(magic_end - 1u));
    if (!check(
            !osf::restoreRetailMagic(
                truncated,
                progress_end,
                restored,
                nullptr,
                &error) &&
                restored.state().availability ==
                    before_bad_restore.availability &&
                restored.state().levels ==
                    before_bad_restore.levels &&
                restored.state().experience ==
                    before_bad_restore.experience &&
                restored.state().bar_slots ==
                    before_bad_restore.bar_slots,
            "A truncated retail magic stream changed live state.")) {
        return 1;
    }

    osf::ItemDatabase items;
    const std::filesystem::path game_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!check(
            items.load(
                game_root / "System" / "Game" /
                    "Parameter" / "Item.Ibn",
                &error),
            "The retail item database could not be loaded.")) {
        std::cerr << error << '\n';
        return 1;
    }
    osf::TableDatabase tables;
    if (!check(
            tables.load(
                game_root / "System" / "Game" /
                    "Parameter" / "Table.Tbd",
                &error),
            "The retail spell-training table could not be loaded.")) {
        std::cerr << error << '\n';
        return 1;
    }

    osf::PlayerMagicState training_state;
    training_state.availability.fill(3);
    training_state.levels.fill(1);
    training_state.experience.fill(0);
    training_state.experience[1] = 28;
    training_state.bar_slots.fill(-1);
    osf::PlayerMagic training;
    training.restore(training_state);
    const bool first_level =
        training.train(1, false, tables);
    const bool second_level =
        training.train(1, false, tables);
    if (!check(
            !first_level &&
                second_level &&
                training.level(1) == 2 &&
                training.experience(1) == 0 &&
                !training.train(7, false, tables) &&
                training.experience(7) == 0 &&
                !training.train(1, true, tables) &&
                training.experience(1) == 0 &&
                !training.train(7, true, tables) &&
                training.experience(7) == 1,
            "Spell practice did not follow FUN_0044f6f0's "
            "threshold and companion-mode split.")) {
        return 1;
    }

    for (std::int32_t slot = 0; slot < 4; ++slot) {
        const std::filesystem::path save =
            game_root / "Save" /
            (std::string("000") +
             static_cast<char>('0' + slot) +
             ".Ssv");
        std::vector<std::uint8_t> retail_payload;
        if (!osf::readRetailSavePayload(
                save, retail_payload, &error)) {
            continue;
        }
        osf::PlayerData player;
        osf::PlayerInventory inventory;
        osf::PlayerEquipment equipment;
        osf::PlayerBelt belt;
        osf::PlayerSpecialItems special_items;
        std::size_t retail_items_end = 0;
        if (!player.loadRetailSave(save, &error) ||
            !osf::restoreRetailOwnedItems(
                retail_payload,
                items,
                player.level(),
                inventory,
                equipment,
                belt,
                special_items,
                &retail_items_end,
                &error)) {
            std::cerr << error << '\n';
            return 1;
        }
        osf::RetailSaveProgress retail_progress;
        std::size_t retail_progress_end = 0;
        osf::PlayerMagic retail_magic;
        retail_magic.initializeNew();
        if (!osf::restoreRetailProgress(
                retail_payload,
                retail_items_end,
                retail_progress,
                &retail_progress_end,
                &error) ||
            !osf::restoreRetailMagic(
                retail_payload,
                retail_progress_end,
                retail_magic,
                nullptr,
                &error)) {
            std::cerr << error << '\n';
            return 1;
        }
    }
    return 0;
}
