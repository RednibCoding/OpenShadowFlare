#include "lwl.h"
#include "core/command_line.hpp"
#include "core/game_config.hpp"
#include "runtime/game_runtime.hpp"

#include <cstring>
#include <filesystem>

namespace {

bool isSmokeTest(int argc, char** argv) {
    for (int index = 1; index < argc; ++index) {
        if (std::strcmp(argv[index], "--smoke-test") == 0) {
            return true;
        }
    }
    return false;
}

#if defined(OSF_PLATFORM_PSP)

std::filesystem::path findDataRoot(int argc, char** argv) {
    if (argc > 0 && argv && argv[0]) {
        const std::string executable = argv[0];
        const std::size_t separator =
            executable.find_last_of("/\\");
        if (separator != std::string::npos) {
            return std::filesystem::path(
                executable.substr(0, separator));
        }
    }
    return std::filesystem::path("ms0:/PSP/GAME/OpenShadowFlare");
}

#else

std::filesystem::path findDataRoot() {
    const auto isDataRoot =
        [](const std::filesystem::path& candidate) {
            std::error_code error;
            const bool hasConfig = std::filesystem::is_regular_file(
                candidate / "SFlare.Cfg", error);
            error.clear();
            const bool hasTitle = std::filesystem::is_regular_file(
                candidate / "System" / "Title" / "Pattern" /
                    "Title.njp",
                error);
            return hasConfig && hasTitle;
        };
    const auto searchParents =
        [&isDataRoot](std::filesystem::path directory) {
            for (;;) {
                const std::filesystem::path candidates[] = {
                    directory,
                    directory / "ShadowFlare",
                    directory / "tmp" / "ShadowFlare",
                };
                for (const std::filesystem::path& candidate :
                     candidates) {
                    if (isDataRoot(candidate)) {
                        return candidate;
                    }
                }

                const std::filesystem::path parent =
                    directory.parent_path();
                if (parent.empty() || parent == directory) {
                    break;
                }
                directory = parent;
            }
            return std::filesystem::path{};
        };

    std::error_code error;
    const std::filesystem::path fromWorkingDirectory =
        searchParents(std::filesystem::current_path(error));
    if (!fromWorkingDirectory.empty()) {
        return fromWorkingDirectory;
    }

    char executablePath[4096]{};
    if (lwl_exe_path(
            executablePath,
            static_cast<int>(sizeof(executablePath)))) {
        const std::filesystem::path fromExecutable =
            searchParents(
                std::filesystem::absolute(
                    executablePath, error).parent_path());
        if (!fromExecutable.empty()) {
            return fromExecutable;
        }
    }

    const std::filesystem::path fallbacks[] = {
        ".",
        std::filesystem::path("tmp") / "ShadowFlare",
    };
    for (const std::filesystem::path& candidate : fallbacks) {
        if (isDataRoot(candidate)) {
            return candidate;
        }
    }
    return ".";
}

#endif  // OSF_PLATFORM_PSP

}  // namespace

int main(int argc, char** argv) {
#if defined(OSF_PLATFORM_PSP)
    const std::filesystem::path dataRoot = findDataRoot(argc, argv);
#else
    const std::filesystem::path dataRoot = findDataRoot();
#endif
    osf::GameConfig gameConfig;

    // Retail ignores config-load failure and retains its constructor defaults.
    osf::loadGameConfigFile(
        (dataRoot / "SFlare.Cfg").string(), gameConfig);
    for (int index = 1; index < argc; ++index) {
        osf::applyRetailCommandLine(argv[index], gameConfig);
    }

    return osf::runtime::runGame(
        dataRoot, gameConfig, isSmokeTest(argc, argv));
}
