#include "npc_actor.hpp"
#include "player_actor.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

namespace osf {
namespace {

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

std::string resourceDirectory(std::int32_t resource_id) {
    std::ostringstream name;
    name << std::setfill('0') << std::setw(8) << resource_id;
    return name.str();
}

template <typename Value>
void copyParts(
    std::vector<Value>& destination,
    const std::vector<Value>& source) {
    std::copy_n(
        source.begin(),
        std::min(destination.size(), source.size()),
        destination.begin());
}

WorldPosition movementStep(
    WorldPosition current,
    WorldPosition destination,
    std::int32_t speed) {
    const std::int64_t delta_x =
        static_cast<std::int64_t>(destination.x) - current.x;
    const std::int64_t delta_y =
        static_cast<std::int64_t>(destination.y) - current.y;
    const double distance = std::hypot(
        static_cast<double>(delta_x),
        static_cast<double>(delta_y));
    if (distance <= static_cast<double>(speed)) {
        return destination;
    }
    return {
        current.x + static_cast<std::int32_t>(
            static_cast<double>(delta_x) / distance * speed),
        current.y + static_cast<std::int32_t>(
            static_cast<double>(delta_y) / distance * speed),
    };
}

WorldPosition furthestWalkablePosition(
    const GroundMap& ground,
    const ObjectMap& objects,
    const ObjectBounds& bounds,
    WorldPosition start,
    WorldPosition end,
    bool* reached) {
    const std::int32_t delta_x = end.x - start.x;
    const std::int32_t delta_y = end.y - start.y;
    const std::int32_t steps = std::max(
        std::abs(delta_x), std::abs(delta_y));
    WorldPosition result = start;
    *reached = true;
    for (std::int32_t step = 1; step <= steps; ++step) {
        const WorldPosition position{
            start.x + delta_x * step / steps,
            start.y + delta_y * step / steps,
        };
        if (!positionIsWalkable(
                ground, objects, position, bounds)) {
            *reached = false;
            break;
        }
        result = position;
    }
    return result;
}

}  // namespace

bool NpcActor::load(
    const std::filesystem::path& data_root,
    const ScenarioPerson& person,
    std::string* error) {
    clear();
    if (person.resource_id < 0) {
        setError(error, "The person has no animation resource.");
        return false;
    }

    const std::filesystem::path root =
        data_root / "Character" / "PEOPLE" /
        resourceDirectory(person.resource_id);
    std::string asset_error;
    if (!patterns_.load(root / "Animation.Njp", &asset_error) ||
        !shadow_patterns_.load(
            root / "Animation.Sdw", &asset_error) ||
        !animation_.load(root / "Animation.Caf", &asset_error)) {
        setError(
            error,
            "The NPC animation could not be loaded: " +
                asset_error);
        clear();
        return false;
    }

    id_ = person.id;
    resource_id_ = person.resource_id;
    name_ = person.name;
    position_ = {person.world_x, person.world_y};
    destination_ = position_;
    judgement_ = {
        person.judgement_left,
        person.judgement_top,
        person.judgement_right,
        person.judgement_bottom,
    };
    direction_ = person.direction;
    walk_speed_ = std::max(person.walk_speed, 0);
    walk_duration_ = std::max(person.walk_duration, 0);
    idle_duration_ = std::max(person.idle_duration, 0);
    wander_min_ = {
        person.wander_left,
        person.wander_top,
    };
    wander_max_ = {
        person.wander_right,
        person.wander_bottom,
    };
    if (person.wander_bounds_relative) {
        wander_min_.x += position_.x;
        wander_min_.y += position_.y;
        wander_max_.x += position_.x;
        wander_max_.y += position_.y;
    }
    if (wander_min_.x > wander_max_.x) {
        std::swap(wander_min_.x, wander_max_.x);
    }
    if (wander_min_.y > wander_max_.y) {
        std::swap(wander_min_.y, wander_max_.y);
    }
    wandering_enabled_ =
        person.wandering_enabled &&
        walk_speed_ > 0 &&
        walk_duration_ > 0;
    random_.seed(static_cast<std::uint32_t>(person.id + 1));

    const std::size_t part_count = animation_.maxPartCount();
    part_visibility_.assign(part_count, 1);
    red_strength_.assign(part_count, 1000);
    green_strength_.assign(part_count, 1000);
    blue_strength_.assign(part_count, 1000);

    copyParts(part_visibility_, person.part_overrides);
    if (!person.part_visibility.empty()) {
        copyParts(part_visibility_, person.part_visibility);
        copyParts(red_strength_, person.red_strength);
        copyParts(green_strength_, person.green_strength);
        copyParts(blue_strength_, person.blue_strength);
    }
    if (error) {
        error->clear();
    }
    return true;
}

void NpcActor::clear() {
    id_ = -1;
    resource_id_ = -1;
    name_.clear();
    position_ = {};
    destination_ = {};
    judgement_ = {};
    direction_ = 0;
    animation_chart_ = 0;
    animation_frame_ = 0;
    action_counter_ = 0;
    walk_speed_ = 0;
    walk_duration_ = 0;
    idle_duration_ = 0;
    wander_min_ = {};
    wander_max_ = {};
    wandering_enabled_ = false;
    walking_ = false;
    random_.seed(1);
    part_visibility_.clear();
    red_strength_.clear();
    green_strength_.clear();
    blue_strength_.clear();
    patterns_.clear();
    shadow_patterns_.clear();
    animation_.clear();
}

void NpcActor::update(
    const GroundMap& ground,
    const ObjectMap& objects) {
    const auto retailRandom = [this]() {
        return random_.next();
    };
    const auto randomCoordinate =
        [&retailRandom](std::int32_t first, std::int32_t last) {
            const std::uint32_t span =
                static_cast<std::uint32_t>(last - first) + 1u;
            return first + static_cast<std::int32_t>(
                               retailRandom() % span);
        };

    if (!walking_) {
        animation_chart_ = 0;
        animation_frame_ = action_counter_;
        if (!wandering_enabled_ ||
            action_counter_++ < idle_duration_) {
            return;
        }

        destination_ = {
            randomCoordinate(wander_min_.x, wander_max_.x),
            randomCoordinate(wander_min_.y, wander_max_.y),
        };
        walking_ = destination_.x != position_.x ||
                   destination_.y != position_.y;
        action_counter_ = 0;
        if (!walking_) {
            return;
        }
    }

    animation_chart_ = 1;
    animation_frame_ = action_counter_;
    direction_ = retailDirectionForVector(
        destination_.x - position_.x,
        destination_.y - position_.y);

    const WorldPosition candidate =
        movementStep(position_, destination_, walk_speed_);
    bool reached = false;
    position_ = furthestWalkablePosition(
        ground,
        objects,
        judgement_,
        position_,
        candidate,
        &reached);
    ++action_counter_;
    if (!reached ||
        (position_.x == destination_.x &&
         position_.y == destination_.y) ||
        action_counter_ >= walk_duration_) {
        walking_ = false;
        destination_ = position_;
        action_counter_ = 0;
    }
}

std::int32_t NpcActor::id() const {
    return id_;
}

std::int32_t NpcActor::resourceId() const {
    return resource_id_;
}

const std::string& NpcActor::name() const {
    return name_;
}

WorldPosition NpcActor::position() const {
    return position_;
}

const ObjectBounds& NpcActor::judgement() const {
    return judgement_;
}

std::int32_t NpcActor::direction() const {
    return direction_;
}

std::int32_t NpcActor::animationChart() const {
    return animation_chart_;
}

std::int32_t NpcActor::animationFrame() const {
    return animation_frame_;
}

bool NpcActor::partEnabled(std::size_t part) const {
    return part < part_visibility_.size() &&
           part_visibility_[part] != 0;
}

std::int32_t NpcActor::partBrightness(std::size_t part) const {
    if (part >= red_strength_.size() ||
        part >= green_strength_.size() ||
        part >= blue_strength_.size()) {
        return 1000;
    }
    return std::clamp(
        (static_cast<std::int32_t>(red_strength_[part]) +
         static_cast<std::int32_t>(green_strength_[part]) +
         static_cast<std::int32_t>(blue_strength_[part])) /
            3,
        0,
        1000);
}

const gapi::NjpImage& NpcActor::patterns() const {
    return patterns_;
}

const gapi::NjpImage& NpcActor::shadowPatterns() const {
    return shadow_patterns_;
}

const gapi::CafAnimation& NpcActor::animation() const {
    return animation_;
}

}  // namespace osf
