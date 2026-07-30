#include "core/retail_random.hpp"
#include "items/item_database.hpp"
#include "items/player_equipment.hpp"
#include "items/player_special_items.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "world/enemy_effect_impact.hpp"
#include "world/player_damage_receiver.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
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

void writeStateI32(
    std::vector<std::uint8_t>& state,
    std::size_t word,
    std::int32_t value) {
    const std::size_t offset = word * 4u;
    const std::uint32_t raw =
        static_cast<std::uint32_t>(value);
    state[offset] =
        static_cast<std::uint8_t>(raw);
    state[offset + 1] =
        static_cast<std::uint8_t>(raw >> 8u);
    state[offset + 2] =
        static_cast<std::uint8_t>(raw >> 16u);
    state[offset + 3] =
        static_cast<std::uint8_t>(raw >> 24u);
}

osf::TableDatabase retailTables() {
    osf::TableDatabase tables;
    std::string error;
    if (!tables.load(
            std::string(OPENSHADOWFLARE_SOURCE_DIR) +
                "/tmp/ShadowFlare/System/Game/Parameter/Table.Tbd",
            &error)) {
        std::cerr
            << "The retail Table.Tbd fixture could not be decoded: "
            << error << '\n';
    }
    return tables;
}

osf::PlayerDamageReceiverState state() {
    osf::PlayerDamageReceiverState state;
    state.defense.character_number = 3;
    state.defense.attack = 30;
    state.defense.physical_defense = 20;
    state.defense.magical_defense = 10;
    state.position = {100, 50};
    state.judgement = {-10, -20, 9, 19};
    state.effect_owner_identifier = 103;
    state.level = 7;
    state.maximum_life = 100;
    state.current_life = 100;
    state.maximum_mana = 80;
    state.current_mana = 80;
    state.presentation_action = 0;
    return state;
}

osf::CombatPacket directPacket(
    std::int32_t damage) {
    osf::CombatPacket packet;
    packet.write(0, 2);
    packet.write(1, 0);
    packet.write(2, 14000000);
    packet.write(3, 0);
    packet.write(4, damage);
    packet.write(32, 0);
    packet.write(34, -1);
    packet.write(35, 8);
    packet.write(37, 1);
    packet.write(38, 0);
    packet.write(40, 0);
    packet.write(41, 0);
    packet.write(42, 0);
    packet.write(43, 1);
    packet.write(44, 0);
    packet.write(72, 0);
    packet.write(74, -1);
    packet.write(75, 8);
    packet.write(76, 0);
    return packet;
}

bool testOwnershipAndEnergyShield() {
    const osf::TableDatabase tables = retailTables();
    const osf::ItemDatabase items;
    osf::PlayerDamageReceiverState receiving = state();
    receiving.energy_shield_active = true;
    receiving.spell_levels[9] = 1;

    osf::PlayerDamageReceiverContext local;
    local.local_player_character_number = 3;
    osf::RetailRandom random(1);
    const osf::PlayerDamageReceiverResult result =
        osf::resolvePlayerDamage(
            receiving,
            directPacket(25),
            {},
            local,
            items,
            tables,
            random);
    if (!check(
            result.valid &&
                result.damage.damage == 25 &&
                result.state.current_life == 100 &&
                result.state.current_mana == 55,
            "Energy Shield did not route ordinary retail damage "
            "from life to mana.")) {
        return false;
    }

    osf::PlayerDamageReceiverContext remote;
    remote.local_player_character_number = 9;
    osf::RetailRandom remote_random(1);
    const osf::PlayerDamageReceiverResult remote_result =
        osf::resolvePlayerDamage(
            receiving,
            directPacket(25),
            {},
            remote,
            items,
            tables,
            remote_random);
    return check(
        remote_result.valid &&
            remote_result.state.current_life == 100 &&
            remote_result.state.current_mana == 80,
        "A non-owned player applied local life or mana changes.");
}

bool testMagicShieldCostAndTraining() {
    const osf::TableDatabase tables = retailTables();
    const osf::ItemDatabase items;
    osf::PlayerDamageReceiverState receiving = state();
    receiving.magic_shield_active = true;
    receiving.spell_levels[18] = 1;
    receiving.selected_magic = 18;
    osf::CombatPacket packet = directPacket(40);
    packet.write(1, 3);

    const std::int32_t reduction =
        osf::retailEffectParameter(
            tables, 18, 1, 0);
    const std::int32_t cost =
        osf::retailEffectParameter(
            tables, 18, 1, 2);
    osf::PlayerDamageReceiverContext context;
    context.local_player_character_number = 3;
    osf::RetailRandom random(7);
    const osf::PlayerDamageReceiverResult result =
        osf::resolvePlayerDamage(
            receiving,
            packet,
            {},
            context,
            items,
            tables,
            random);
    const std::int32_t expected_damage =
        std::max<std::int32_t>(
            (100 - reduction) * 40 / 100, 1);
    return check(
        result.valid &&
            result.damage.damage == expected_damage &&
            result.state.current_life ==
                100 - expected_damage &&
            result.state.current_mana ==
                std::max<std::int32_t>(80 - cost, 0) &&
            !result.effects.empty() &&
            result.effects.front().effect_number == 21029 &&
            !result.audio_samples.empty() &&
            result.audio_samples.front() == 60 &&
            (expected_damage < 20 ||
             (!result.spell_training.empty() &&
              result.spell_training.front().spell_number == 18)),
        "Magic Shield did not preserve retail reduction, mana, "
        "training, visual, or audio behavior.");
}

bool testRevivalAndDurability() {
    const osf::TableDatabase tables = retailTables();
    const osf::ItemDatabase items;
    osf::PlayerDamageReceiverState receiving = state();
    receiving.current_life = 10;
    receiving.current_mana = 2;
    osf::InventoryItem revival;
    revival.category = 4;
    revival.definition_id = 98000000;
    if (!check(
            receiving.special_items.place(
                revival, 0, 0).accepted,
            "The revival-item fixture could not be placed.")) {
        return false;
    }

    osf::ItemDefinition helmet;
    helmet.category = 1;
    helmet.id = 1;
    helmet.subtype = 0;
    osf::InventoryItem equipped =
        osf::makeInventoryItem(helmet);
    equipped.durability = 1;
    if (!check(
            receiving.equipment.place(
                osf::EquipmentSlot::helmet,
                equipped,
                helmet,
                99).accepted,
            "The durability fixture could not be equipped.")) {
        return false;
    }

    std::uint32_t seed = 1;
    for (;; ++seed) {
        osf::RetailRandom probe(seed);
        if (probe.next() % 100 < 20) {
            break;
        }
    }
    osf::PlayerDamageReceiverContext context;
    context.local_player_character_number = 3;
    osf::RetailRandom random(seed);
    const osf::PlayerDamageReceiverResult result =
        osf::resolvePlayerDamage(
            receiving,
            directPacket(20),
            {},
            context,
            items,
            tables,
            random);
    const osf::InventoryItem* result_helmet =
        result.state.equipment.item(
            osf::EquipmentSlot::helmet);
    return check(
        result.valid &&
            result.revived &&
            result.state.current_life == 100 &&
            result.state.current_mana == 80 &&
            result.state.special_items.items().empty() &&
            result_helmet &&
            result_helmet->durability == 0 &&
            result.equipment_sync_requested &&
            result.derived_values_refresh_requested &&
            result.effects.front().effect_number == 21020 &&
            result.audio_samples.front() == 17,
        "Revival or the following retail durability checks "
        "differed.");
}

bool testEquipmentReflection() {
    const osf::TableDatabase tables = retailTables();
    const osf::ItemDatabase items;
    osf::PlayerDamageReceiverState receiving = state();

    osf::ItemDefinition body;
    body.category = 1;
    body.id = 2;
    body.subtype = 1;
    osf::InventoryItem equipped =
        osf::makeInventoryItem(body);
    equipped.durability = 100;
    equipped.retail_state.resize(200);
    writeStateI32(equipped.retail_state, 20, 100);
    writeStateI32(equipped.retail_state, 21, 50);
    if (!check(
            receiving.equipment.place(
                osf::EquipmentSlot::body,
                equipped,
                body,
                99).accepted,
            "The reflection fixture could not be equipped.")) {
        return false;
    }

    osf::CombatPacket packet = directPacket(30);
    packet.write(38, 1);
    osf::PlayerDamageReceiverContext context;
    context.local_player_character_number = 3;
    context.reflection_target.found = true;
    context.reflection_target.character_number = 14000007;
    context.reflection_target.actor_kind = 2;
    context.reflection_target.active_value = 1;
    context.reflection_target.position = {120, 50};
    osf::RetailRandom random(9);
    const osf::PlayerDamageReceiverResult result =
        osf::resolvePlayerDamage(
            receiving,
            packet,
            {},
            context,
            items,
            tables,
            random);
    return check(
        result.valid &&
            result.reflection.valid &&
            result.reflection.target_character_number ==
                14000007 &&
            result.reflection.packet[4] == 15 &&
            result.reflection.packet[37] == 1 &&
            result.reflection.packet[72] == 1 &&
            result.reflection.packet[74] == -1 &&
            !result.effects.empty() &&
            result.effects.front().effect_number == 20014 &&
            result.audio_samples.front() == 60,
        "Equipment reflection did not build the retail callback, "
        "effect, and sample.");
}

bool testCounterBurstAndReaction() {
    const osf::TableDatabase tables = retailTables();
    const osf::ItemDatabase items;
    osf::PlayerDamageReceiverState receiving = state();
    receiving.counter_burst_active = true;
    receiving.spell_levels[19] = 1;
    receiving.selected_magic = 19;
    osf::CombatPacket reflected = directPacket(30);
    reflected.write(38, 1);

    osf::PlayerDamageReceiverContext context;
    context.local_player_character_number = 3;
    context.reflection_target.found = true;
    context.reflection_target.character_number = 14000008;
    context.reflection_target.actor_kind = 2;
    context.reflection_target.active_value = 1;
    context.reflection_target.damage_scale_value = 100;
    context.reflection_target.position = {80, 50};
    const std::int32_t percent =
        osf::retailEffectParameter(
            tables, 19, 1, 0);
    const std::int32_t cost =
        osf::retailEffectParameter(
            tables, 19, 1, 2);
    osf::RetailRandom reflection_random(11);
    const osf::PlayerDamageReceiverResult result =
        osf::resolvePlayerDamage(
            receiving,
            reflected,
            {},
            context,
            items,
            tables,
            reflection_random);
    if (!check(
            result.valid &&
                result.reflection.valid &&
                result.reflection.packet[4] ==
                    std::max<std::int32_t>(
                        30 * percent / 100 / 2, 1) &&
                result.state.current_mana ==
                    std::max<std::int32_t>(
                        80 - cost, 0) &&
                !result.effects.empty() &&
                result.effects.front().effect_number == 21030 &&
                !result.spell_training.empty() &&
                result.spell_training.front().spell_number == 19,
            "Counter Burst did not add its retail reflection, "
            "mana cost, effect, and training request.")) {
        return false;
    }

    osf::CombatPacket reaction = directPacket(10);
    reaction.write(40, 0);
    reaction.write(41, 100);
    reaction.write(43, 30);
    osf::RetailRandom reaction_random(4);
    const osf::PlayerDamageReceiverResult reaction_result =
        osf::resolvePlayerDamage(
            state(),
            reaction,
            {0, 50},
            context,
            items,
            tables,
            reaction_random);
    return check(
        reaction_result.valid &&
            reaction_result.state.presentation_action == 4 &&
            reaction_result.state.reaction_stage == 0 &&
            reaction_result.state.reaction_duration == 15 &&
            !reaction_result.state.reaction_motion &&
            reaction_result.state.action_lock == 1,
        "Player hit reaction did not preserve its chance, "
        "non-motion cap, and action transition.");
}

bool testDeathWithoutRevival() {
    const osf::TableDatabase tables = retailTables();
    const osf::ItemDatabase items;
    osf::PlayerDamageReceiverState receiving = state();
    receiving.current_life = 5;
    osf::PlayerDamageReceiverContext context;
    context.local_player_character_number = 3;
    osf::RetailRandom random(2);
    const osf::PlayerDamageReceiverResult result =
        osf::resolvePlayerDamage(
            receiving,
            directPacket(10),
            {},
            context,
            items,
            tables,
            random);
    return check(
        result.valid &&
            !result.revived &&
            result.state.current_life == -5 &&
            result.state.presentation_action == 5 &&
            result.state.presentation_counter == 0 &&
            result.state.action_lock == 1 &&
            result.state.event_number == 4,
        "Lethal player damage did not select the retail death "
        "presentation and event.");
}

}  // namespace

int main() {
    if (!testOwnershipAndEnergyShield() ||
        !testMagicShieldCostAndTraining() ||
        !testRevivalAndDurability() ||
        !testEquipmentReflection() ||
        !testCounterBurstAndReaction() ||
        !testDeathWithoutRevival()) {
        return 1;
    }
    return 0;
}
