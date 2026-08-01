#include "movement_destination_selector.hpp"

#include "movement_controller.hpp"

#include <cmath>
#include <cstdint>
#include <limits>

namespace osf {
namespace {

constexpr double kRandomAngleStep =
    0.0010471973333333333;
constexpr double kRectangleEdgeAngle =
    0.5235987756;

std::int32_t floorHalf(std::int64_t value) {
    if (value >= 0) {
        return static_cast<std::int32_t>(value / 2);
    }
    return static_cast<std::int32_t>(
        -((-value + 1) / 2));
}

bool samePosition(
    WorldPosition first,
    WorldPosition second) {
    return first.x == second.x &&
           first.y == second.y;
}

bool inclusiveRange(
    std::int32_t minimum,
    std::int32_t maximum,
    std::int32_t& range) {
    const std::int64_t calculated =
        static_cast<std::int64_t>(maximum) -
        minimum + 1;
    if (calculated <= 0 ||
        calculated >
            std::numeric_limits<std::int32_t>::max()) {
        return false;
    }
    range = static_cast<std::int32_t>(calculated);
    return true;
}

WorldPosition pointAtDistance(
    WorldPosition from,
    WorldPosition toward,
    std::int32_t distance) {
    const double delta_x =
        static_cast<double>(toward.x) - from.x;
    const double delta_y =
        static_cast<double>(toward.y) - from.y;
    const double length = std::hypot(delta_x, delta_y);
    const double scale =
        length == 0.0 ? 1.0 : distance / length;
    return {
        from.x + static_cast<std::int32_t>(
                     delta_x * scale),
        from.y + static_cast<std::int32_t>(
                     delta_y * scale),
    };
}

WorldPosition randomTurn(
    WorldPosition from,
    WorldPosition toward,
    RetailRandom& random) {
    const std::int64_t delta_x =
        static_cast<std::int64_t>(toward.x) - from.x;
    const std::int64_t delta_y =
        static_cast<std::int64_t>(toward.y) - from.y;
    const std::int32_t distance =
        static_cast<std::int32_t>(
            std::hypot(
                static_cast<double>(delta_x),
                static_cast<double>(delta_y)));
    const std::int32_t angle_step =
        random.next() % 2001 - 1000;
    const double angle =
        std::atan2(
            static_cast<double>(delta_y),
            static_cast<double>(delta_x)) +
        angle_step * kRandomAngleStep;
    return {
        from.x + static_cast<std::int32_t>(
                     std::cos(angle) * distance),
        from.y + static_cast<std::int32_t>(
                     std::sin(angle) * distance),
    };
}

MovementDestinationResult inactive(
    WorldPosition position) {
    return {false, position};
}

bool isApproachMode(MovementDestinationMode mode) {
    return mode ==
               MovementDestinationMode::
                   approach_scenario_actor ||
           mode ==
               MovementDestinationMode::approach_player;
}

bool isRetreatMode(MovementDestinationMode mode) {
    return mode ==
               MovementDestinationMode::
                   retreat_from_scenario_actor ||
           mode ==
               MovementDestinationMode::
                   retreat_from_player;
}

MovementTargetKind targetKind(
    MovementDestinationMode mode) {
    if (mode ==
            MovementDestinationMode::approach_player ||
        mode ==
            MovementDestinationMode::retreat_from_player) {
        return MovementTargetKind::player;
    }
    return MovementTargetKind::scenario_actor;
}

}  // namespace

void MovementDestinationSelector::reset() {
    request_ = {};
    destination_ = {};
    target_position_ = {};
    counter_ = 0;
}

void MovementDestinationSelector::initialize(
    const MovementDestinationRequest& request,
    WorldPosition position) {
    request_ = request;
    destination_ = position;
    target_position_ = request.destination;
    counter_ = 0;
}

MovementDestinationResult
MovementDestinationSelector::update(
    const MovementDestinationContext& context,
    RetailRandom& random) {
    if (request_.mode ==
        MovementDestinationMode::fixed_point) {
        destination_ = request_.destination;
        return {true, destination_};
    }

    if (request_.mode ==
        MovementDestinationMode::patrol) {
        if (counter_ == request_.duration) {
            return inactive(context.position);
        }
        if (counter_ == 0) {
            std::int32_t horizontal_range = 0;
            std::int32_t vertical_range = 0;
            if (!inclusiveRange(
                    request_.destination_bounds.left,
                    request_.destination_bounds.right,
                    horizontal_range) ||
                !inclusiveRange(
                    request_.destination_bounds.top,
                    request_.destination_bounds.bottom,
                    vertical_range)) {
                return inactive(context.position);
            }
            target_position_ = {
                request_.destination_bounds.left +
                    random.next() % horizontal_range,
                request_.destination_bounds.top +
                    random.next() % vertical_range,
            };
        }
        destination_ = target_position_;
        ++counter_;
        return {true, destination_};
    }

    if (request_.mode ==
        MovementDestinationMode::rectangle_edge) {
        const std::int64_t horizontal_sum =
            static_cast<std::int64_t>(
                request_.destination_bounds.left) +
            request_.destination_bounds.right;
        const std::int64_t vertical_sum =
            static_cast<std::int64_t>(
                request_.destination_bounds.top) +
            request_.destination_bounds.bottom;
        const WorldPosition center{
            floorHalf(horizontal_sum),
            floorHalf(vertical_sum),
        };
        const double angle =
            samePosition(context.position, center)
            ? 0.0
            : std::atan2(
                  static_cast<double>(
                      static_cast<std::int64_t>(
                          context.position.y) -
                      center.y),
                  static_cast<double>(
                      static_cast<std::int64_t>(
                          context.position.x) -
                      center.x)) +
                  kRectangleEdgeAngle;
        const std::int64_t width =
            static_cast<std::int64_t>(
                request_.destination_bounds.right) -
            request_.destination_bounds.left;
        const std::int64_t height =
            static_cast<std::int64_t>(
                request_.destination_bounds.bottom) -
            request_.destination_bounds.top;
        destination_ = {
            center.x + static_cast<std::int32_t>(
                           std::cos(angle) *
                           static_cast<double>(width) *
                           0.5),
            center.y + static_cast<std::int32_t>(
                           std::sin(angle) *
                           static_cast<double>(height) *
                           0.5),
        };
        target_position_ = destination_;
        return {true, destination_};
    }

    if (!isApproachMode(request_.mode) &&
        !isRetreatMode(request_.mode)) {
        return inactive(context.position);
    }
    if (!context.resolve_target) {
        return inactive(context.position);
    }
    const MovementTargetState target =
        context.resolve_target(
            targetKind(request_.mode),
            request_.target_identifier);
    if (!target.found) {
        return inactive(context.position);
    }

    const std::int32_t distance =
        distanceBetweenBounds(
            context.position,
            context.bounds,
            target.position,
            target.bounds);
    if (isApproachMode(request_.mode)) {
        if (distance <= request_.stop_distance) {
            return inactive(context.position);
        }
        target_position_ = target.position;
    } else {
        if (distance >= request_.stop_distance) {
            return inactive(context.position);
        }
        target_position_ = pointAtDistance(
            target.position,
            context.position,
            request_.stop_distance + 1);
    }

    const std::int32_t refresh_interval =
        request_.target_refresh_interval == 0
        ? 1
        : request_.target_refresh_interval;
    const bool refresh =
        counter_ % refresh_interval == 0 ||
        samePosition(destination_, context.position);
    bool random_turn = false;
    if (refresh) {
        random_turn =
            random.next() % 100 <
            request_.random_turn_chance;
        destination_ = random_turn
            ? randomTurn(
                  context.position,
                  target_position_,
                  random)
            : target_position_;
    }
    ++counter_;

    if (request_.mode ==
        MovementDestinationMode::
            retreat_from_scenario_actor) {
        return inactive(context.position);
    }
    return {true, destination_};
}

const MovementDestinationRequest&
MovementDestinationSelector::request() const {
    return request_;
}

WorldPosition
MovementDestinationSelector::destination() const {
    return destination_;
}

WorldPosition
MovementDestinationSelector::targetPosition() const {
    return target_position_;
}

std::int32_t MovementDestinationSelector::counter() const {
    return counter_;
}

}  // namespace osf
