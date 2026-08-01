#include "libs/RKC_RPG_AICONTROL/rkc_rpg_aicontrol.hpp"

#include <array>
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

void appendI32(
    std::vector<std::uint8_t>& bytes,
    std::int32_t value) {
    const std::uint32_t data =
        static_cast<std::uint32_t>(value);
    bytes.push_back(static_cast<std::uint8_t>(data));
    bytes.push_back(
        static_cast<std::uint8_t>(data >> 8u));
    bytes.push_back(
        static_cast<std::uint8_t>(data >> 16u));
    bytes.push_back(
        static_cast<std::uint8_t>(data >> 24u));
}

std::vector<std::uint8_t> makeDatabase(
    std::int32_t version) {
    std::vector<std::uint8_t> bytes{
        'R', 'K', 'C', '_', 'A', 'I', 'D', 'A',
        'T', 'A', ' ', 'v',
        '0', '0',
        static_cast<std::uint8_t>('0' + version),
        0x1a,
    };
    appendI32(bytes, 1);
    appendI32(
        bytes,
        static_cast<std::int32_t>(
            osf::kAiControlEventCount));
    appendI32(bytes, 4);
    bytes.insert(bytes.end(), {'T', 'e', 's', 't'});
    if (version > 0) {
        appendI32(bytes, 12);
    }
    for (std::size_t event = 0;
         event < osf::kAiControlEventCount;
         ++event) {
        appendI32(bytes, event == 0 ? 1 : 0);
        if (event != 0) {
            continue;
        }
        appendI32(bytes, 7);
        for (std::int32_t value = 1; value <= 9; ++value) {
            appendI32(bytes, value);
        }
        for (std::int32_t value = -1; value >= -6; --value) {
            appendI32(bytes, value);
        }
    }
    return bytes;
}

bool testSyntheticDatabase() {
    osf::AiControlDatabase database;
    std::string error;
    std::vector<std::uint8_t> bytes = makeDatabase(1);
    if (!check(
            database.decode(
                bytes.data(), bytes.size(), &error),
            "A valid version-one AI database was rejected.")) {
        std::cerr << error << '\n';
        return false;
    }

    const osf::AiControlList* list = database.find("Test");
    const osf::AiEventData* event =
        list ? list->event(0) : nullptr;
    if (!check(
            database.version() == 1 &&
                database.lists().size() == 1 &&
                list &&
                database.list(0) == list &&
                database.indexOf(list) == 0 &&
                list->walkPointSpeed() == 12 &&
                list->events().size() ==
                    osf::kAiControlEventCount &&
                list->actionCount() == 1 &&
                event &&
                event->actions().size() == 1 &&
                event->actions()[0].event_number == 0 &&
                event->actions()[0].action_number == 7 &&
                event->actions()[0].parameters.front() == 1 &&
                event->actions()[0].parameters.back() == 9 &&
                event->actions()[0].conditions.front() == -1 &&
                event->actions()[0].conditions.back() == -6 &&
                !list->event(-1) &&
                !list->event(18) &&
                !database.list(-1) &&
                !database.list(1) &&
                database.indexOf(nullptr) == -1,
            "AI list, event, action, or bounds lookup changed.")) {
        return false;
    }

    bytes = makeDatabase(0);
    if (!check(
            database.decode(
                bytes.data(), bytes.size(), &error) &&
                database.version() == 0 &&
                database.list(0) &&
                database.list(0)->walkPointSpeed() == 10,
            "Version-zero AI lists did not retain retail's default "
            "walk-point speed.")) {
        return false;
    }

    bytes = makeDatabase(1);
    bytes[20] = 17;
    if (!check(
            !database.decode(
                bytes.data(), bytes.size(), &error) &&
                database.lists().empty(),
            "A non-retail AI event count was accepted.")) {
        return false;
    }

    bytes = makeDatabase(1);
    bytes.pop_back();
    if (!check(
            !database.decode(
                bytes.data(), bytes.size(), &error) &&
                database.lists().empty(),
            "A truncated AI database was accepted.")) {
        return false;
    }

    bytes = makeDatabase(1);
    bytes.push_back(0);
    if (!check(
            !database.decode(
                bytes.data(), bytes.size(), &error) &&
                database.lists().empty(),
            "Trailing AI database bytes were accepted.")) {
        return false;
    }

    const std::uint8_t invalid[] = {0, 1, 2, 3};
    return check(
        !database.decode(
            invalid, sizeof(invalid), &error) &&
            database.lists().empty() &&
            database.version() == 0,
        "An invalid AI header left partially decoded state.");
}

bool testRetailDatabase() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    osf::AiControlDatabase database;
    std::string error;
    if (!check(
            database.load(
                std::string(OPENSHADOWFLARE_SOURCE_DIR) +
                    "/tmp/ShadowFlare/System/Game/Parameter/"
                    "Control.aid",
                &error),
            "The retail Control.aid fixture could not be decoded.")) {
        std::cerr << error << '\n';
        return false;
    }

    std::size_t action_count = 0;
    for (const osf::AiControlList& list :
         database.lists()) {
        action_count += list.actionCount();
    }
    const osf::AiControlList* first = database.list(0);
    const osf::AiControlList* second = database.list(1);
    const osf::AiControlList* last = database.list(63);
    const std::string first_name{
        static_cast<char>(0x93),
        static_cast<char>(0x47),
        static_cast<char>(0x82),
        static_cast<char>(0xcc),
        static_cast<char>(0x91),
        static_cast<char>(0x83),
    };
    if (!check(
            database.version() == 1 &&
                database.lists().size() == 64 &&
                action_count == 1338 &&
                first &&
                first->name() == first_name &&
                first->walkPointSpeed() == 0 &&
                first->actionCount() == 1 &&
                database.find(first_name) == first &&
                database.indexOf(first) == 0 &&
                second &&
                second->walkPointSpeed() == 40 &&
                second->actionCount() == 12 &&
                last &&
                last->walkPointSpeed() == 40 &&
                last->actionCount() == 51,
            "The retail AI catalog dimensions or stable list facts "
            "changed.")) {
        return false;
    }

    const osf::AiActionData& first_action =
        first->event(0)->actions().front();
    const std::array<std::int32_t, 9> parameters{
        0, -1, 100, 0, 0, 0, 0, 0, 0};
    const std::array<std::int32_t, 6> conditions{
        0, 0, 0, 0, 0, 0};
    return check(
        first_action.event_number == 0 &&
            first_action.action_number == 0 &&
            first_action.parameters == parameters &&
            first_action.conditions == conditions,
        "The first retail AI action record changed.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    return testSyntheticDatabase() &&
                   testRetailDatabase()
               ? 0
               : 1;
}
