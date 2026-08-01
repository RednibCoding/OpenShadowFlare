#include "core/retail_random.hpp"
#include "gapi/gapi.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "render/gameplay_blackjack_renderer.hpp"
#include "states/gameplay_blackjack.hpp"
#include "world/player_data.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

osf::BlackjackHand hand(
    std::initializer_list<osf::BlackjackCard> cards) {
    osf::BlackjackHand result;
    for (const osf::BlackjackCard& card : cards) {
        result.cards[result.count++] = card;
    }
    return result;
}

bool testRules() {
    const osf::BlackjackHand natural = hand({{0, 1}, {2, 13}});
    const osf::BlackjackHand soft_twenty_one =
        hand({{0, 1}, {1, 1}, {2, 9}});
    const osf::BlackjackHand joker_twenty_one =
        hand({{4, 0}, {3, 12}});
    const osf::BlackjackHand bust =
        hand({{0, 13}, {1, 12}, {2, 2}});
    const osf::BlackjackHand dealer_twenty_one =
        hand({{0, 7}, {1, 7}, {2, 7}});
    return check(
        osf::retailBlackjackScore(natural) == 21 &&
            osf::retailBlackjackScore(soft_twenty_one) == 21 &&
            osf::retailBlackjackScore(joker_twenty_one) == 21 &&
            osf::retailBlackjackScore(bust) == -1 &&
            osf::retailBlackjackOutcome(
                natural, dealer_twenty_one) ==
                osf::BlackjackOutcome::player_wins &&
            osf::retailBlackjackOutcome(
                dealer_twenty_one, natural) ==
                osf::BlackjackOutcome::dealer_wins &&
            osf::retailBlackjackOutcome(natural, natural) ==
                osf::BlackjackOutcome::draw,
        "The retail Blackjack score or natural-21 tie rule differs.");
}

bool testRound() {
    osf::GameplayBlackjack game;
    osf::RetailRandom random(1);
    game.open();
    std::vector<std::int32_t> audio;
    for (std::int32_t update = 0; update < 60; ++update) {
        const osf::GameplayBlackjackResult result =
            game.update({}, random);
        if (result.audio_sample >= 0) {
            audio.push_back(result.audio_sample);
        }
    }
    std::set<std::int32_t> cards;
    for (const osf::BlackjackHand* dealt :
         {&game.playerHand(), &game.dealerHand()}) {
        for (std::size_t index = 0; index < dealt->count; ++index) {
            const osf::BlackjackCard& card = dealt->cards[index];
            cards.insert(card.suit * 100 + card.rank);
        }
    }
    if (!check(
            game.initialDealComplete() &&
                game.playerHand().count == 2 &&
                game.dealerHand().count == 2 &&
                game.dealerHand().cards[1].hidden &&
                cards.size() == 4 &&
                audio == std::vector<std::int32_t>({44, 44, 44, 44}),
            "The timed retail Blackjack opening deal differs.")) {
        return false;
    }

    game.update({true, 230, 338}, random);
    if (!check(
            game.dealAnimationActive() &&
                game.dealingToPlayer(),
            "The Hit button did not begin the retail card transition.")) {
        return false;
    }
    bool hit_dealt = false;
    for (std::int32_t update = 0; update < 20; ++update) {
        const osf::GameplayBlackjackResult result =
            game.update({}, random);
        if (result.audio_sample == 44) {
            hit_dealt = true;
        }
        if (game.playerHand().count == 3) {
            break;
        }
    }
    if (!check(
            hit_dealt && game.playerHand().count == 3,
            "The Hit transition did not add one unique player card.")) {
        return false;
    }

    osf::GameplayBlackjack round;
    osf::RetailRandom round_random(1);
    round.open();
    for (std::int32_t update = 0; update < 60; ++update) {
        round.update({}, round_random);
    }
    round.update({true, 336, 338}, round_random);
    if (!check(
            round.playerFinished() &&
                !round.dealerHand().cards[1].hidden,
            "Stand did not lock the player and reveal the dealer hand.")) {
        return false;
    }
    for (std::int32_t update = 0;
         update < 500 && !round.resultVisible();
         ++update) {
        round.update({}, round_random);
    }
    if (!check(
            round.resultVisible(),
            "The dealer did not settle the Blackjack round.")) {
        return false;
    }
    const osf::BlackjackOutcome expected =
        osf::retailBlackjackOutcome(
            round.playerHand(), round.dealerHand());
    const osf::GameplayBlackjackResult result_frame =
        round.update({true, 10, 10}, round_random);
    if (!check(
            !result_frame.completed &&
                round.outcome() == expected &&
                (result_frame.audio_sample == -1 ||
                 result_frame.audio_sample == 64 ||
                 result_frame.audio_sample == 65),
            "The first retail result frame did not publish its outcome.")) {
        return false;
    }
    const osf::GameplayBlackjackResult dismissed =
        round.update({true, 10, 10}, round_random);
    return check(
        dismissed.completed && dismissed.outcome == expected &&
            !round.active(),
        "A click after the first result frame did not close Blackjack.");
}

class RecordingBackend final : public osf::gapi::Backend {
public:
    struct Pattern {
        std::size_t index = 0;
        osf::gapi::PatternDraw draw;
    };

    void beginFrame(osf::gapi::Color) override {}
    bool drawPattern(
        const osf::gapi::NjpImage&,
        std::size_t index,
        const osf::gapi::PatternDraw& draw) override {
        patterns.push_back({index, draw});
        return true;
    }
    bool drawBitmap(
        const osf::gapi::BitmapImage&,
        const osf::gapi::BitmapDraw&) override {
        return true;
    }
    bool drawText(
        const osf::gapi::NjpImage&,
        std::string_view,
        const osf::gapi::TextDraw&) override {
        return true;
    }
    bool drawRectangle(
        const osf::gapi::RectangleDraw&) override {
        return true;
    }
    void endFrame() override {}

    std::vector<Pattern> patterns;
};

bool testRetailArtwork() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    osf::WorldScene world;
    osf::PlayerLoadRequest player;
    player.name = "Blackjack";
    std::string error;
    osf::gapi::NjpImage cards;
    osf::gapi::NjpImage status;
    if (!world.loadInitialScenario(root, player, &error) ||
        !cards.load(
            root / "System" / "Game" / "Pattern" / "Card.Njp",
            &error) ||
        !status.load(
            root / "System" / "Game" / "Pattern" / "Status.njp",
            &error)) {
        std::cerr << error << '\n';
        return false;
    }
    osf::GameplayBlackjack game;
    osf::RetailRandom random(1);
    game.open();
    game.update({}, random);
    RecordingBackend renderer;
    osf::renderGameplayBlackjack(
        renderer, cards, status, game, world, 1);
    const auto contains = [&renderer](std::size_t index) {
        return std::any_of(
            renderer.patterns.begin(),
            renderer.patterns.end(),
            [index](const RecordingBackend::Pattern& pattern) {
                return pattern.index == index;
            });
    };
    return check(
        renderer.patterns.size() > 15 &&
            renderer.patterns[0].index == 65 &&
            renderer.patterns[0].draw.x == 32 &&
            renderer.patterns[0].draw.y == 40 &&
            renderer.patterns[8].index == 119 &&
            contains(55) && contains(56) &&
            contains(59) && contains(60) &&
            contains(63) && contains(53) && contains(54),
        "The retail Card.njp board, deck, controls, or transition differs.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    return testRules() && testRound() && testRetailArtwork() ? 0 : 1;
}
