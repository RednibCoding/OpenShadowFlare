#include "gameplay_blackjack_renderer.hpp"

#include "character_renderer.hpp"
#include "gapi/gapi.hpp"
#include "states/gameplay_blackjack.hpp"
#include "world/companion_actor.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstddef>

namespace osf {
namespace {

std::size_t cardPattern(const BlackjackCard& card) {
    if (card.hidden) {
        return 53;
    }
    return static_cast<std::size_t>(
        card.suit * 13 - 1 + card.rank);
}

void drawCard(
    gapi::Backend& renderer,
    const gapi::NjpImage& patterns,
    const BlackjackCard& card,
    std::int32_t x,
    std::int32_t y) {
    renderer.drawPattern(
        patterns, 54, {x - 3, y + 3, 1000, 1000, 500});
    renderer.drawPattern(patterns, cardPattern(card), {x, y});
}

void drawHand(
    gapi::Backend& renderer,
    const gapi::NjpImage& patterns,
    const BlackjackHand& hand,
    bool player) {
    const std::size_t count = std::min(
        hand.count, hand.cards.size());
    if (count == 0) {
        return;
    }
    const std::int32_t base_x = player ? 158 : 182;
    const std::int32_t y = player ? 210 : 70;
    if (count < 5) {
        for (std::size_t index = 0; index < count; ++index) {
            drawCard(
                renderer,
                patterns,
                hand.cards[index],
                base_x + static_cast<std::int32_t>(index) * 80,
                y);
        }
        return;
    }
    const std::int32_t step =
        240 / static_cast<std::int32_t>(count - 1);
    for (std::size_t index = 0; index + 1 < count; ++index) {
        drawCard(
            renderer,
            patterns,
            hand.cards[index],
            base_x + static_cast<std::int32_t>(index) * step,
            y);
    }
    drawCard(
        renderer,
        patterns,
        hand.cards[count - 1],
        player ? 398 : 422,
        y);
}

void drawPlayer(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::uint32_t gameplay_counter) {
    const WorldPosition position = world.playerRenderPosition(1.0);
    const ScreenPosition screen = calculateRealPosition(position);
    renderCharacterAnimationPass(
        renderer,
        world.playerAnimation(),
        world.playerPatterns(),
        world.playerShadowPatterns(),
        position,
        9,
        5,
        static_cast<std::int32_t>(gameplay_counter),
        [&world](std::size_t part) {
            return world.playerPartEnabled(part);
        },
        [&world](std::size_t part) {
            return CharacterColorStrength{
                world.playerPartRedStrength(part),
                world.playerPartGreenStrength(part),
                world.playerPartBlueStrength(part),
            };
        },
        screen.x - 504,
        screen.y - 320,
        false,
        1000);
}

void drawCompanion(
    gapi::Backend& renderer,
    const CompanionActor& companion,
    std::uint32_t gameplay_counter) {
    const WorldPosition position = companion.renderPosition(1.0);
    const ScreenPosition screen = calculateRealPosition(position);
    renderCharacterAnimationPass(
        renderer,
        companion.animation(),
        companion.patterns(),
        companion.shadowPatterns(),
        position,
        0,
        5,
        static_cast<std::int32_t>(gameplay_counter),
        [&companion](std::size_t part) {
            return companion.partEnabled(part);
        },
        [&companion](std::size_t part) {
            return CharacterColorStrength{
                companion.partRedStrength(part),
                companion.partGreenStrength(part),
                companion.partBlueStrength(part),
            };
        },
        screen.x - 564,
        screen.y - 340,
        false,
        1000);
}

}  // namespace

void renderGameplayBlackjack(
    gapi::Backend& renderer,
    const gapi::NjpImage& card_patterns,
    const gapi::NjpImage& status_patterns,
    const GameplayBlackjack& blackjack,
    const WorldScene& world,
    std::uint32_t gameplay_counter) {
    if (!blackjack.active() || !world.hasPlayer()) {
        return;
    }

    renderer.drawPattern(card_patterns, 65, {32, 40});
    for (std::int32_t x = 562; x > 492; x -= 10) {
        renderer.drawPattern(card_patterns, 53, {x, 60});
    }
    renderer.drawPattern(status_patterns, 119);
    drawHand(
        renderer,
        card_patterns,
        blackjack.dealerHand(),
        false);
    drawHand(
        renderer,
        card_patterns,
        blackjack.playerHand(),
        true);

    renderer.drawPattern(card_patterns, 55, {80, 120});
    renderer.drawPattern(
        card_patterns, 56, {80, 120, 1000, 1000, 500});
    drawPlayer(renderer, world, gameplay_counter);
    if (world.hasCompanion()) {
        drawCompanion(
            renderer, world.companion(), gameplay_counter);
    }

    if (!blackjack.playerFinished()) {
        renderer.drawPattern(
            card_patterns, 63, {227, 341, 1000, 1000, 500});
        renderer.drawPattern(card_patterns, 59, {230, 338});
        renderer.drawPattern(
            card_patterns, 63, {333, 341, 1000, 1000, 500});
        renderer.drawPattern(card_patterns, 60, {336, 338});
    }

    if (blackjack.dealAnimationActive()) {
        const bool player = blackjack.dealingToPlayer();
        const BlackjackHand& hand = player
            ? blackjack.playerHand()
            : blackjack.dealerHand();
        const std::int32_t count = static_cast<std::int32_t>(
            std::min(hand.count, hand.cards.size()));
        const std::int32_t target_x = count + 1 < 5
            ? (count + 1) * 80 + 78 + (player ? 0 : 24)
            : 398 + (player ? 0 : 24);
        const std::int32_t target_y = player ? 210 : 70;
        const std::int32_t phase =
            blackjack.dealAnimationUpdate();
        const std::int32_t x =
            492 + (target_x - 492) * phase /
                      GameplayBlackjack::deal_updates;
        const std::int32_t y =
            60 + (target_y - 60) * phase /
                     GameplayBlackjack::deal_updates;
        renderer.drawPattern(
            card_patterns, 54, {x - 3, y + 3, 1000, 1000, 500});
        renderer.drawPattern(card_patterns, 53, {x, y});
    }

    if (!blackjack.resultVisible()) {
        return;
    }
    const std::int32_t player_score =
        retailBlackjackScore(blackjack.playerHand());
    if (blackjack.playerHand().count == 2 && player_score == 21) {
        renderer.drawPattern(card_patterns, 57, {196, 242});
    } else if (player_score == -1) {
        renderer.drawPattern(card_patterns, 58, {196, 242});
    }
    const std::int32_t dealer_score =
        retailBlackjackScore(blackjack.dealerHand());
    if (blackjack.dealerHand().count == 2 && dealer_score == 21) {
        renderer.drawPattern(card_patterns, 57, {220, 102});
    } else if (dealer_score == -1) {
        renderer.drawPattern(card_patterns, 58, {220, 102});
    }

    std::size_t dealer_result = 66;
    std::size_t player_result = 66;
    if (blackjack.outcome() == BlackjackOutcome::player_wins) {
        dealer_result = 62;
        player_result = 61;
    } else if (
        blackjack.outcome() == BlackjackOutcome::dealer_wins) {
        dealer_result = 61;
        player_result = 62;
    }
    renderer.drawPattern(
        card_patterns, 64, {36, 143, 1000, 1000, 500});
    renderer.drawPattern(card_patterns, dealer_result, {39, 140});
    renderer.drawPattern(
        card_patterns, 64, {483, 203, 1000, 1000, 500});
    renderer.drawPattern(card_patterns, player_result, {486, 200});
}

}  // namespace osf
