#ifndef OPENSHADOWFLARE_GAMEPLAY_BLACKJACK_HPP
#define OPENSHADOWFLARE_GAMEPLAY_BLACKJACK_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace osf {

class RetailRandom;

enum class BlackjackOutcome : std::int32_t {
    draw = 0,
    player_wins = 1,
    dealer_wins = 2,
};

struct BlackjackCard {
    std::int32_t suit = 0;
    std::int32_t rank = 1;
    bool hidden = false;
};

struct BlackjackHand {
    static constexpr std::size_t maximum_cards = 16;

    std::array<BlackjackCard, maximum_cards> cards{};
    std::size_t count = 0;
};

std::int32_t retailBlackjackScore(const BlackjackHand& hand);
BlackjackOutcome retailBlackjackOutcome(
    const BlackjackHand& player,
    const BlackjackHand& dealer);

struct GameplayBlackjackInput {
    bool pointer_primary_down = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
};

struct GameplayBlackjackResult {
    bool completed = false;
    bool pointer_consumed = false;
    BlackjackOutcome outcome = BlackjackOutcome::draw;
    std::int32_t audio_sample = -1;
};

class GameplayBlackjack {
public:
    static constexpr std::int32_t deal_updates = 15;
    static constexpr std::int32_t result_updates = 200;

    void open();
    void close();
    GameplayBlackjackResult update(
        const GameplayBlackjackInput& input,
        RetailRandom& random);

    bool active() const;
    bool initialDealComplete() const;
    bool playerFinished() const;
    bool resultVisible() const;
    bool dealAnimationActive() const;
    std::int32_t dealAnimationUpdate() const;
    bool dealingToPlayer() const;
    const BlackjackHand& playerHand() const;
    const BlackjackHand& dealerHand() const;
    BlackjackOutcome outcome() const;

private:
    bool cardAlreadyDealt(
        std::int32_t suit,
        std::int32_t rank) const;
    bool dealCard(
        BlackjackHand& hand,
        bool allow_joker,
        bool hidden,
        RetailRandom& random);
    void revealDealerHand();
    void beginResult();

    bool active_ = false;
    BlackjackHand player_;
    BlackjackHand dealer_;
    std::int32_t phase_counter_ = 0;
    bool initial_deal_complete_ = false;
    std::int32_t result_counter_ = 0;
    BlackjackOutcome outcome_ = BlackjackOutcome::draw;
    bool dealing_to_player_ = false;
    bool dealer_finished_ = false;
    bool player_finished_ = false;
    bool pending_card_ = false;
};

}  // namespace osf

#endif
