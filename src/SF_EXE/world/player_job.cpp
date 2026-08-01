#include "player_job.hpp"

namespace osf {

std::int32_t retailScriptJobSelection(
    std::int32_t job) {
    switch (job) {
    case playerJobValue(PlayerJob::warrior):
        return 1;
    case playerJobValue(PlayerJob::hunter):
        return 2;
    case playerJobValue(PlayerJob::spellcaster):
        return 3;
    case playerJobValue(PlayerJob::mercenary):
    default:
        return 0;
    }
}

std::optional<PlayerJob> retailJobForScriptSelection(
    std::int32_t selection) {
    switch (selection) {
    case 1:
        return PlayerJob::warrior;
    case 2:
        return PlayerJob::hunter;
    case 3:
        return PlayerJob::spellcaster;
    default:
        return std::nullopt;
    }
}

}  // namespace osf
