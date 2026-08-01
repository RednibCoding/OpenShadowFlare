#ifndef OPENSHADOWFLARE_LIBS_RKC_RPG_AICONTROL_HPP
#define OPENSHADOWFLARE_LIBS_RKC_RPG_AICONTROL_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace osf {

constexpr std::size_t kAiControlEventCount = 18;
constexpr std::size_t kAiParameterValueCount = 9;
constexpr std::size_t kAiConditionValueCount = 6;

struct AiActionData {
    std::int32_t event_number = 0;
    std::int32_t action_number = 0;
    std::array<
        std::int32_t,
        kAiParameterValueCount> parameters{};
    std::array<
        std::int32_t,
        kAiConditionValueCount> conditions{};
};

class AiEventData {
public:
    const std::vector<AiActionData>& actions() const;

private:
    friend class AiControlDatabase;

    std::vector<AiActionData> actions_;
};

class AiControlList {
public:
    const std::string& name() const;
    std::int32_t walkPointSpeed() const;
    const AiEventData* event(std::int32_t event_number) const;
    const std::array<
        AiEventData,
        kAiControlEventCount>& events() const;
    std::size_t actionCount() const;

private:
    friend class AiControlDatabase;

    std::string name_;
    std::int32_t walk_point_speed_ = 10;
    std::array<
        AiEventData,
        kAiControlEventCount> events_;
};

class AiControlDatabase {
public:
    bool load(
        const std::filesystem::path& path,
        std::string* error = nullptr);
    bool decode(
        const std::uint8_t* bytes,
        std::size_t size,
        std::string* error = nullptr);
    void clear();

    std::int32_t version() const;
    const AiControlList* list(std::int32_t index) const;
    const AiControlList* find(std::string_view name) const;
    std::int32_t indexOf(const AiControlList* list) const;
    const std::vector<AiControlList>& lists() const;

private:
    std::int32_t version_ = 0;
    std::vector<AiControlList> lists_;
};

}  // namespace osf

#endif
