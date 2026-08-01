#include "gameplay_blackjack.hpp"

#include "core/retail_random.hpp"

#include <algorithm>
#include <cstddef>

namespace osf {
namespace {

bool inside(
    std::int32_t x,
    std::int32_t y,
    std::int32_t left,
    std::int32_t top,
    std::int32_t right,
    std::int32_t bottom) {
    return x > left && x < right && y > top && y < bottom;
}

}  // namespace

std::int32_t retailBlackjackScore(const BlackjackHand& hand) {
    constexpr std::array<std::int32_t, 13> values{{
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10,
    }};
    std::int32_t score = 0;
    std::int32_t flexible_aces = 0;
    const std::size_t count = std::min(
        hand.count, hand.cards.size());
    for (std::size_t index = 0; index < count; ++index) {
        const BlackjackCard& card = hand.cards[index];
        if (card.suit == 4) {
            ++flexible_aces;
            continue;
        }
        if (card.rank < 1 || card.rank > 13) {
            return -1;
        }
        const std::int32_t value = values[
            static_cast<std::size_t>(card.rank - 1)];
        if (value == 1) {
            ++flexible_aces;
        } else {
            score += value;
        }
    }
    while (flexible_aces > 0) {
        const std::int32_t remaining = flexible_aces + 10;
        if (21 - score < remaining) {
            ++score;
        } else {
            score += 11;
        }
        --flexible_aces;
    }
    return score > 21 ? -1 : score;
}

BlackjackOutcome retailBlackjackOutcome(
    const BlackjackHand& player,
    const BlackjackHand& dealer) {
    const std::int32_t dealer_score =
        retailBlackjackScore(dealer);
    const std::int32_t player_score =
        retailBlackjackScore(player);
    if (dealer_score < player_score) {
        return BlackjackOutcome::player_wins;
    }
    if (player_score < dealer_score) {
        return BlackjackOutcome::dealer_wins;
    }
    if (player_score == 21) {
        if (dealer.count == 2 && player.count != 2) {
            return BlackjackOutcome::dealer_wins;
        }
        if (dealer.count != 2 && player.count == 2) {
            return BlackjackOutcome::player_wins;
        }
    }
    return BlackjackOutcome::draw;
}

void GameplayBlackjack::open() {
    active_ = true;
    player_ = {};
    dealer_ = {};
    phase_counter_ = 0;
    initial_deal_complete_ = false;
    result_counter_ = 0;
    outcome_ = BlackjackOutcome::draw;
    dealing_to_player_ = false;
    dealer_finished_ = false;
    player_finished_ = false;
    pending_card_ = false;
}

void GameplayBlackjack::close() {
    active_ = false;
}

GameplayBlackjackResult GameplayBlackjack::update(
    const GameplayBlackjackInput& input,
    RetailRandom& random) {
    GameplayBlackjackResult result;
    if (!active_) {
        return result;
    }
    result.pointer_consumed = input.pointer_primary_down;

    if (result_counter_ != 0) {
        if (input.pointer_primary_down &&
            result_counter_ <= result_updates - 1) {
            result_counter_ = 1;
        }
        if (result_counter_ == result_updates) {
            outcome_ = retailBlackjackOutcome(player_, dealer_);
            if (outcome_ == BlackjackOutcome::player_wins) {
                result.audio_sample = 64;
            } else if (
                outcome_ == BlackjackOutcome::dealer_wins) {
                result.audio_sample = 65;
            }
        }
        --result_counter_;
        if (result_counter_ == 0) {
            result.completed = true;
            result.outcome = outcome_;
            close();
        }
        return result;
    }

    if (!initial_deal_complete_) {
        ++phase_counter_;
        if (phase_counter_ == 15) {
            dealing_to_player_ = true;
            if (dealCard(dealer_, false, false, random)) {
                result.audio_sample = 44;
            }
        } else if (phase_counter_ == 30) {
            dealing_to_player_ = false;
            if (dealCard(player_, false, false, random)) {
                result.audio_sample = 44;
            }
        } else if (phase_counter_ == 45) {
            dealing_to_player_ = true;
            if (dealCard(dealer_, true, true, random)) {
                result.audio_sample = 44;
            }
        } else if (phase_counter_ == 60) {
            dealing_to_player_ = true;
            if (dealCard(player_, false, false, random)) {
                result.audio_sample = 44;
            }
            initial_deal_complete_ = true;
            phase_counter_ = 0;
        }
        return result;
    }

    if (dealing_to_player_ && input.pointer_primary_down) {
        if (inside(
                input.pointer_x,
                input.pointer_y,
                229,
                337,
                328,
                370) &&
            phase_counter_ % deal_updates == 0) {
            pending_card_ = true;
            ++phase_counter_;
        }
        if (inside(
                input.pointer_x,
                input.pointer_y,
                335,
                337,
                434,
                370) &&
            phase_counter_ % deal_updates == 0) {
            dealing_to_player_ = false;
            player_finished_ = true;
            if (dealer_finished_) {
                beginResult();
            }
            phase_counter_ = 1;
            revealDealerHand();
        }
    }

    if (result_counter_ != 0) {
        return result;
    }
    if (phase_counter_ % deal_updates != 0) {
        ++phase_counter_;
    }
    if (phase_counter_ % deal_updates == 0 &&
        !dealing_to_player_) {
        revealDealerHand();
        if (!pending_card_ && !dealer_finished_) {
            const std::int32_t score =
                retailBlackjackScore(dealer_);
            if (score > 16) {
                dealer_finished_ = true;
            }
            if (score == -1 || dealer_finished_) {
                beginResult();
            } else {
                pending_card_ = true;
                ++phase_counter_;
            }
        } else if (pending_card_ && !dealer_finished_) {
            if (dealCard(dealer_, false, false, random)) {
                result.audio_sample = 44;
            }
            pending_card_ = false;
            ++phase_counter_;
        }
    }
    if (phase_counter_ % deal_updates == 0 &&
        dealing_to_player_ && pending_card_) {
        if (dealCard(player_, false, false, random)) {
            result.audio_sample = 44;
        }
        ++phase_counter_;
        if (retailBlackjackScore(player_) == -1) {
            player_finished_ = true;
            dealer_finished_ = true;
            beginResult();
            revealDealerHand();
        }
        pending_card_ = false;
    }
    return result;
}

bool GameplayBlackjack::active() const {
    return active_;
}

bool GameplayBlackjack::initialDealComplete() const {
    return initial_deal_complete_;
}

bool GameplayBlackjack::playerFinished() const {
    return player_finished_;
}

bool GameplayBlackjack::resultVisible() const {
    return result_counter_ != 0;
}

bool GameplayBlackjack::dealAnimationActive() const {
    const std::int32_t phase = dealAnimationUpdate();
    return phase != 0 &&
           (!initial_deal_complete_ || pending_card_);
}

std::int32_t GameplayBlackjack::dealAnimationUpdate() const {
    return phase_counter_ % deal_updates;
}

bool GameplayBlackjack::dealingToPlayer() const {
    return dealing_to_player_;
}

const BlackjackHand& GameplayBlackjack::playerHand() const {
    return player_;
}

const BlackjackHand& GameplayBlackjack::dealerHand() const {
    return dealer_;
}

BlackjackOutcome GameplayBlackjack::outcome() const {
    return outcome_;
}

bool GameplayBlackjack::cardAlreadyDealt(
    std::int32_t suit,
    std::int32_t rank) const {
    const auto contains = [suit, rank](const BlackjackHand& hand) {
        const std::size_t count = std::min(
            hand.count, hand.cards.size());
        for (std::size_t index = 0; index < count; ++index) {
            if (hand.cards[index].suit == suit &&
                hand.cards[index].rank == rank) {
                return true;
            }
        }
        return false;
    };
    return contains(player_) || contains(dealer_);
}

bool GameplayBlackjack::dealCard(
    BlackjackHand& hand,
    bool allow_joker,
    bool hidden,
    RetailRandom& random) {
    if (hand.count >= hand.cards.size()) {
        return false;
    }
    const std::int32_t deck_size = allow_joker ? 53 : 52;
    for (;;) {
        const std::int32_t card = random.next() % deck_size;
        const std::int32_t suit = card / 13;
        const std::int32_t rank = suit == 4 ? 0 : card % 13 + 1;
        if (cardAlreadyDealt(suit, rank)) {
            continue;
        }
        hand.cards[hand.count] = {suit, rank, hidden};
        ++hand.count;
        return true;
    }
}

void GameplayBlackjack::revealDealerHand() {
    for (std::size_t index = 0;
         index < dealer_.count && index < dealer_.cards.size();
         ++index) {
        dealer_.cards[index].hidden = false;
    }
}

void GameplayBlackjack::beginResult() {
    result_counter_ = result_updates;
}

}  // namespace osf
